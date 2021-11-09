/*
Header file for the files.c source file
*/

#pragma once

// include the catch-all header
#include "general.h"

/*
Global variable to keep track of the current directory
All commands using file paths will treat this as the default/base file path
*/
//char currentDirectory[MAX_PATH];

/*
Initialize/reset the current directory to the directory this program launched from
Expects no arguments
Returns nothing
*/
void getLaunchDirectory();

/*
Return the current directory
*/
char* getCurrentDirectory();

/*
Change the current directory similar to using the 'cd' command in command prompt
Expects a file path and a socket as arguments
The file path can be relative ('Desktop') or absolute ('C:\Users\...\Desktop')
The socket will be used to send either an error message or the new current directory
Returns nothing, but the global 'currentDirectory' will be updated if successful
*/
void changeDirectory(char* dirName, SOCKET *sock);

/*
List the contents of the current directory similar to using 'dir' or 'ls'
Expects a socket as the only argument
The socket will be used to send either an error message or the directory contents
Returns nothing
TODO - optional argument to list contents of an arbitrary directory
*/
void listDirectoryContents(SOCKET *sock);