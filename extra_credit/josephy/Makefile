CFLAGS=-W -Wall -g 

file_crawler: file_crawler.o libADTS.a
	gcc $(CFLAGS) -o file_crawler file_crawler.o libADTS.a -lpthread

libADTS.a: iterator.o linkedlist.o treeset.o re.o
	rm -f libADTS.a
	ar r libADTS.a iterator.o linkedlist.o treeset.o re.o
	ranlib libADTS.a

clean:
	rm -f *.o file_crawler

iterator.o: iterator.c iterator.h
linkedlist.o: linkedlist.c linkedlist.h iterator.h
treeset.o: treeset.c treeset.h iterator.h
re.o: re.c re.h
file_crawler.o: file_crawler.c iterator.h linkedlist.h treeset.h re.h
