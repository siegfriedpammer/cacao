## src/vm/Makefile.am
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
## $Id: Makefile.am 3863 2005-12-03 14:34:09Z twisti $

## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@ -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@/@OS_DIR@

SUBDIRS = jit

if STATISTICS
STATISTICS_OBJ = \
	statistics.c \
	statistics.h
endif

if USE_ZLIB
ZLIB_OBJ = \
	unzip.c \
	unzip.h
endif

noinst_HEADERS = \
	global.h \
	types.h

noinst_LTLIBRARIES = libvmcore.la libvm.la

libvmcore_la_SOURCES = \
	access.c \
	access.h \
	builtin.c \
	builtin.h \
	builtintable.inc \
	class.c \
	class.h \
	classcache.c \
	classcache.h \
	descriptor.c \
	descriptor.h \
	field.c \
	field.h \
	finalizer.c \
	finalizer.h \
	hashtable.c \
	hashtable.h \
	initialize.c \
	initialize.h \
	linker.c \
	linker.h \
	loader.c \
	loader.h \
	method.c \
	method.h \
	options.c \
	options.h \
	references.h \
	resolve.c \
	resolve.h \
	$(STATISTICS_OBJ) \
	signal.c \
	signallocal.h \
	string.c \
	stringlocal.h \
	suck.c \
	suck.h \
	utf8.c \
	utf8.h \
	$(ZLIB_OBJ)

libvm_la_SOURCES = \
	exceptions.c \
	exceptions.h

libvm_la_LIBADD = \
	libvmcore.la \
	jit/libjit.la


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
