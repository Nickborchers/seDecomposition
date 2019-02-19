CC = gcc
CFLAGS  = -g -Wall -pedantic

all: sedecomp

sedecomp: 
	$(CC) $(CFLAGS) -o sedecomp.out sedecomp.c -lm -fopenmp

clean: 
	$(RM) sedecomp *.o *~
