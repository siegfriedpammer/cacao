## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1124 2004-06-03 20:47:05Z twisti $


INCLUDES = -I$(top_srcdir)

EXTRA_DIST = \
	asmpart.S \
	bfd.h \
	codegen.c \
	codegen.h \
	dis-asm.h \
	disass.c \
	disass.h \
	machine-instr.h \
	native-math.h \
	types.h

noinst_LIBRARIES = libarch.a

libarch_a_SOURCES = asmpart.S codegen.c disass.c i386-dis.c dis-buf.c

asmpart.o: asmpart.S offsets.h
	$(COMPILE) -c asmpart.S


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
