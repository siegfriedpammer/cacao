## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1266 2004-07-01 20:38:16Z twisti $


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
	native-math.h \
	types.h

noinst_LIBRARIES = libarch.a

libarch_a_SOURCES = asmpart.S codegen.c disass.c emitfuncs.c i386-dis.c dis-buf.c

%.o: %.S
	$(COMPILE) -c $<


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
