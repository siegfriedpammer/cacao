
#######################################################################
#          Makefile f"ur meine programmunabh"angige Toolbox           #
#######################################################################

OBJECTS = locks.o thread.o threadio.o
CC = gcc

ifeq ($(USE_THREADS),YES)
THREAD_CFLAGS = -DUSE_THREADS -DEXTERNAL_OVERFLOW -DDONT_FREE_FIRST
else
THREAD_CFLAGS =
endif

CFLAGS = $(THREAD_CFLAGS)

threads.a: $(OBJECTS) Makefile sysdep/threads.h
	rm -f threads.a
	ar qcs threads.a $(OBJECTS)
#	ranlib threads.a

locks.o : locks.c
thread.o : thread.c
threadio.o : threadio.c

clean: 
	rm -f *.o *.a
