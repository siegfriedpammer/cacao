## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1622 2004-11-30 13:18:52Z twisti $

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@

SUBDIRS = jit

if USE_ZLIB
ZLIB_OBJ = unzip.c
endif

noinst_LIBRARIES = libvm.a

libvm_a_SOURCES = \
	builtin.c \
	exceptions.c \
	loader.c \
	options.c \
	statistics.c \
	tables.c \
	$(ZLIB_OBJ)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
