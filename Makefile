################################################################################
#                    Makefile for the JavaVM - compiler CACAO                  #
################################################################################
#
# Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst
#
# See file COPYRIGHT for information on usage and disclaimer of warranties
#
# Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
#          Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
#
# Last Change: $Id: Makefile 70 1998-11-16 16:49:18Z schani $
#
#
# ATTENTION: This version of the makefile only works with gmake.
#            This Makefile not only generates object files, but also additional
#            files needed during compilation:
#                nativetypes.hh
#                nativetables.hh
#            All object files and the *.hh can be deleted savely. They will be
#            generated automatically.
#
################################################################################

VERSION_MAJOR = 0
VERSION_MINOR = 30
VERSION_POSTFIX = p2

VERSION_STRING=$(VERSION_MAJOR).$(VERSION_MINOR)$(VERSION_POSTFIX)

##################### generation of the excutable ##############################

# Enabling/disabling thread support
USE_THREADS = YES
#USE_THREADS = NO

ifeq ($(USE_THREADS),YES)
THREAD_OBJ = threads/threads.a
THREAD_CFLAGS = -DUSE_THREADS -DEXTERNAL_OVERFLOW -DDONT_FREE_FIRST
else
THREAD_OBJ =
THREAD_CFLAGS =
endif

#CC = cc
#CFLAGS = -g -mieee -Wall $(THREAD_CFLAGS)
#CFLAGS = -mieee -O3 -Wall $(THREAD_CFLAGS)
#LFLAGS = -lm

CC = cc
CFLAGS = -g -ieee $(THREAD_CFLAGS)
#CFLAGS = -O3 -ieee $(THREAD_CFLAGS)
LFLAGS = -lm

# IRIX 6.5 MIPSPro 7.2.1
#CC = cc
#CFLAGS = -g $(THREAD_CFLAGS) -DMAP_ANONYMOUS=0 -woff 1048,1110,1164,1515
#CFLAGS = -O2 -OPT:Olimit=0 $(THREAD_CFLAGS) -DMAP_ANONYMOUS=0
#LFLAGS = -lm -lelfutil

OBJ = main.o tables.o loader.o compiler.o jit.o builtin.o asmpart.o \
	toolbox/toolbox.a native.o $(THREAD_OBJ) mm/mm.o
OBJH = headers.o tables.o loader.o builtin.o toolbox/toolbox.a $(THREAD_OBJ) \
mm/mm.o

cacao: $(OBJ)
	$(CC) $(CFLAGS) -o cacao $(OBJ) $(LFLAGS)
cacaoh: $(OBJH)
	$(CC) $(CFLAGS) -o cacaoh $(OBJH) $(LFLAGS)

main.o: main.c global.h tables.h loader.h jit.h compiler.h \
        asmpart.h builtin.h native.h

headers.o:  headers.c global.h tables.h loader.h

loader.o:   loader.c global.h loader.h tables.h native.h asmpart.h

