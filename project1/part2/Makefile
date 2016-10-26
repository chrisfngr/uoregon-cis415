CFLAGS=-W -Wall

all: thv2

clean:
	rm -f *.o thv2

thv2: thv2.o p1fxns.o
	cc -o thv2 thv2.o p1fxns.o


thv2.o: thv2.c p1fxns.o

p1fxns.o: p1fxns.h p1fxns.c
