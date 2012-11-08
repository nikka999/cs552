all: client server

client: client.c
	gcc -m32 client.c -o client

server: server.c
	gcc -m32 server.c -o server

clean:
	rm client server
