/*
Header file for the utils.c source file
*/

#pragma once

// include the catch-all header
#include "general.h"

/*
Connect to a TCP server on a given port at the given IP address
Expects an IP address, port number, and socket as arguments
Returns 0 on success or the associated error code on failure
*/
int connectToTCP(char* addr, int port, SOCKET sock);

/*
Send a string over a given socket
Expects a socket and a string as arguments
Returns 0 unless a socket error occurs, in which case the error code is returned instead
*/
int sendString(char* data, SOCKET sock);