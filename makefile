CC = gcc 
PFLAG = -pthread
all: simulator list.o
simulator: list.o
	$(CC) $(PFLAG) -o $@ simulator.c $^ -lm
list.o: list.h

clean:
	rm -f simulator list.o
