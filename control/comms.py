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

	# echo messages back to the sender
	while True:
		data = clientsocket.recv(4096)
		if not data:
			break
		print(data)
		data_s = b'ECHO: ' + data + b'\0'
		print(data_s)
		clientsocket.sendall(data_s)