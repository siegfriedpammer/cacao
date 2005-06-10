## src/vm/jit/powerpc/Makefile.am
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
## $Id: Makefile.am 2633 2005-06-10 18:22:29Z twisti $

## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@ -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@/@OS_DIR@ -I$(top_builddir) -I$(top_builddir)/src

SUBDIRS = @OS_DIR@

AM_CCASFLAGS = $(INCLUDES)

CLEANFILES = offsets.h

noinst_HEADERS = \
	arch.h \
	machine-instr.h \
	types.h

noinst_LTLIBRARIES = libarch.la

libarch_la_SOURCES = \
	asmoffsets.h \
	asmpart.S \
	codegen.c \
	codegen.h \
	dis-asm.h \
	disass.c \
	disass.h \
	patcher.c \
	ppc-dis.c \
	ppc-opc.c \
	ppc.h

libarch_la_LIBADD = \
	@OS_DIR@/libmd.la

EXTRA_DIST = \
	darwin/md-abi.h \
	darwin/md-abi.inc \
	darwin/md-asm.h \
	\
	linux/md-abi.h \
	linux/md-abi.inc \
	linux/md-asm.h

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
