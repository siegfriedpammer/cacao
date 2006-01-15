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
## $Id: Makefile.am 4207 2006-01-15 00:37:34Z twisti $

## Process this file with automake to produce Makefile.in

SUBDIRS = codepatching native regression

EXTRA_DIST = \
	hello.java \
	prop.java \
	x.java \
	x.output \
	TestBase.java \
	TestArrayClasses.java \
	TestArrayClasses.output \
	BasicToStrings.java \
	BasicToStrings.output \
	GCBench.java \
	JavaPerformance.java \
	param_test.java

JAVA = $(top_builddir)/src/cacao/cacao

checkall: x.tst TestArrayClasses.tst BasicToStrings.tst param_test.tst

%.tst:
	$(JAVAC) $*.java
	sh Test.sh $(JAVA) $*
#	$(JAVA) $* > $*.thisoutput
#	diff --brief $*.output $*.thisoutput
	$(RM) $*.thisoutput
	$(RM) $*.this2output

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
