## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 568 2003-11-06 14:42:16Z twisti $


INCLUDES = -I$(top_srcdir)/jit

EXTRA_DIST = \
	asmpart.S \
	codegen.c \
	codegen.h \
	dis-asm.h \
	disass.c \
	disass.h \
	native-math.h \
	ppc-dis.c \
	ppc-opc.c
	ppc.h \
	types.h

noinst_LIBRARIES = libarch.a

libarch_a_SOURCES = asmpart.S codegen.c disass.c ppc-dis.c ppc-opc.c

%.o: %.S
	$(COMPILE) -c $<


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
