## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1638 2004-12-01 10:42:28Z twisti $

INCLUDES = -I$(top_srcdir)/src

EXTRA_DIST = \
	asmpart.S \
	disass.c \
	disass.h \
	codegen.c \
	codegen.h \
	types.h

noinst_LIBRARIES = libarch.a

libarch_a_SOURCES = asmpart.S codegen.c disass.c

asmpart.o: asmpart.S offsets.h
	$(COMPILE) -E asmpart.S > asmpart.s
	$(AS) -64 -KPIC -O -o asmpart.o asmpart.s

offsets.h: $(top_srcdir)/config.h $(top_srcdir)/src/vm/global.h $(top_srcdir)/src/threads/native/threads.h
	$(top_srcdir)/src/vm/jit/tools/genoffsets > offsets.h


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
