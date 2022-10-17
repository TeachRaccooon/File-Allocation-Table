# Generic makefile

all: FATRW

clean:
	rm -f FATRW *.o

LIBS = $(HOME)/$(LIB)/libfdr.a
INCLUDE = $(HOME)/include
CC = gcc

.SUFFIXES: .cpp .c .o .out .hist .jgr .jps .eps .nt .bib .tab .tex .dvi .fig .txt .ps .pdf .bin .od .odh .odd .ppm .gif

.cpp.o: 
	g++ -c -I$(INCLUDE) $*.cpp

.c.o: 
	g++ -c -I$(INCLUDE) $*.c

jdisk.o: jdisk.h
FATRW.o: jdisk.h

FATRW: FATRW.o jdisk.o
	g++ -o FATRW FATRW.o jdisk.o
