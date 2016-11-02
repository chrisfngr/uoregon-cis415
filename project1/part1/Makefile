CFLAGS=-W -Wall

all: thv1

clean:
	rm -f *.o thv1

thv1: thv1.o p1fxns.o
	cc -o thv1 thv1.o p1fxns.o


thv1.o: thv1.c p1fxns.o

p1fxns.o: p1fxns.h p1fxns.c
