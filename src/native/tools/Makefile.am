## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1649 2004-12-02 09:33:38Z twisti $

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@

if USE_THREADS
if NATIVE_THREADS
THREAD_LIBRARY = $(top_srcdir)/src/threads/native/libthreads.a
else
THREAD_LIBRARY = $(top_srcdir)/src/threads/green/libthreads.a
endif
else
THREAD_LIBRARY =
endif

noinst_PROGRAMS = gennativetable

gennativetable_SOURCES = gennativetable.c

gennativetable_LDADD = \
	$(top_srcdir)/src/cacaoh/headers.o \
	$(THREAD_LIBRARY) \
	$(top_srcdir)/src/vm/libvm.a \
	$(top_srcdir)/src/mm/libmm.a \
	$(top_srcdir)/src/toolbox/libtoolbox.a \
	$(top_srcdir)/src/boehm-gc/.libs/libgc.a \
	$(top_srcdir)/src/classpath/native/jni/java-lang/.libs/libjavalang.a

gennativetable_DEPENDENCIES = \
	$(top_srcdir)/src/cacaoh/headers.o \
	$(THREAD_LIBRARY) \
	$(top_srcdir)/src/vm/libvm.a \
	$(top_srcdir)/src/mm/libmm.a \
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
