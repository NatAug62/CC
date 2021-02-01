"""
This file will contain all the constants used in the program
"""

# port number for initial connection and sending commands
# this socket will also be used for mouse & keyboard control
SERVER_PORT = 8080
# port number for receiving video stream from victim computer
VIDEO_PORT = 8081

# change directory and list directory contents
CHANGE_DIR = 1
LIST_DIR = 2
# upload and download files from/to attacker
UPLOAD = 3
DOWNLOAD = 4
# run an arbitrary comamnd
RUN_CMD = 5
# start and end video, mouse, audio, and keyboard streams
START_VIDEO = 6
END_VIDEO = 7
START_MOUSE = 8
END_MOUSE = 9
START_AUDIO = 10
END_AUDIO = 11
START_KEYS = 13
END_KEYS = 14
# end the process (on the victim's computer)
KILL_PROC = 12
# print the rest of the message to provide useful info
PRINT_INFO = 15
# inform the attacker what the current directory is
NEW_CURR_DIR = 16