compiler.o: builtin.h compiler.h global.h loader.h tables.h native.h \
            asmpart.h compiler.c comp/*.c sysdep/gen.c sysdep/disass.c

jit.o:  builtin.h jit.h global.h loader.h tables.h native.h asmpart.h \
            jit/jitdef.h jit/*.c sysdep/ngen.h sysdep/ngen.c sysdep/disass.c

builtin.o: builtin.c global.h loader.h builtin.h tables.h sysdep/native-math.h

native.o: native.c global.h tables.h native.h asmpart.h builtin.h \
          nativetypes.hh nativetable.hh nat/*.c

tables.o: tables.c global.h tables.h

global.h: sysdep/types.h toolbox/*.h
	touch global.h

toolbox/toolbox.a: toolbox/*.c toolbox/*.h
	cd toolbox; $(MAKE) toolbox.a "CFLAGS=$(CFLAGS)" "CC=$(CC)" 

ifeq ($(USE_THREADS),YES)
threads/threads.a: threads/*.c threads/*.h sysdep/threads.h
	cd threads; $(MAKE) threads.a "USE_THREADS=$(USE_THREADS)" "CFLAGS=$(CFLAGS)" "CC=$(CC)" 
endif

mm/mm.o: mm/*.[ch] mm/Makefile
	cd mm; $(MAKE) mm.o "USE_THREADS=$(USE_THREADS)" "CFLAGS=$(CFLAGS)" "CC=$(CC)"

asmpart.o: sysdep/asmpart.c sysdep/offsets.h
	rm -f asmpart.s
	$(CC) -E sysdep/asmpart.c > asmpart.s
	$(CC) -c -o asmpart.o asmpart.s
	rm -f asmpart.s


########################### support targets ####################################

clean:
	rm -f *.o cacao cacaoh cacao.tgz nativetable.hh nativetypes.hh \
	      core tst/core
	cd toolbox; $(MAKE) clean
	cd threads; $(MAKE) clean
	cd mm; $(MAKE) clean


### DISTRIBUTION TARGETS ###

DISTRIBUTION_FILES = \
Makefile \
*/Makefile \
README \
COPYRIGHT \
tst/*.java \
doc/*.doc \
html/*.html \
*.[ch] \
comp/*.[ch] \
jit/*.[ch] \
alpha/*.doc \
alpha/*.[ch] \
nat/*.[ch] \
toolbox/*.[ch] \
threads/*.[ch] \
mm/*.[ch] \
# sparc/*.[ch]

tar:
	rm -f cacao.tgz cacao.tar
	tar -cvf cacao.tar $(DISTRIBUTION_FILES)
	ls -l cacao.tar
	gzip -9 cacao.tar
	mv cacao.tar.gz cacao.tgz
	ls -l cacao.tgz

dist:
	rm -rf cacao-$(VERSION_STRING).tar.gz cacao-$(VERSION_STRING);
	( mkdir cacao-$(VERSION_STRING); \
	  tar -cvf cacao-$(VERSION_STRING).tar $(DISTRIBUTION_FILES); \
	  cd cacao-$(VERSION_STRING); \
	  tar -xf ../cacao-$(VERSION_STRING).tar; \
	  cd ..; \
	  rm cacao-$(VERSION_STRING).tar; \
	  tar -cvf cacao-$(VERSION_STRING).tar cacao-$(VERSION_STRING); \
	  rm -rf cacao-$(VERSION_STRING); )
	gzip -9 cacao-$(VERSION_STRING).tar
	ls -l cacao-$(VERSION_STRING).tar.gz

########################## supported architectures #############################

config-alpha:
	rm -f sysdep
	ln -s alpha sysdep
	rm -f threads/sysdep
	ln -s ../sysdep threads/sysdep
	$(MAKE) clean

config-sparc:
	rm -f sysdep
	ln -s sparc sysdep
	rm -f threads/sysdep
	ln -s ../sysdep threads/sysdep
	$(MAKE) clean

config-mips:
	rm -f sysdep
	ln -s mips sysdep
	rm -f threads/sysdep
	ln -s ../sysdep threads/sysdep
	$(MAKE) clean


##################### generation of NATIVE - header files ######################

sysdep/offsets.h nativetypes.hh nativetable.hh : cacaoh
	./cacaoh java.lang.Object \
       java.lang.String \
       java.lang.Class \
       java.lang.ClassLoader \
       java.lang.Compiler \
       java.lang.Double \
       java.lang.Float \
       java.lang.Math \
       java.lang.Runtime \
       java.lang.SecurityManager \
       java.lang.System \
       java.lang.Thread \
       java.lang.ThreadGroup \
       java.lang.Throwable \
\
       java.io.File \
       java.io.FileDescriptor \
       java.io.FileInputStream \
       java.io.FileOutputStream \
       java.io.PrintStream \
       java.io.RandomAccessFile \
\
       java.util.Properties \
       java.util.Date
