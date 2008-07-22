## src/vm/Makefile.am
##
## Copyright (C) 1996-2005, 2006, 2007, 2008
## CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
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


AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR) -I$(top_builddir)/src -I$(top_srcdir)/contrib/vmlog -I$(top_srcdir)/src/native

LIBS =

SUBDIRS = jit

if ENABLE_ASSERTION
ASSERTION_SOURCES = \
	assertion.c \
	assertion.h
endif

if ENABLE_CYCLES_STATS
CYCLES_STATS_SOURCES = \
	cycles-stats.c \
	cycles-stats.h
endif

noinst_HEADERS = \
	global.h \
	types.h

noinst_LTLIBRARIES = \
	libvm.la

libvm_la_SOURCES = \
	$(ASSERTION_SOURCES) \
	access.c \
	access.h \
	array.c \
	array.h \
	builtin.c \
	builtin.h \
	builtintable.inc \
	$(CYCLES_STATS_SOURCES) \
	exceptions.cpp \
	exceptions.hpp \
	finalizer.c \
	finalizer.h \
	initialize.c \
	initialize.h \
	jit_interface.h \
	package.cpp \
	package.hpp \
	primitive.cpp \
	primitive.hpp \
	properties.c \
	properties.h \
	resolve.c \
	resolve.h \
	signal.c \
	signallocal.h \
	string.cpp \
	string.hpp \
	vm.cpp \
	vm.hpp

libvm_la_LIBADD = \
	jit/libjit.la


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
