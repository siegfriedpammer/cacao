## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 557 2003-11-02 22:51:59Z twisti $

SUBDIRS = boehm-gc

noinst_LIBRARIES = libboehm.a

libboehm_a_SOURCES = boehm.c

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/jit -I$(top_srcdir)/jit/@ARCH_DIR@ -I./boehm-gc/include


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
