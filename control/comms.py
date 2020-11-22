import socket

SERVER_PORT = 8080

if __name__ == "__main__":
	print('Starting program...')

	# create an IPv4 TCP server socket and set it up to listen
	serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	serversocket.bind(('', SERVER_PORT))
	serversocket.listen(5)

	# listen for incoming connections and accept them
	(clientsocket, address) = serversocket.accept()

	print('Client connected')