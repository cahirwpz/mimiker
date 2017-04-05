CC = gcc
CFLAGS = -g -O2 -Wall -Wextra -std=gnu11

TARGET = pool_alloc

all: main

main: main.c pool_alloc.c
	$(CC) $(CFLAGS) -o main main.c pool_alloc.c

clean:
	$(RM) main *.o *~