## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1289 2004-07-09 11:41:47Z twisti $


INCLUDES = -I$(top_srcdir)

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

offsets.h: $(top_srcdir)/config.h $(top_srcdir)/global.h $(top_srcdir)/threads/thread.h
	$(COMPILE) -o $(top_srcdir)/genoffsets $(top_srcdir)/genoffsets.c
	$(top_srcdir)/genoffsets > offsets.h


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
