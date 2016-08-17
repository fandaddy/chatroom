all:server client

server:make_socket_bind.c server.c
	gcc make_socket_bind.c server.c -o server -std=c99

client:make_socket_connect.c client.c
	gcc make_socket_connect.c client.c -o client -std=c99

clean:
	rm -rf server
	rm -rf client
