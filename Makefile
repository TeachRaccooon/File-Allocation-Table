# Generic makefile

all: fatrw

clean:
	rm -f fatrw *.o

LIBS = $(HOME)/$(LIB)/libfdr.a
INCLUDE = $(HOME)/include
CC = gcc

.SUFFIXES: .cpp .c .o .out .hist .jgr .jps .eps .nt .bib .tab .tex .dvi .fig .txt .ps .pdf .bin .od .odh .odd .ppm .gif

.cpp.o: 
	g++ -c -I$(INCLUDE) $*.cpp

.c.o: 
	g++ -c -I$(INCLUDE) $*.c

jdisk.o: jdisk.h
fatrw.o: jdisk.h

fatrw: fatrw.o jdisk.o
	g++ -o fatrw fatrw.o jdisk.o
