## tests/compiler2/catch/Makefile.am
##
## Copyright (C) 1996-2014
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

AM_CPPFLAGS += -I$(top_srcdir) -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR)
AM_CPPFLAGS += -I$(top_srcdir)/src/vm/jit/compiler2/$(ARCH_DIR)

AM_JAVACFLAGS = -source 1.6 -target 1.6 -nowarn -bootclasspath $(BOOTCLASSPATH)

AUTOMAKE_OPTIONS = subdir-objects

#bin_PROGRAMS = catch-tests

javadir = $(builddir)
JAVAROOT = $(builddir)

## Add Java files you need compiled here
#dist_java_JAVA = \
#	CriticalEdgeSplitTest.java

## Add Test cases here
#catch_tests_SOURCES = \
#	CriticalEdgeSplitTest.cpp \
#	OperandSetTest.cpp

## Add the framework
#catch_tests_SOURCES += \
#	framework/CacaoVM.cpp \
#	framework/CacaoVM.hpp \
#	framework/PassRunner.cpp \
#	framework/PassRunner.hpp \
#	framework/tests-main.cpp

#LDADD = $(top_builddir)/src/cacao/libjvm.la

#TESTS = catch-tests

#check_PROGRAMS = $(TESTS)

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End: