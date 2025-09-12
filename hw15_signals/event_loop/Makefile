CC = gcc
CFLAGS = -Wall -Wextra -O2

all: event sender

event: event.c 
	$(CC) $(CFLAGS) -o event event.c

sender: sender.c
	$(CC) $(CFLAGS) -o sender sender.c

clean:
	rm -f event sender
