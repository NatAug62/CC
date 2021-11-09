/*
Header file for the utils.c source file
*/

#pragma once

// include the catch-all header
#include "general.h"

/*
Connect to a TCP server on a given port at the given IP address
Expects an IP address, port number, and socket as arguments
The provided socket will be set to a connected socket on success and INVALID_SOCKET on failure
Returns 0 on success or the associated error code on failure
If an error occurs, an error message will also be printed
*/
int connectToTCP(const char* addr, int port, SOCKET *sock);

/*
Send a string over a given socket
Expects a socket and a string as arguments
Returns 0 unless a socket error occurs, in which case the error code is returned instead
*/
int sendString(const char* data, SOCKET *sock);

/*
Shift the contents of a char array left
Expects a char array, array length, and shift amount as arguments
Returns nothing
*/
void shiftStringLeft(char* buffer, int len, int shift);

/*
Print the system time in hours, minutes, secs, millisecs
Useful for profiling
*/
void printTime();