
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

threads.a: $(OBJECTS) Makefile
	rm -f threads.a
	ar qcs threads.a $(OBJECTS)
#	ranlib threads.a

locks.o : locks.c thread.h sysdep/threads.h ../tables.h ../native.h\
           ../loader.h ../builtin.h ../asmpart.h
thread.o : thread.c thread.h sysdep/threads.h ../tables.h ../native.h\
           ../loader.h ../builtin.h ../asmpart.h
threadio.o : threadio.c thread.h sysdep/threads.h ../tables.h ../native.h\
           ../loader.h ../builtin.h ../asmpart.h

clean: 
	rm -f *.o *.a
