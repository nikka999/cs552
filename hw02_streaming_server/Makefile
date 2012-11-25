all: client server

client: client.c
	gcc -g -m32 -pthread -I ./support/include -L ./support/lib/netpbm client.c -lGLU -lGL -lnetpbm -lX11 -lm -o client

server: server.c
	gcc -g -m32 -pthread -I ./support/include -L ./support/lib/netpbm server.c -lGLU -lGL -lnetpbm -lX11 -lm -o server

clean:
	rm client server
