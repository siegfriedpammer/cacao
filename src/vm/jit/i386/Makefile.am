## src/vm/jit/i386/Makefile.am
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
## $Id: Makefile.am 3018 2005-07-12 22:12:20Z twisti $

## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR) -I$(top_builddir) -I$(top_builddir)/src

SUBDIRS = $(OS_DIR)

DIST_SUBDIRS = freebsd linux

AM_CCASFLAGS = $(INCLUDES)

CLEANFILES = offsets.h

noinst_HEADERS = \
	arch.h \
	icmd_uses_reg_res.inc \
	machine-instr.h \
	types.h \
	\
	md-abi.h \
	md-abi.inc

noinst_LTLIBRARIES = libarch.la

libarch_la_SOURCES = \
	ansidecl.h \
	asmoffsets.h \
	asmpart.S \
	bfd.h \
	codegen.c \
	codegen.h \
	dis-buf.c \
	dis-asm.h \
	disass.c \
	disass.h \
	emitfuncs.c \
	emitfuncs.h \
	i386-dis.c \
	md.c \
	patcher.c \
	symcat.h \
	\
	md-abi.c

libarch_la_LIBADD = \
	$(OS_DIR)/libmd.la

$(srcdir)/asmpart.S: $(top_builddir)/config.h offsets.h

offsets.h: $(top_builddir)/src/vm/jit/tools/genoffsets $(top_builddir)/config.h
	$(top_builddir)/src/vm/jit/tools/genoffsets > offsets.h


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
