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
## $Id: Makefile.am 1737 2004-12-07 17:31:44Z twisti $

## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src

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
	ppc-dis.c \
	ppc-opc.c \
	ppc.h

asmpart.S: $(top_srcdir)/config.h offsets.h

.S.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

offsets.h: $(top_srcdir)/src/vm/jit/tools/genoffsets $(top_srcdir)/config.h $(top_srcdir)/src/vm/global.h $(top_srcdir)/src/threads/native/threads.h
	$(top_srcdir)/src/vm/jit/tools/genoffsets > offsets.h


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
