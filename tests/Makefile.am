## tests/Makefile.am
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
## $Id: Makefile.am 1820 2004-12-23 11:19:44Z twisti $

## Process this file with automake to produce Makefile.in

SUBDIRS = jvm98 kaffe native

EXTRA_DIST = \
	x.java x.output \
	jctest.java jctest.output \
	fptest.java fptest.output \
	fp.java fp.output \
	extest.java extest.output \
	TestBase.java TestArrayClasses.java TestArrayClasses.output \
	BasicToStrings.java BasicToStrings.output \
	GCBench.java \
	JavaPerformance.java

JAVA = $(top_srcdir)/src/cacao/cacao
JAVAC = jikes -nowarn

checkall: x.tst jctest.tst fptest.tst fp.tst extest.tst TestArrayClasses.tst BasicToStrings.tst

checkrun: GCBench.tstrun JavaPerformance.tstrun

%.tst:
	$(JAVAC) $*.java
	$(JAVA) $* > $*.thisoutput
	diff --brief $*.output $*.thisoutput
	rm -f $*.thisoutput

%.tstrun:
	$(JAVAC) $*.java
	$(JAVA) $*


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
