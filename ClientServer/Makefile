CC = gcc -g
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g

all: client server

client: client.o
		$(CC) -o client client.o

client.o: client.c
		$(CC) $(CFLAGS) -c -o client.o client.c
		
server: server.o
	$(CC) -o server server.o

server.o: server.c
	$(CC) $(CFLAGS) -c -o server.o server.c

clean:
		rm -rf *.o server client 3.tgz

tar:
		tar -cvzf 3.tgz client.c server.c Makefile
