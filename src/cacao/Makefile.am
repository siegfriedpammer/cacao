## cacao/Makefile.am
##
## Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
## R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
## C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
## TU Wien
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
## $Id: Makefile.am 1676 2004-12-03 16:52:15Z twisti $

## Process this file with automake to produce Makefile.in

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
cacao_LDFLAGS =
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
endif

if USE_GTK_PEER
cacao_LDFLAGS += \
	-lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lpangoxft-1.0 \
	-lpangox-1.0 -lpango-1.0 -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 \
	-lgthread-2.0 -lart_lgpl_2
endif

bindir = $(prefix)/jre/bin

bin_PROGRAMS = cacao

cacao_SOURCES = \
	cacao.c \
	cacao.h

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
