## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1655 2004-12-02 16:51:20Z carolyn $

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@

SUBDIRS = tools include vm

# include NO_REGEN_CLASSES, CLASSES and AWT_CLASSES
include $(srcdir)/classes.am

noinst_LIBRARIES = libnative.a

libnative_a_SOURCES = \
	jni.c \
	native.c

native.c: nativetable.inc nativecalls.inc

if USE_GTK_PEER
AWT_HEADERS_TOUCH_FILE = $(srcdir)/include/awtheaders-generated
endif

nativetable.inc: $(top_srcdir)/src/classpath/lib/bootstrap.zip $(top_srcdir)/config.h $(srcdir)/include/headers-generated $(AWT_HEADERS_TOUCH_FILE)
	CLASSPATH=$(top_srcdir)/src/classpath/lib/bootstrap.zip \
	$(top_srcdir)/src/native/tools/gennativetable \
	$(NO_REGEN_CLASSES) \
	$(CLASSES) \
	$(AWT_CLASSES) \
	> nativetable.inc


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
