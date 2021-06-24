#
#
#	Makefile for test programs
#
#


all: monitor simulador

monitor: monitor.o util.o fileFunctions.o
	gcc -o monitor monitor.o util.o fileFunctions.o

simulador: simulador.o util.o fileFunctions.o
	gcc -o simulador simulador.o util.o fileFunctions.o -lpthread

clean:
	rm -f *.o *~ monitor simulador


