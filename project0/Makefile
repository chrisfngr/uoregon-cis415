CFLAGS=-W -Wall

all: finddupl

clean:
	rm -f *.o finddupl

finddupl: finddupl.o mentry.o mlist.o
	cc -g -o finddupl finddupl.o mentry.o mlist.o

finddupl.o: finddupl.c mentry.h mlist.h
mentry.o: mentry.h mentry.c
mlist2.o: mlist.c mlist.h mentry.h
	
