## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1624 2004-11-30 14:49:45Z twisti $

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = \
	asmpart.S \
	disass.c \
	disass.h \
	native-math.h \
	machine-instr.h \
	codegen.c \
	codegen.h \
	types.h

noinst_LIBRARIES = libarch.a

libarch_a_SOURCES = \
	asmpart.S \
	codegen.c \
	disass.c

asmpart.o: asmpart.S $(top_srcdir)/config.h offsets.h
	$(COMPILE) -c $<

offsets.h: $(top_srcdir)/config.h $(top_srcdir)/src/vm/global.h $(top_srcdir)/src/threads/native/threads.h
	$(top_srcdir)/src/vm/jit/tools/genoffsets > offsets.h


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
