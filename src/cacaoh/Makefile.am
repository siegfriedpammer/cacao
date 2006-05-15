## src/cacaoh/Makefile.am
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
## $Id: Makefile.am 4921 2006-05-15 14:24:36Z twisti $

## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR)

if DISABLE_GC
GC_LIB =
else
GC_LIB = $(top_builddir)/src/boehm-gc/libgc.la
endif

if ENABLE_THREADS
THREAD_LIB = $(top_builddir)/src/threads/native/libthreads.la
endif

if ENABLE_RT_TIMING
cacaoh_LDFLAGS = -lrt
endif

noinst_LTLIBRARIES = libcacaoh.la

libcacaoh_la_SOURCES = \
	headers.c \
	headers.h

libcacaoh_la_LIBADD = \
	$(top_builddir)/src/mm/libmm.la \
	$(THREAD_LIB) \
	$(top_builddir)/src/toolbox/libtoolbox.la \
	$(top_builddir)/src/vm/libvmcore.la

noinst_PROGRAMS = cacaoh

cacaoh_SOURCES = \
	cacaoh.c

cacaoh_LDADD = \
	libcacaoh.la \
	$(top_builddir)/src/fdlibm/libfdlibm.la \
	$(GC_LIB)

cacaoh_DEPENDENCIES = \
	$(cacaoh_LDADD)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
