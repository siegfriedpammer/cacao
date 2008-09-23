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
	assertion.cpp \
	assertion.hpp
endif

if ENABLE_CYCLES_STATS
CYCLES_STATS_SOURCES = \
	cycles-stats.c \
	cycles-stats.h
endif

if ENABLE_JAVASE
if ENABLE_ANNOTATIONS
ANNOTATION_SOURCES = \
	annotation.c \
	annotation.h
endif

STACKMAP_SOURCES = \
	stackmap.c \
	stackmap.h
endif

if ENABLE_RT_TIMING
RT_TIMING_SOURCES = \
	rt-timing.c \
	rt-timing.h
endif

if ENABLE_STATISTICS
STATISTICS_SOURCES = \
	statistics.c \
	statistics.h
endif

if ENABLE_ZLIB
ZLIB_SOURCES = \
	zip.c \
	zip.h
endif

noinst_HEADERS = \
	global.h \
	types.h

noinst_LTLIBRARIES = \
	libvm.la

libvm_la_SOURCES = \
	access.c \
	access.h \
	$(ANNOTATION_SOURCES) \
	array.cpp \
	array.hpp \
	$(ASSERTION_SOURCES) \
	class.cpp \
	class.hpp \
	classcache.c \
	classcache.h \
	$(CYCLES_STATS_SOURCES) \
	descriptor.c \
	descriptor.h \
	exceptions.cpp \
	exceptions.hpp \
	field.cpp \
	field.hpp \
	finalizer.c \
	finalizer.h \
	globals.cpp \
	globals.hpp \
	initialize.cpp \
	initialize.hpp \
	javaobjects.cpp \
	javaobjects.hpp \
	linker.c \
	linker.h \
	loader.cpp \
	loader.hpp \
	method.c \
	method.h \
	options.c \
	options.h \
	os.cpp \
	os.hpp \
	package.cpp \
	package.hpp \
	primitive.cpp \
	primitive.hpp \
	properties.cpp \
	properties.hpp \
	references.h \
	resolve.cpp \
	resolve.hpp \
	$(RT_TIMING_SOURCES) \
	signal.c \
	signallocal.h \
	$(STACKMAP_SOURCES) \
	$(STATISTICS_SOURCES) \
	string.cpp \
	string.hpp \
	suck.cpp \
	suck.hpp \
	utf8.c \
	utf8.h \
	vftbl.hpp \
	vm.cpp \
	vm.hpp \
	$(ZLIB_SOURCES)

libvm_la_LIBADD = \
	jit/libjit.la


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
