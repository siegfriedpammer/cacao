## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 618 2003-11-13 09:39:28Z stefan $

INCLUDES = -I$(top_srcdir)/jit/@ARCH_DIR@ -I$(top_srcdir)/jit

EXTRA_DIST = \
	graph.c \
	loop.c \
	loop.h \
	analyze.c \
	tracing.c

noinst_HEADERS = \
	graph.h \
	analyze.h \
	tracing.h

noinst_LIBRARIES = libloop.a

libloop_a_SOURCES = loop.c analyze.c graph.c tracing.c


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
