## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 590 2003-11-09 19:44:36Z twisti $


INCLUDES = -I$(top_srcdir)/jit

EXTRA_DIST = \
	asmpart.S \
	bfd.h \
	codegen.c \
	codegen.h \
	dis-asm.h \
	disass.c \
	disass.h \
	methodtable.c \
	methodtable.h \
	native-math.h \
	types.h

noinst_LIBRARIES = libarch.a

libarch_a_SOURCES = asmpart.S codegen.c disass.c i386-dis.c dis-buf.c methodtable.c

%.o: %.S
	$(COMPILE) -c $<


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
