## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 557 2003-11-02 22:51:59Z twisti $


INCLUDES = -I$(top_srcdir)/jit

EXTRA_DIST = \
	asmpart.S \
	disass.c \
	disass.h \
	native-math.h \
	codegen.c \
	codegen.h \
	types.h \
	threads.h \
	sigcontext.h

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
