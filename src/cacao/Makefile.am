## src/cacao/Makefile.am
##
## Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
## R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
## C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
## Institut f. Computersprachen - TU Wien
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
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
## 02111-1307, USA.
##
## Contact: cacao@complang.tuwien.ac.at
##
## Authors: Christian Thalinger
##
## Changes:
##
## $Id: Makefile.am 2520 2005-05-23 15:22:08Z twisti $

## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@

if USE_THREADS
if NATIVE_THREADS
THREAD_LIB = $(top_builddir)/src/threads/native/libthreads.la
else
THREAD_LIB = $(top_builddir)/src/threads/green/libthreads.la
endif
else
THREAD_LIB =
endif

if STATIC_CLASSPATH
cacao_LDFLAGS = \
	$(LTLIBICONV)

CLASSPATH_LIBS = \
	$(top_builddir)/src/classpath/native/jni/java-io/.libs/libjavaio.a \
	$(top_builddir)/src/classpath/native/jni/java-nio/.libs/libjavanio.a \
	$(top_builddir)/src/classpath/native/jni/java-net/.libs/libjavanet.a \
	$(top_builddir)/src/classpath/native/jni/java-util/.libs/libjavautil.a \
	$(top_builddir)/src/classpath/native/jni/java-lang/.libs/libjavalang.a \
	$(top_builddir)/src/classpath/native/jni/java-lang/.libs/libjavalangreflect.a

if USE_GTK_PEER
cacao_CFLAGS = \
	$(GTK_CFLAGS) $(CAIRO_CFLAGS) $(PANGOFT2_CFLAGS) \
	$(XML_CFLAGS) $(XSLT_CFLAGS)

cacao_LDFLAGS += \
	$(GTK_LIBS) $(CAIRO_LIBS) $(PANGOFT2_LIBS) $(X_LIBS) -lXtst \
	$(XML_LIBS) $(XSLT_LIBS)

CLASSPATH_LIBS += \
	$(top_builddir)/src/classpath/native/jni/gtk-peer/.libs/libgtkpeer.a
endif

else
cacao_LDFLAGS = -ldl -Wl,--export-dynamic
CLASSPATH_LIBS =
endif

noinst_LTLIBRARIES = libcacao.la

libcacao_la_SOURCES =

libcacao_la_LIBADD = \
	$(top_builddir)/src/mm/libmm.la \
	$(top_builddir)/src/native/libnative.la \
	$(THREAD_LIB) \
	$(top_builddir)/src/toolbox/libtoolbox.la \
	$(top_builddir)/src/vm/libvm.la

bindir = $(prefix)/jre/bin

bin_PROGRAMS = cacao

cacao_SOURCES = \
	cacao.c \
	cacao.h

cacao_LDADD = \
	libcacao.la \
	$(top_builddir)/src/boehm-gc/libgc.la \
	$(CLASSPATH_LIBS)

cacao_DEPENDENCIES = $(cacao_LDADD)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
