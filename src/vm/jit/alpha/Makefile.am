## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1276 2004-07-05 17:27:52Z twisti $


INCLUDES = -I$(top_srcdir)

EXTRA_DIST = \
	asmpart.S \
	disass.c \
	disass.h \
	native-math.h \
	machine-instr.h \
	codegen.c \
	codegen.h \
	types.h \
	sigcontext.h

noinst_LIBRARIES = libarch.a

libarch_a_SOURCES = asmpart.S codegen.c disass.c

asmpart.o: asmpart.S $(top_srcdir)/config.h offsets.h
	$(COMPILE) -c $<

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
