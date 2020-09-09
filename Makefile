CC = gcc
CFLAGS = -Wall -g

all: server client

server: server.c 
	$(CC) $(CFLAGS) -o server server.c -lpthread

client: client.c 
	$(CC) $(CFLAGS) -o client client.c -lpthread

.PHONY: clean
clean:
	rm -f client server
