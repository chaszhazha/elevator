CC = gcc 
all: simulator list.o
simulator: list.o
	$(CC) -o $@ simulator.c $^ -lm
list.o: list.h

clean:
	rm -f simulator list.o
