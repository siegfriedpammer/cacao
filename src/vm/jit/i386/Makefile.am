## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1142 2004-06-05 23:20:53Z twisti $


INCLUDES = -I$(top_srcdir)

EXTRA_DIST = \
	asmpart.S \
	bfd.h \
	codegen.c \
	codegen.h \
	dis-asm.h \
	disass.c \
	disass.h \
	emitfuncs.c \
	emitfuncs.h \
	machine-instr.h \
	native-math.h \
	types.h

noinst_LIBRARIES = libarch.a

libarch_a_SOURCES = asmpart.S codegen.c disass.c emitfuncs.c i386-dis.c dis-buf.c

asmpart.o: asmpart.S offsets.h
	$(COMPILE) -c $<


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
