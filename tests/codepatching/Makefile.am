## tests/codepatching/Makefile.am
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
## $Id: Makefile.am 3941 2005-12-11 14:06:03Z twisti $

## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	checkcastC.java \
	checkcastI.java \
	getfieldD.java \
	getfieldF.java \
	getfieldI.java \
	getfieldJ.java \
	getfieldL.java \
	getstaticD.java \
	getstaticF.java \
	getstaticI.java \
	getstaticJ.java \
	getstaticL.java \
	instanceofC.java \
	instanceofI.java \
	invokespecial.java \
	invokestatic.java \
	multianewarray.java \
	newarray.java \
	putfieldconstIF.java \
	putfieldconstJDL.java \
	putfieldD.java \
	putfieldF.java \
	putfieldI.java \
	putfieldJ.java \
	putfieldL.java \
	putstaticD.java \
	putstaticF.java \
	putstaticI.java \
	putstaticJ.java \
	putstaticL.java \
	test.java

MAINCLASS = \
	test.class

CLASSES = \
	checkcastC.class \
	checkcastI.class \
	getfieldD.class \
	getfieldF.class \
	getfieldI.class \
	getfieldJ.class \
	getfieldL.class \
	getstaticD.class \
	getstaticF.class \
	getstaticI.class \
	getstaticJ.class \
	getstaticL.class \
	instanceofC.class \
	instanceofI.class \
	invokespecial.class \
	invokestatic.class \
	multianewarray.class \
	newarray.class \
	putfieldconstIF.class \
	putfieldconstJDL.class \
	putfieldD.class \
	putfieldF.class \
	putfieldI.class \
	putfieldJ.class \
	putfieldL.class \
	putstaticD.class \
	putstaticF.class \
	putstaticI.class \
	putstaticJ.class \
	putstaticL.class

CLEANFILES = \
	$(MAINCLASS) \
	$(CLASSES)

JAVA = $(top_builddir)/src/cacao/cacao
JAVAC = jikes -nowarn

build:
	$(JAVAC) test.java

checkall: build
	$(JAVA) test

remove:
	$(RM) $(CLASSES)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
