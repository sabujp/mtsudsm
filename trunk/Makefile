INCLUDE=-I/home/sabuj/mpich2i/include
LDFLAGS=-L/home/sabuj/mpich2i/lib -L.
LIBS=-ldsm -lmpich -pthread

all: libdsm.o libdsm.a
libdsm.o: dsm.c
	gcc -Wall -fPIC -c $< ${INCLUDE} -o libdsm.o
libdsm.a: libdsm.o
	ar rcs libdsm.a $<
test1: test1.c
	gcc -Wall $< ${INCLUDE} ${LDFLAGS} ${LIBS} -o test1
clean:
	rm -f *.o *.a test1
