#  @file Makefile
#  @author
#  @brief This file is used to compile the project
#  @version 0.1
#  @date 12.11.2022
#  
#  @copyright Copyright (c) 2022
#

CC = gcc
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
LDFLAGS = -pthread -lrt

TARGET_COMPILE = $(CC) -o $@ $^ $(LDFLAGS)
OBJECT_COMPILE = $(CC) $(CFLAGS) -c -o $@ $<

.PHONY: all clean tar
all: supervisor generator

generator: generator.o
	$(TARGET_COMPILE)

supervisor: supervisor.o
	$(TARGET_COMPILE)

%.o: %.c
	$(OBJECT_COMPILE)

clean:
	rm -rf *.o supervisor generator 3color.tgz

tar:
	tar -cvzf 3color.tgz generator.c supervisor.c semaphore.c sharedmemory.c 3color.h makefile