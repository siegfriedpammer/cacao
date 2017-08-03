## src/vm/jit/compiler2/x86_64/Makefile.am
##
## Copyright (C) 2013
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


AM_CPPFLAGS += -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR) -I$(top_builddir)/src

LIBS =

noinst_LTLIBRARIES = \
	libaarch64.la

libaarch64_la_SOURCES = \
	Aarch64.hpp \
	Aarch64Backend.cpp \
	Aarch64Backend.hpp \
	Aarch64Cond.cpp \
	Aarch64Cond.hpp \
	Aarch64Emitter.cpp \
	Aarch64Emitter.hpp \
	Aarch64Instructions.cpp \
	Aarch64Instructions.hpp \
	Aarch64MachineMethodDescriptor.cpp \
	Aarch64MachineMethodDescriptor.hpp \
	Aarch64Register.cpp \
	Aarch64Register.hpp \
	Grammar.inc \
	Target.hpp

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
