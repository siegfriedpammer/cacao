## src/vm/jit/Makefile.am
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


AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR) -I$(top_builddir)/src -I$(top_srcdir)/contrib/vmlog

LIBS =

DIST_SUBDIRS = \
	allocator \
	inline \
	ir \
	loop \
	optimizing \
	schedule \
	verify \
	\
	intrp \
	alpha \
	arm \
	i386 \
	m68k \
	mips \
	parisc \
	powerpc \
	powerpc64 \
	s390 \
	sparc64 \
	x86_64

SUBDIRS = \
	ir \
	optimizing \
	$(ARCH_DIR)

ARCH_LIB = \
	$(ARCH_DIR)/libarch.la

if ENABLE_JIT
SUBDIRS += \
	allocator

OPTIMIZING_LIB = \
	optimizing/liboptimizing.la
endif

if ENABLE_INTRP
SUBDIRS += \
	intrp

INTRP_LIB = \
	intrp/libintrp.la
endif

if ENABLE_INLINING
SUBDIRS += \
	inline

INLINE_LIB = \
	inline/libinline.la
endif

if ENABLE_LOOP
SUBDIRS += \
	loop

LOOP_LIB = \
	loop/libloop.la
endif

if USE_SCHEDULER
SUBDIRS += \
	schedule

SCHEDULE_LIB = \
	schedule/libschedule.la
endif

REPLACE_SOURCES =

if ENABLE_JIT
REG_SOURCES = \
	reg.c \
	reg.h

REPLACE_SOURCES += \
	replace.hpp

STACK_SOURCES = \
	stack.c \
	stack.h

TRAP_SOURCES = \
	trap.c \
	trap.h
endif

if ENABLE_REPLACEMENT
REPLACE_SOURCES += \
	replace.cpp
endif

if ENABLE_VERIFIER
SUBDIRS += \
	verify

VERIFIER_LIB = \
	verify/libverify.la
endif

if ENABLE_OPAGENT
OPAGENT_SOURCES = \
	oprofile-agent.cpp \
	oprofile-agent.hpp

endif

if WITH_BINUTILS_DISASSEMBLER
DISASS_SOURCES = disass-common.c
endif

noinst_HEADERS = \
	abi.h \
	abi-asm.h \
	asmpart.h \
	methodheader.h

noinst_LTLIBRARIES = \
	libjit.la

libjit_la_SOURCES = \
	argument.cpp \
	argument.hpp \
	builtin.cpp \
	builtin.hpp \
	builtintable.inc \
	code.cpp \
	code.hpp \
	codegen-common.cpp \
	codegen-common.hpp \
	disass.h \
	$(DISASS_SOURCES) \
	dseg.c \
	dseg.h \
	emit-common.cpp \
	emit-common.hpp \
	exceptiontable.c \
	exceptiontable.h \
	executionstate.c \
	executionstate.h \
	jit.cpp \
	jit.hpp \
	linenumbertable.cpp \
	linenumbertable.hpp \
	methodtree.c \
	methodtree.h \
	parse.c \
	parse.h \
	patcher-common.cpp \
	patcher-common.hpp \
	$(RECOMPILE_SOURCES) \
	$(REG_SOURCES) \
	$(REPLACE_SOURCES) \
	show.cpp \
	show.hpp \
	$(STACK_SOURCES) \
	stacktrace.cpp \
	stacktrace.hpp \
	stubs.cpp \
	stubs.hpp \
	trace.cpp \
	trace.hpp \
	$(TRAP_SOURCES) \
	$(OPAGENT_SOURCES)

libjit_la_SOURCES += \
	cfg.c \
	cfg.h

libjit_la_LIBADD = \
	allocator/liballocator.la \
	$(INLINE_LIB) \
	ir/libir.la \
	$(LOOP_LIB) \
	$(OPTIMIZING_LIB) \
	$(PROFILE_LIB) \
	$(SCHEDULE_LIB) \
	$(VERIFIER_LIB) \
	$(INTRP_LIB) \
	$(ARCH_LIB)

if ENABLE_PYTHON
libjit_la_SOURCES += \
	python.c
AM_CPPFLAGS += \
	@PYTHON_CSPEC@
LIBS += \
	@PYTHON_LSPEC@
endif


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
