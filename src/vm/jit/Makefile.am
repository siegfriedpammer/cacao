## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1251 2004-06-30 20:24:06Z twisti $


SUBDIRS = . loop @ARCH_DIR@
DIST_SUBDIRS = loop alpha i386 mips powerpc x86_64

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/jit/@ARCH_DIR@

EXTRA_DIST = \
	jit.c \
	jit.h \
	codegen.inc \
	parse.c \
	reg.inc \
	stack.c \
	inline.c \
	sets.c \
	sets.h \
	parseRT.c \
	parseRTprint.h \
	parseRTstats.c \
	typecheck.c

noinst_HEADERS = \
	parse.h \
	stack.h \
	reg.h \
	inline.h \
	parseRT.h \
	parseRTstats.h

noinst_LIBRARIES = libjit.a

libjit_a_SOURCES = jit.c parse.c stack.c inline.c sets.c parseRT.c parseRTstats.c typecheck.c


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
