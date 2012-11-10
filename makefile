CC = gcc

all: simulator list.o
simulator: list.o
list.o: list.h

clean:
	rm -f simulator list.o
