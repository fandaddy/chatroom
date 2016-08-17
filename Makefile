server:make_socket_bind.c server.c
	gcc make_socket_bind.c server.c -o server -std=c99
