CC = gcc
CFLAGS = -Wall -Wextra -O2

all: server client

server: server.c 
	$(CC) $(CFLAGS) -o server server.c -lrt

client: client.c
	$(CC) $(CFLAGS) -o client client.c -lrt

clean:
	rm -f server client
