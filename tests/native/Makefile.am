## src/tests/native/Makefile.am
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
## $Id: Makefile.am 2895 2005-07-04 20:38:29Z christian $

## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@

CLEANFILES = \
	libcheckjni.so \
	checkjni.class \
	checkjni.h \
	\
	libtestarguments.so \
	testarguments.class \
	testarguments.h

CACAO = $(top_srcdir)/src/cacao/cacao
CACAOC = jikes -nowarn 
CACAOH = $(top_srcdir)/src/cacaoh/cacaoh

checkall: test.tst checkjni.tst testarguments.tst
test:
	$(CACAOC) test.java
	$(CACAOH) test
	$(CC) -shared $(INCLUDES) $(CFLAGS) test.c -o libtest.so -fPIC

checkjni:
	$(CACAOC) checkjni.java
	$(CACAOH) checkjni
	$(CC) -shared $(INCLUDES) $(CFLAGS) checkjni.c -o libcheckjni.so -fPIC

check: testarguments
testarguments:
	$(CACAOC) testarguments.java
	$(CACAOH) testarguments
	$(CC) -shared $(INCLUDES) $(CFLAGS) testarguments.c -o libtestarguments.so -fPIC
	$(CACAOC) testgetobjectclass0.java
	$(CACAOH) testgetobjectclass0
	$(CC) -shared $(INCLUDES) $(CFLAGS) testgetobjectclass0.c -o libtestgetobjectclass0.so -fPIC

%.tst: %
	sh ../Test.sh $(CACAO) $*
	rm -f $*.thisoutput
	rm -f $*.this2output

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
