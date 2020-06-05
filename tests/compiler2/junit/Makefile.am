## tests/compiler2/junit/Makefile.am
##
## Copyright (C) 1996-2014
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


SUBDIRS =

JAVA     = LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH} $(top_builddir)/src/cacao/cacao
JAVACMD  = $(JAVA) -Xbootclasspath:$(BOOTCLASSPATH)
JAVACCMD = $(JAVAC) -source 1.6 -target 1.6 -nowarn -bootclasspath $(BOOTCLASSPATH)
TIMING = 

EXTRA_DIST = \
	$(srcdir)/org/cacaojvm/compiler2/test/*.java \
	$(srcdir)/org/cacaojvm/compiler2/test/aarch64/*.java

CLEANFILES = \
	$(builddir)/org/cacaojvm/compiler2/test/*.class \
	$(builddir)/org/cacaojvm/compiler2/test/aarch64/*.class

check: build run

build:
	$(JAVACCMD) -Xbootclasspath/a:$(JUNIT_JAR) -d . $(srcdir)/org/cacaojvm/compiler2/test/*.java
	$(JAVACCMD) -Xbootclasspath/a:$(JUNIT_JAR) -d . $(srcdir)/org/cacaojvm/compiler2/test/aarch64/*.java
	$(JAVACCMD) -Xbootclasspath/a:$(JUNIT_JAR) -d . $(srcdir)/org/cacaojvm/compiler2/test/regalloc/*.java
	
	
run:
	$(JAVACMD) -Xbootclasspath/a:$(JUNIT_JAR):. -DTIMING=$(TIMING) org.cacaojvm.compiler2.test.CacaoJUnitCore org.cacaojvm.compiler2.test.All
	$(JAVACMD) -Xbootclasspath/a:$(JUNIT_JAR):. -DTIMING=$(TIMING) org.cacaojvm.compiler2.test.CacaoJUnitCore org.cacaojvm.compiler2.test.aarch64.All
	$(JAVACMD) -Xbootclasspath/a:$(JUNIT_JAR):. -DTIMING=$(TIMING) org.cacaojvm.compiler2.test.CacaoJUnitCore org.cacaojvm.compiler2.test.regalloc.All
	

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
