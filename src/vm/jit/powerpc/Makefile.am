## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1073 2004-05-19 23:21:15Z stefan $


INCLUDES = -I$(top_srcdir)/jit

EXTRA_DIST = \
	asmpart.S \
	codegen.c \
	codegen.h \
	dis-asm.h \
	disass.c \
	disass.h \
	native-math.h \
	machine-instr.h \
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
