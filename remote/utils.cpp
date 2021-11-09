/*
General utility functions
*/

#include "utils.h"

/*
Connect to a TCP server on a given port at the given IP address
Expects an IP address, port number, and socket as arguments
The provided socket will be set to a connected socket on success and INVALID_SOCKET on failure
Returns 0 on success or the associated error code on failure
If an error occurs, an error message will also be printed
*/
int connectToTCP(const char* addr, int port, SOCKET *sock) {
	struct sockaddr_in server;
	int error;

	// create the TCP socket
	*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sock == INVALID_SOCKET) { // return error code if failed
		error = WSAGetLastError();
		printf("Could not create socket: %d\n", error);
		return error;
	}

	// build the server info structure
	server.sin_addr.s_addr = inet_addr(addr);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	// try connecting to the TPC server
	if (connect(*sock, (struct sockaddr*) & server, sizeof(server)) == SOCKET_ERROR) {
		error = WSAGetLastError();
		printf("Could not conenct to %s on port %d: %d\n", addr, port, error);
		// try closing the socket if we couldn't connect
		if(closesocket(*sock) == SOCKET_ERROR) {
			error = WSAGetLastError();
			printf("Could not close socket: %d\n", error);
		}
		// return the last error code we got
		return error;
	}

	// if we got this far, we've connected successfully
	return 0;
}

/*
Send a string over a given socket
Expects a socket and a string as arguments
Returns 0 unless a socket error occurs, in which case the error code is returned instead
*/
int sendString(const char* data, SOCKET *sock) {
	int bytesToSend = strlen(data);
	int bytesSent = 0;
	int retCode;

	// loop until all data is sent
	do {
		// try sending the remaining data
		retCode = send(*sock, (char*)(data + bytesSent), bytesToSend - bytesSent, 0);
		if (retCode == SOCKET_ERROR) { // return error code on failure
			retCode = WSAGetLastError();
			printf("Send failed with error: %d\n", retCode);
			return retCode;
		}
		// update the number of bytes sent
		bytesSent += retCode;
		// debug info if need to loop again
		if (bytesSent < bytesToSend) {
			printf("Could not send all data in one attempt! Looping to send remaining data...\n"); }
	} while (bytesSent < bytesToSend);
	// if we got this far, the data was sent successfully
	printf("Message has been sent!\n");
	printf("Message is as follows:\n%s\n", data);
	return 0;
}

/*
Shift the contents of a char array left
Expects a char array, array length, and shift amount as arguments
Returns nothing
*/
void shiftStringLeft(char* buffer, int len, int shift) {
	for (int i = shift; i < len; i++) {
		buffer[i-shift] = buffer[i]; }
}

/*
Print the system time in hours, minutes, secs, millisecs
Useful for profiling
*/
void printTime() {
	SYSTEMTIME st;
	GetSystemTime(&st);
	printf("%i:%i:%i.%i\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}