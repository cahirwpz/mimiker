CC = gcc
CPPFLAGS = -I../../include
CFLAGS = -g -O2 -Wall -Wextra -std=gnu11

TARGET = pool_alloc

all: main

main: main.o pool_alloc.o

clean:
	$(RM) main *.o *~ out.txt
