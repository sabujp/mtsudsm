all: libdsm.o libdsm.a
libdsm.o: dsm.c
	gcc -Wall -O2 -fPIC -c $<
libdsm.a: libdsm.o
	ar rc libdsm.a libdsm.o
test1: test1.c
	gcc -Wall -O2 test1.c -L. -ldsm -o test1
clean:
	rm -f *.o *.a test1
