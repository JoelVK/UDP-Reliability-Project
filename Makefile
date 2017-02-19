all: server client

server: udpserver.c
	gcc -Wall -g udpserver.c -o server

client: udpclient.c
	gcc -Wall -g udpclient.c -lm -o client

runclient: client
	./client 5555 127.0.0.1

runserver: server
	./server 5555
