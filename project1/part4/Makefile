CFLAGS=-W -Wall

all: thv4

clean:
	rm -f *.o thv4

thv4: thv4.o p1fxns.o
	cc -o thv4 thv4.o p1fxns.o


thv4.o: thv4.c p1fxns.o

p1fxns.o: p1fxns.h p1fxns.c
