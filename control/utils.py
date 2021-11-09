"""
This file contains utility/helper functions used across multiple modules/files
"""

import socket

"""
	Create a server socket and listen for an incoming connection
	Returns the socket associated with the connection
"""
def openConnection(portNum):
	print(f"Listening for connection on port {portNum} ...")

	# create an IPv4 TCP server socket and set it up to listen
	serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # TODO - what is making us need this???
	serversocket.bind(('0.0.0.0', portNum))
	serversocket.listen()

	# listen for incoming connections and accept them
	clientsocket, address = serversocket.accept()
	print("Connection established")

	# close the server socket and return the socket associated with the connection
	serversocket.close()
	return clientsocket