## tests/regression/Makefile.am
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
## $Id: Makefile.am 4218 2006-01-17 16:55:21Z twisti $

## Process this file with automake to produce Makefile.in

JAVA      = $(top_builddir)/src/cacao/cacao
JAVAFLAGS = -Xbootclasspath:$(top_builddir)/src/lib/vm.zip:$(CLASSPATH_PREFIX)/share/classpath/$(GLIBJZ)

EXTRA_DIST = \
	Test.sh \
	HelloWorld.java \
	jctest.java \
	jctest.output \
	fptest.java \
	fptest.output \
	fp.java \
	fp.output\
	extest.java \
	extest.2output \
	clinitexception.java \
	clinitexception.2output \
	LoadDisplacementOverflow.java \
	LoadDisplacementOverflow.output \
	FieldDisplacementOverflow.java \
	FieldDisplacementOverflow.output \
	StackDisplacementOverflow.java \
	StackDisplacementOverflow.output

CLEANFILES = \
	*.class \
	*.thisoutput

SIMPLE_JAVA_TESTS = \
	HelloWorld

OUTPUT_JAVA_TESTS = \
	jctest \
	fptest \
	fp \
	extest \
	\
	clinitexception \
	LoadDisplacementOverflow \
	FieldDisplacementOverflow \
	StackDisplacementOverflow

check: $(SIMPLE_JAVA_TESTS) $(OUTPUT_JAVA_TESTS)

$(SIMPLE_JAVA_TESTS):
	@$(JAVAC) -d . $(srcdir)/$@.java
	@echo -n "$@: "
	@$(JAVA) $(JAVAFLAGS) $@


$(OUTPUT_JAVA_TESTS):
	@$(JAVAC) -d . $(srcdir)/$@.java
	@$(SHELL) $(srcdir)/Test.sh "$(JAVA) $(JAVAFLAGS)" $@ $(srcdir)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
