CC = gcc
CFLAGS = -Wall -Wextra -O2

all: reciever sender

reciever: reciever.c 
	$(CC) $(CFLAGS) -o reciever reciever.c

sender: sender.c
	$(CC) $(CFLAGS) -o sender sender.c

clean:
	rm -f reciever sender
