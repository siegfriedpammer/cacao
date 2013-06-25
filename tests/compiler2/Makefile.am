## tests/regression/Makefile.am
##
## Copyright (C) 1996-2005, 2006, 2007, 2008
## CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
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
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
## 02110-1301, USA.

JAVA     = $(top_builddir)/src/cacao/cacao
JAVACMD  = $(JAVA) -Xbootclasspath:$(BOOTCLASSPATH)
JAVACCMD = $(JAVAC) -source 1.5 -target 1.5 -nowarn -bootclasspath $(BOOTCLASSPATH)

SOURCE_FILES = \
	$(srcdir)/Test.java

EXTRA_DIST = \
	$(SOURCE_FILES) \
	Compile.sh

CLEANFILES = \
	*.class \
	*.o \
	*.run \
	*.thisoutput

OUTPUT_JAVA_TESTS = \
	Test.fact \
	Test.OverflowLong \
	Test.testParameterLong

check: build run

build:
	$(JAVACCMD) -d . $(SOURCE_FILES)

run: $(OUTPUT_JAVA_TESTS)

%: %.thisoutput
	diff $(srcdir)/$@.output $<

%.o:
	LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Compile.sh "$(JAVACMD)" $@ $(srcdir)

%.run: %.o $(srcdir)/%.main.c
	$(CC) $^ -o $@

%.thisoutput: %.run
	./$< > $@

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
