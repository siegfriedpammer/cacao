## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1117 2004-06-02 10:56:49Z twisti $


INCLUDES = -I$(top_srcdir)/jit

EXTRA_DIST = \
	asmpart.S \
	disass.c \
	disass.h \
	native-math.h \
	codegen.c \
	codegen.h \
	types.h

noinst_LIBRARIES = libarch.a

libarch_a_SOURCES = asmpart.S codegen.c disass.c

asmpart.o: asmpart.S offsets.h
	$(COMPILE) -E asmpart.S > asmpart.s
	$(AS) -64 -KPIC -O -o asmpart.o asmpart.s

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
