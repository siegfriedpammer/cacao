## src/vm/Makefile.am
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
## $Id: Makefile.am 4762 2006-04-12 22:20:49Z edwin $

## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR)

LIBS =

SUBDIRS = jit

if ENABLE_STATISTICS
STATISTICS_OBJ = \
	statistics.c \
	statistics.h
endif

if ENABLE_RT_TIMING
RT_TIMING_OBJ = \
	rt-timing.c
endif

if ENABLE_ZLIB
ZLIB_OBJ = \
	zip.c \
	zip.h
endif

noinst_HEADERS = \
	global.h \
	types.h

noinst_LTLIBRARIES = \
	libvmcore.la \
	libvm.la

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
	properties.c \
	properties.h \
	references.h \
	resolve.c \
	resolve.h \
	$(RT_TIMING_OBJ) \
	rt-timing.h \
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
	exceptions.h \
	vm.c \
	vm.h

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
