## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1621 2004-11-30 13:06:55Z twisti $

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@

SUBDIRS = inline loop verify tools @ARCH_DIR@

DIST_SUBDIRS = inline loop verify tools alpha i386 mips powerpc x86_64

noinst_LIBRARIES = libjit.a

libjit_a_SOURCES = \
	jit.c \
	parse.c \
	stack.c

libjit_a_LIBADD = \
	inline/libinline.a \
	loop/libloop.a \
	verify/libverify.a


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
