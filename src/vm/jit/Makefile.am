## src/vm/jit/Makefile.am
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
## $Id: Makefile.am 2032 2005-03-10 16:33:45Z twisti $

## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@

SUBDIRS = inline loop tools verify @ARCH_DIR@

DIST_SUBDIRS = inline loop schedule tools verify alpha i386 mips powerpc x86_64

if USE_SCHEDULER
SCHEDULE_LIB = schedule/libschedule.la
SUBDIRS += schedule
endif

noinst_HEADERS = \
	asmpart.h \
	codegen.inc \
	codegen.inc.h \
	lsra.inc \
	lsra.h \
	reg.inc \
	reg.h

noinst_LTLIBRARIES = libjit.la

libjit_la_SOURCES = \
	jit.c \
	jit.h \
	parse.c \
	parse.h \
	stack.c \
	stack.h \
	stacktrace.c \
	stacktrace.h

libjit_la_LIBADD = \
	inline/libinline.la \
	loop/libloop.la \
	$(SCHEDULE_LIB) \
	verify/libverify.la \
	@ARCH_DIR@/libarch.la


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
