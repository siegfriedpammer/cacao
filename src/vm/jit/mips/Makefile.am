## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 860 2004-01-06 18:49:09Z twisti $


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

%.o: %.S
	$(CC) -E asmpart.S > asmpart.s
	$(AS) -64 -KPIC -O -o asmpart.o asmpart.s

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
