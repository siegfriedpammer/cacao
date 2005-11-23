## src/vm/jit/intrp/Makefile.am
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
## $Id: Makefile.am 3790 2005-11-23 23:16:14Z twisti $

## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR) -I$(top_srcdir)/src/libffi/include -I$(top_builddir) -I$(top_builddir)/src

EXTRA_DIST = \
	java.vmg

noinst_LTLIBRARIES = libintrp.la

libintrp_la_SOURCES = \
	asmpart.c \
	codegen.c \
	codegen.h \
	disasm.c \
	disass.c \
	engine.c \
	intrp.h \
	md.c \
	patcher.c \
	peephole.c \
	\
	java-vm.i \
	java-disasm.i \
	java-gen.i \
	java-labels.i \
	java-profile.i \
	java-peephole.i

java-vm.i java-disasm.i java-gen.i java-labels.i java-profile.i java-peephole.i: java.vmg
	vmgen java.vmg


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
