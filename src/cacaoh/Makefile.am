## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1622 2004-11-30 13:18:52Z twisti $

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@

if USE_THREADS
if NATIVE_THREADS
THREAD_LIB = $(top_srcdir)/src/threads/native/libthreads.a
else
THREAD_LIB = $(top_srcdir)/src/threads/green/libthreads.a
endif
else
THREAD_LIB =
endif

bin_PROGRAMS = cacaoh

cacaoh_SOURCES = \
	cacaoh.c \
	headers.c

cacaoh_LDADD = \
	$(top_srcdir)/src/mm/libmm.a \
	$(THREAD_LIB) \
	$(top_srcdir)/src/vm/libvm.a \
	$(top_srcdir)/src/toolbox/libtoolbox.a \
	$(top_srcdir)/src/boehm-gc/.libs/libgc.a \
	$(top_srcdir)/src/classpath/native/jni/java-lang/.libs/libjavalang.a

cacaoh_DEPENDENCIES = \
	$(top_srcdir)/src/mm/libmm.a \
	$(THREAD_LIB) \
	$(top_srcdir)/src/vm/libvm.a \
	$(top_srcdir)/src/toolbox/libtoolbox.a \
	$(top_srcdir)/src/boehm-gc/.libs/libgc.a \
	$(top_srcdir)/src/classpath/native/jni/java-lang/.libs/libjavalang.a


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
