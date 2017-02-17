all: server client

server: udpserver.c
	gcc -Wall -g udpserver.c -o server

client: udpclient.c
	gcc -Wall -g udpclient.c -o client


