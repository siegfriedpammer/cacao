## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1635 2004-12-01 09:57:58Z twisti $

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = \
	asmpart.S \
	codegen.c \
	codegen.h \
	dis-asm.h \
	disass.c \
	disass.h \
	machine-instr.h \
	ppc-dis.c \
	ppc-opc.c
	ppc.h \
	types.h

noinst_LIBRARIES = libarch.a

libarch_a_SOURCES = \
	asmpart.S \
	codegen.c \
	disass.c \
	ppc-dis.c \
	ppc-opc.c

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
