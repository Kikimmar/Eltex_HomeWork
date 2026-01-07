CC = gcc
CFLAGS = -Wall -Wextra -O2

all: server client

server: echo_server.c 
	$(CC) $(CFLAGS) -o server echo_server.c

client: echo_client.c
	$(CC) $(CFLAGS) -o client echo_client.c

clean:
	rm -f server client