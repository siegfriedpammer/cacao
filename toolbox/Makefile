################################################################################
#                 Makefile for the program independent toolbox                 #
################################################################################
#
# Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst
#
# See the file COPYRIGHT for information on usage and disclaimer of warranties
#
# Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
#
# Last Change: 1997/10/30
#
################################################################################

OBJECTS = memory.o loging.o chain.o tree.o list.o

toolbox.a: $(OBJECTS) Makefile
	rm -f toolbox.a
	ar qcs toolbox.a $(OBJECTS)
	
memory.o: memory.c memory.h loging.h
loging.o: loging.c loging.h
chain.o: chain.c chain.h memory.h
tree.o: tree.c tree.h memory.h
list.o: list.c list.h

clean: 
	rm -f *.o *.a
