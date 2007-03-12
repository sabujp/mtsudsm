#PREFIX=/home/sabuj/mpich2i
PREFIX=/home/rbutler/public/courses/osd/mpich2i

INCLUDE=-I${PREFIX}/include
LDFLAGS=-L${PREFIX}/lib -L.
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
