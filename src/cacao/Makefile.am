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

if STATIC_CLASSPATH
CLASSPATH_LIBS = \
	$(top_srcdir)/src/classpath/native/jni/java-io/.libs/libjavaio.a \
	$(top_srcdir)/src/classpath/native/jni/java-nio/.libs/libjavanio.a \
	$(top_srcdir)/src/classpath/native/jni/java-net/.libs/libjavanet.a \
	$(top_srcdir)/src/classpath/native/jni/java-util/.libs/libjavautil.a \
	$(top_srcdir)/src/classpath/native/jni/java-lang/.libs/libjavalang.a \
	$(top_srcdir)/src/classpath/native/jni/java-lang/.libs/libjavalangreflect.a
else
cacao_LDFLAGS = -ldl -Wl,--export-dynamic
CLASSPATH_LIBS =
endif

bin_PROGRAMS = cacao

cacao_SOURCES = cacao.c

cacao_LDADD = \
	$(top_srcdir)/src/mm/libmm.a \
	$(THREAD_LIB) \
	$(top_srcdir)/src/vm/libvm.a \
	$(top_srcdir)/src/vm/jit/libjit.a \
	$(top_srcdir)/src/vm/jit/inline/libinline.a \
	$(top_srcdir)/src/vm/jit/loop/libloop.a \
	$(top_srcdir)/src/vm/jit/verify/libverify.a \
	$(top_srcdir)/src/vm/jit/@ARCH_DIR@/libarch.a \
	$(top_srcdir)/src/native/libnative.a \
	$(top_srcdir)/src/native/vm/libnativevm.a \
	$(top_srcdir)/src/toolbox/libtoolbox.a \
	$(top_srcdir)/src/boehm-gc/.libs/libgc.a \
	$(CLASSPATH_LIBS) \
	@AWT_OBJS@

cacao_DEPENDENCIES = \
	$(top_srcdir)/src/mm/libmm.a \
	$(THREAD_LIB) \
	$(top_srcdir)/src/vm/libvm.a \
	$(top_srcdir)/src/vm/jit/libjit.a \
	$(top_srcdir)/src/vm/jit/inline/libinline.a \
	$(top_srcdir)/src/vm/jit/loop/libloop.a \
	$(top_srcdir)/src/vm/jit/verify/libverify.a \
	$(top_srcdir)/src/vm/jit/@ARCH_DIR@/libarch.a \
	$(top_srcdir)/src/native/libnative.a \
	$(top_srcdir)/src/native/vm/libnativevm.a \
	$(top_srcdir)/src/toolbox/libtoolbox.a \
	$(top_srcdir)/src/boehm-gc/.libs/libgc.a \
	$(CLASSPATH_LIBS) \
	@AWT_OBJS@


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
