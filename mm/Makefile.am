## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1225 2004-06-30 19:26:19Z twisti $

SUBDIRS = boehm-gc

noinst_LIBRARIES = libboehm.a

noinst_HEADERS = boehm.h

libboehm_a_SOURCES = boehm.c

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/jit/@ARCH_DIR@


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
