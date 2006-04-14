## src/vm/jit/Makefile.am
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
## Changes: Edwin Steiner
##
## $Id: Makefile.am 4773 2006-04-14 11:20:38Z twisti $

## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR)

LIBS =

DIST_SUBDIRS = \
	allocator \
	ifconv \
	inline \
	loop \
	profile \
	schedule \
	tools \
	verify \
	\
	intrp \
	alpha \
	i386 \
	mips \
	powerpc \
	x86_64

SUBDIRS = \
	allocator \
	tools \
	verify

if ENABLE_JIT
SUBDIRS += $(ARCH_DIR)
ARCH_LIB = $(ARCH_DIR)/libarch.la
endif

if ENABLE_INTRP
SUBDIRS += intrp
INTRP_LIB = intrp/libintrp.la
endif

if ENABLE_IFCONV
SUBDIRS += ifconv
IFCONV_LIB = ifconv/libifconv.la
endif

if ENABLE_INLINING
SUBDIRS += inline
INLINE_LIB = inline/libinline.la
endif

if ENABLE_LOOP
SUBDIRS += loop
LOOP_LIB = loop/libloop.la
endif

if ENABLE_PROFILING
SUBDIRS += profile
PROFILE_LIB = profile/libprofile.la
endif

if USE_SCHEDULER
SUBDIRS += schedule
SCHEDULE_LIB = schedule/libschedule.la
endif

if WITH_BINUTILS_DISASSEMBLER
DISASS_SOURCES = disass-common.c
else
DISASS_SOURCES =
endif

noinst_HEADERS = \
	abi.h \
	abi-asm.h \
	asmpart.h \
	emit.h \
	methodheader.h \
	patcher.h 

noinst_LTLIBRARIES = libjit.la

libjit_la_SOURCES = \
	code.c \
	code.h \
	codegen-common.c \
	codegen-common.h \
	disass.h \
	$(DISASS_SOURCES) \
	dseg.c \
	dseg.h \
	jit.c \
	jit.h \
	parse.c \
	parse.h \
	reg.c \
	reg.h \
	replace.c \
	replace.h \
	stack.c \
	stack.h \
	stacktrace.c \
	stacktrace.h

libjit_la_LIBADD = \
	allocator/liballocator.la \
	verify/libverify.la \
	$(IFCONV_LIB) \
	$(INLINE_LIB) \
	$(LOOP_LIB) \
	$(PROFILE_LIB) \
	$(SCHEDULE_LIB) \
	$(INTRP_LIB) \
	$(ARCH_LIB)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
