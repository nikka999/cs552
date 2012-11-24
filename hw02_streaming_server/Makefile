all: client server

client: client.c
	gcc -g -m32 client.c -o client

server: server.c
	gcc -g -m32 -pthread server.c -o server

clean:
	rm client server
