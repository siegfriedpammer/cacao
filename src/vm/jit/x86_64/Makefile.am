## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1621 2004-11-30 13:06:55Z twisti $


INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = \
	asmpart.S \
	bfd.h \
	codegen.c \
	codegen.h \
	dis-asm.h \
	dis-buf.c \
	disass.c \
	disass.h \
	emitfuncs.c \
	emitfuncs.h \
	i386-dis.c \
	types.h

noinst_LIBRARIES = libarch.a

libarch_a_SOURCES = \
	asmpart.S \
	codegen.c \
	dis-buf.c \
	disass.c \
	emitfuncs.c \
	i386-dis.c

asmpart.o: asmpart.S $(top_srcdir)/config.h offsets.h
	$(COMPILE) -c $<

offsets.h: $(top_srcdir)/config.h $(top_srcdir)/src/vm/global.h $(top_srcdir)/src/threads/native/threads.h
	$(top_srcdir)/src/vm/jit/tools/genoffsets > offsets.h


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
