## src/cacao/Makefile.am
##
## Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
## C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
## E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
## J. Wenninger, Institut f. Computersprachen - TU Wien
##
## This file is part of CACAO.
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as
## published by the Free Software Foundation; either version 2, or (at
## your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
## 02110-1301, USA.
##
## Contact: cacao@cacaojvm.org
##
## Authors: Christian Thalinger
##
## Changes:
##
## $Id: Makefile.am 4627 2006-03-16 12:53:32Z twisti $

## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR)

if DISABLE_GC
if NATIVE_THREADS
GC_LIB = $(top_builddir)/src/boehm-gc/libgc.la
else
GC_LIB =
endif
else
GC_LIB = $(top_builddir)/src/boehm-gc/libgc.la
endif

if USE_THREADS
if NATIVE_THREADS
THREAD_LIB = $(top_builddir)/src/threads/native/libthreads.la
else
THREAD_LIB = $(top_builddir)/src/threads/green/libthreads.la
endif
else
THREAD_LIB =
endif

if ENABLE_STATICVM
noinst_LTLIBRARIES = \
	libcacaovm.la

cacao_LDFLAGS = \
	$(EXPORT_SYMBOLS_FLAGS)
else
lib_LTLIBRARIES = \
	libcacaovm.la

libcacaovm_la_LDFLAGS = \
	-release $(PACKAGE_VERSION)

cacao_LDFLAGS = \
	$(EXPORT_SYMBOLS_FLAGS)
endif

if WITH_STATIC_CLASSPATH
cacao_LDFLAGS += \
	$(LTLIBICONV)

CLASSPATH_LIBS = \
	$(CLASSPATH_LIBDIR)/classpath/libjavaio.a \
	$(CLASSPATH_LIBDIR)/classpath/libjavanio.a \
	$(CLASSPATH_LIBDIR)/classpath/libjavanet.a \
	$(CLASSPATH_LIBDIR)/classpath/libjavautil.a \
	$(CLASSPATH_LIBDIR)/classpath/libjavalang.a \
	$(CLASSPATH_LIBDIR)/classpath/libjavalangreflect.a

#if USE_GTK_PEER
#cacao_CFLAGS = \
#	$(GTK_CFLAGS) $(CAIRO_CFLAGS) $(PANGOFT2_CFLAGS) \
#	$(XML_CFLAGS) $(XSLT_CFLAGS)
#
#cacao_LDFLAGS += \
#	$(GTK_LIBS) $(CAIRO_LIBS) $(PANGOFT2_LIBS) $(X_LIBS) -lXtst \
#	$(XML_LIBS) $(XSLT_LIBS)
#
#CLASSPATH_LIBS += \
#	$(CLASSPATH_LIBDIR)/classpath/libgtkpeer.a
#endif
endif

libcacaovm_la_SOURCES =

libcacaovm_la_LIBADD = \
	$(top_builddir)/src/fdlibm/libfdlibm.la \
	$(top_builddir)/src/mm/libmm.la \
	$(top_builddir)/src/native/libnative.la \
	$(top_builddir)/src/toolbox/libtoolbox.la \
	$(top_builddir)/src/vm/libvm.la \
	$(GC_LIB) \
	$(THREAD_LIB)

bin_PROGRAMS = \
	cacao

cacao_SOURCES = \
	cacao.c \
	cacao.h

cacao_LDADD = \
	libcacaovm.la \
	$(CLASSPATH_LIBS)

cacao_DEPENDENCIES = \
	$(cacao_LDADD)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
