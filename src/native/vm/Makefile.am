## src/native/vm/Makefile.am
##
## Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
## R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
## C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
## TU Wien
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
## $Id: Makefile.am 1718 2004-12-06 18:00:40Z twisti $

## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@

CLEANFILES = implementednatives.data

noinst_LTLIBRARIES = libnativevm.la

libnativevm_la_SOURCES = \
	Constructor.c \
	Field.c \
	Method.c \
	Proxy.c \
	VMClass.c \
	VMClassLoader.c \
	VMObject.c \
	VMRuntime.c \
	VMSecurityManager.c \
	VMString.c \
	VMSystem.c \
	VMThread.c \
	VMThrowable.c

all: implementednatives.data

implementednatives.data: $(libnativevm_a_SOURCES)
	echo "" > implementednatives.data
	grep "JNICALL Java_" *.c | perl -p -i -e "s/.*JNICALL\s//" | perl -p -i -e"s/\(.*//"  | ( \
		while read line; do \
			grep $$line $(top_srcdir)/src/native/include/*.h > /dev/null; \
			if test $$? == 0; then \
				echo $$line >> implementednatives.data; \
			fi ; \
		done )


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
