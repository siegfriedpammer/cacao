## src/native/Makefile.am
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
## $Id: Makefile.am 1699 2004-12-06 12:44:24Z twisti $

## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@

SUBDIRS = tools include vm

# include NO_REGEN_CLASSES, CLASSES and AWT_CLASSES
include $(srcdir)/classes.am

noinst_LTLIBRARIES = libnative.la

libnative_la_SOURCES = \
	jni.c \
	jni.h \
	native.c \
	native.h \
	nativecalls.inc

libnative_la_LIBADD = \
	vm/libnativevm.la

native.c: nativetable.inc nativecalls.inc

if USE_GTK_PEER
AWT_HEADERS_TOUCH_FILE = $(srcdir)/include/awtheaders-generated
endif

nativetable.inc: $(top_srcdir)/src/classpath/lib/bootstrap.zip $(srcdir)/tools/gennativetable $(top_srcdir)/config.h $(srcdir)/include/headers-generated $(AWT_HEADERS_TOUCH_FILE)
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
