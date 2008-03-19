## src/cacao/Makefile.am
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


AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR)

if ENABLE_THREADS
THREAD_LIB = \
	$(top_builddir)/src/threads/libthreads.la
endif

if ENABLE_STATICVM
cacao_LDFLAGS = \
	-all-static
else
cacao_LDFLAGS =
endif

if ENABLE_LIBJVM
lib_LTLIBRARIES = \
	libjvm.la

libjvm_la_LDFLAGS = \
	-version-info 1:0:0

if WITH_CLASSPATH_SUN
libjvm_la_LDFLAGS += \
	-Xlinker --version-script=$(top_srcdir)/contrib/mapfile-vers-product
endif

cacao_LDFLAGS += \
	-export-dynamic

CACAO_LIBS =
else
noinst_LTLIBRARIES = \
	libjvm.la

cacao_LDFLAGS += \
	-export-dynamic

CACAO_LIBS = \
	libjvm.la
endif

if ENABLE_RT_TIMING
cacao_LDFLAGS += -lrt
endif

if ENABLE_PYTHON
cacao_LDFLAGS += \
    @PYTHON_LSPEC@
endif

libjvm_la_SOURCES =

libjvm_la_LIBADD = \
	$(top_builddir)/src/fdlibm/libfdlibm.la \
	$(top_builddir)/src/mm/libmm.la \
	$(top_builddir)/src/native/libnative.la \
	$(top_builddir)/src/toolbox/libtoolbox.la \
	$(top_builddir)/src/vm/libvm.la \
	$(top_builddir)/src/vmcore/libvmcore.la \
	$(GC_LIB) \
	$(THREAD_LIB)

bin_PROGRAMS = \
	cacao

cacao_SOURCES = \
	cacao.c

cacao_LDADD = \
	$(CACAO_LIBS)

cacao_DEPENDENCIES = \
	$(cacao_LDADD)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
