CC = gcc
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)

OBJECTS = myexpand.o

.PHONY: all clean
all: myexpand

myexpand: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

myexpand.o: myexpand.c

tar:
	tar -cvzf myexpand.tgz myexpand.c makefile

clean:
	rm -rf *.o myexpand