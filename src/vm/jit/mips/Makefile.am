## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 566 2003-11-03 19:06:50Z twisti $


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

libarch_a_SOURCES = asmpart.s codegen.c disass.c

#%.o: %.S
#	$(COMPILE) -c $<

%.o: %.S
	$(CC) -E asmpart.S > asmpart.s
	$(COMPILE) -c asmpart.s

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
