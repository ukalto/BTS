# @file Makefile
# @author ukalto
# @details This file is used to compile the project
# @date 12.10.2022
CFLAGS = -Wall -g -std=c99 -pedantic -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE

.PHONY: all clean tar
all: myexpand

myexpand: myexpand.o 
	gcc -o myexpand myexpand.o

myexpand.o: myexpand.c
	gcc  $(CFLAGS) -c -o myexpand.o myexpand.c

clean:
	rm -rf *.o myexpand

tar:
	tar -cvzf myexpand.tgz myexpand.c makefile