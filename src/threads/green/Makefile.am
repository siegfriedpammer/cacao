## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 662 2003-11-21 18:06:25Z jowenn $


noinst_LIBRARIES = @LIBTHREAD@
EXTRA_LIBRARIES = libthreads.a

libthreads_a_SOURCES = \
	locks.c \
	locks.h \
	thread.c \
	thread.h \
	threadio.c \
	threadio.h

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/jit -I$(top_srcdir)/jit/@ARCH_DIR@


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
