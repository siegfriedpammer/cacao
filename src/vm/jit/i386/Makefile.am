## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1350 2004-07-22 22:38:12Z twisti $


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
	machine-instr.h \
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

#offsets.h: $(top_srcdir)/genoffsets $(top_srcdir)/config.h
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
