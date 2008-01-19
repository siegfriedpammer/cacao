## tests/regression/Makefile.am
##
## Copyright (C) 1996-2005, 2006, 2007, 2008
## CACAOVM - Verein zu Foerderung der freien virtuellen Machine CACAO
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


SUBDIRS = \
	codepatching \
	jasmin \
	native

JAVA      = $(top_builddir)/src/cacao/cacao

if WITH_CLASSPATH_GNU
JAVAFLAGS = -Xbootclasspath:$(top_builddir)/src/lib/classes/:$(CLASSPATH_CLASSES)
else
JAVAFLAGS = -Xbootclasspath:$(CLASSPATH_CLASSES)
endif

SOURCE_FILES = \
	$(srcdir)/HelloWorld.java \
	$(srcdir)/jctest.java \
	$(srcdir)/fptest.java \
	$(srcdir)/fp.java \
	$(srcdir)/extest.java \
	$(srcdir)/clinitexception.java \
	$(srcdir)/LoadDisplacementOverflow.java \
	$(srcdir)/FieldDisplacementOverflow.java \
	$(srcdir)/StackDisplacementOverflow.java \
	$(srcdir)/MinimalClassReflection.java \
	$(srcdir)/TestAnnotations.java

EXTRA_DIST = \
	$(SOURCE_FILES) \
	Test.sh \
	\
	jctest.output \
	fptest.output \
	fp.output\
	extest.2output \
	clinitexception.2output \
	LoadDisplacementOverflow.output \
	FieldDisplacementOverflow.output \
	StackDisplacementOverflow.output \
	MinimalClassReflection.output \
	TestAnnotations.output

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
	StackDisplacementOverflow \
	MinimalClassReflection \
	TestAnnotations

check: build $(SIMPLE_JAVA_TESTS) $(OUTPUT_JAVA_TESTS)

build:
	@$(JAVAC) -d . $(SOURCE_FILES)

$(SIMPLE_JAVA_TESTS):
	@echo "$@: "
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(JAVA) $(JAVAFLAGS) $@


$(OUTPUT_JAVA_TESTS):
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) $(JAVAFLAGS)" $@ $(srcdir)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
