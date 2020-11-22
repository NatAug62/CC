#define _WINSOCK_DEPRECATED_NO_WARNINGS // TODO - work around this

#include<stdio.h> // standard input/output
#include<winsock2.h> // windows socket header

#pragma comment(lib,"ws2_32.lib") // tell the linker to link the 

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 8080

int main(int argc, char* argv[]) {
	WSADATA wsa;
	SOCKET sock;
	struct sockaddr_in server;
	int retCode;
	char buffer[4096];

	// initialize winsock
	retCode = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (retCode != 0) {
		printf("WSAStartup failed with error: %d\n", retCode);
		return 1;
	}

	// create a socket
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		printf("Could not create socket: %d\n", WSAGetLastError());
	}

	// set the server info
	server.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);

	// connect to the server
	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
		printf("Could not connect! Error code: %d\n", WSAGetLastError());
	}

	printf("Connected\n");

	// get input and send it, then wait for a response
	while (1 == 1) {
		if (fgets(buffer, 4096, stdin) == NULL) {
			printf("Error when reading user input! Exiting program...\n");
			return 1;
		}
		retCode = send(sock, buffer, (int) strlen(buffer), 0);
		if (retCode == SOCKET_ERROR) {
			printf("Send failed with error: %d\n", WSAGetLastError());
			return 1;
		}
		retCode = recv(sock, buffer, 4096, 0);
		if (retCode > 0) {
			printf(buffer);
		}
		else if (retCode == 0) {
			printf("Connection closed...\n");
			return 0;
		}
		else {
			printf("Recv failed with error: %d\n", WSAGetLastError());
		}
	}

	return 0;
}