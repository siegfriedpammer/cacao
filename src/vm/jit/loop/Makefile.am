## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 557 2003-11-02 22:51:59Z twisti $

INCLUDES = -I$(top_srcdir)/jit/@ARCH_DIR@ -I$(top_srcdir)/jit

EXTRA_DIST = \
	graph.c \
	loop.c \
	loop.h \
	analyze.c \
	tracing.c

noinst_LIBRARIES = libloop.a

libloop_a_SOURCES = loop.c analyze.c graph.c tracing.c


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
