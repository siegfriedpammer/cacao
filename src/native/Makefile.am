## src/native/Makefile.am
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
## $Id: Makefile.am 4419 2006-02-03 22:57:32Z twisti $

## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_builddir)/src

SUBDIRS = \
	include \
	vm

CLEANFILES =

if ENABLE_JVMTI
SUBDIRS += jvmti
JVMTI_LIB = jvmti/libjvmti.la
endif

if ENABLE_STATICVM
SUBDIRS += tools
CLEANFILES += nativetable.inc

$(srcdir)/native.c: nativetable.inc

noinst_LTLIBRARIES = libnative.la

libnative_la_SOURCES = \
	jni.c \
	jni.h \
	native.c \
	native.h

libnative_la_LIBADD = \
	vm/libnativevm.la \
	$(JVMTI_LIB)

VM_ZIP = $(top_builddir)/src/lib/$(VM_ZIP_STRING)
GLIBJ_ZIP = $(CLASSPATH_PREFIX)/share/classpath/$(GLIBJ_ZIP_STRING)

CLASSPATH = $(VM_ZIP):$(GLIBJ_ZIP)

#nativetable.inc: $(VM_ZIP) $(GLIBJ_ZIP) $(top_builddir)/src/native/tools/gennativetable
nativetable.inc:
	$(top_builddir)/src/native/tools/gennativetable -bootclasspath $(CLASSPATH) > $(top_builddir)/src/native/nativetable.inc
endif


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
