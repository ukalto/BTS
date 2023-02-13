# @file Makefile
# @author Maximilian Gaber 52009273
# @date 08.12.2022
# @brief The makefile 
# 

CC = gcc -g
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
MATHFLAGS = -lm

.PHONY: all clean
all: forkFFT

forkFFT: forkFFT.o
	$(CC) -o $@ $^ $(MATHFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf *.o forkFFT

tar:
	tar -cvzf forkFFT.tgz forkFFT.c makefile
