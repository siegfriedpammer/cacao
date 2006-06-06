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
## $Id: Makefile.am 5013 2006-06-06 11:22:56Z twisti $

## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR) -I$(top_builddir)/src

LIBS =

SUBDIRS = \
	include \
	vm

CLEANFILES = \
	nativetable.inc

if ENABLE_JVMTI
SUBDIRS += jvmti
JVMTI_LIB = jvmti/libjvmti.la
endif

if WITH_STATIC_CLASSPATH
SUBDIRS += tools

CLASSPATH = $(top_builddir)/src/lib/classes/:$(CLASSPATH_GLIBJ_ZIP)

$(srcdir)/native.c: nativetable.inc

nativetable.inc: $(VM_ZIP) $(CLASSPATH_GLIBJ_ZIP) $(top_builddir)/src/native/tools/gennativetable
	$(top_builddir)/src/native/tools/gennativetable -bootclasspath $(CLASSPATH) > $(top_builddir)/src/native/nativetable.inc
endif

noinst_LTLIBRARIES = libnative.la

libnative_la_SOURCES = \
	jni.c \
	jni.h \
	native.c \
	native.h

libnative_la_LIBADD = \
	vm/libnativevm.la \
	$(JVMTI_LIB)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
