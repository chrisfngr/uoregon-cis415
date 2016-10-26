CFLAGS=-W -Wall

all: thv3

clean:
	rm -f *.o thv3

thv3: thv3.o p1fxns.o
	cc -o thv3 thv3.o p1fxns.o


thv3.o: thv3.c p1fxns.o

p1fxns.o: p1fxns.h p1fxns.c
