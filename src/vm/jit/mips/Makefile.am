## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 565 2003-11-03 15:55:43Z twisti $


INCLUDES = -I$(top_srcdir)/jit

EXTRA_DIST = \
	asmpart.S \
	disass.c \
	disass.h \
	native-math.h \
	codegen.c \
	codegen.h \
	types.h \
	threads.h

noinst_LIBRARIES = libarch.a

libarch_a_SOURCES = asmpart.S codegen.c disass.c

%.o: %.S
	$(COMPILE) -c $<


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
