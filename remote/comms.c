#define _WINSOCK_DEPRECATED_NO_WARNINGS // TODO - might want to fix this

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
	} else {
		printf("Connected\n");
	}

	return 0;
}