## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1644 2004-12-01 14:05:27Z twisti $

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

if USE_GTK_PEER
CLASSPATH_LIBS += \
	$(top_srcdir)/src/classpath/native/jni/gtk-peer/.libs/libgtkpeer.a
endif

else
cacao_LDFLAGS = -ldl -Wl,--export-dynamic
CLASSPATH_LIBS =

if USE_GTK_PEER
cacao_LDFLAGS += \
	-lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lpangoxft-1.0 \
	-lpangox-1.0 -lpango-1.0 -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 \
	-lgthread-2.0 -lart_lgpl_2
endif
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
	$(CLASSPATH_LIBS)

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
	$(CLASSPATH_LIBS)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
