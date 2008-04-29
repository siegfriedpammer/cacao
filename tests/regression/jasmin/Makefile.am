## tests/regression/jasmin/Makefile.am
##
## Copyright (C) 1996-2005, 2006, 2008
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


JAVA     = LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(top_builddir)/src/cacao/cacao
JAVACMD  = $(JAVA) -Xbootclasspath:$(BOOTCLASSPATH)
JAVACCMD = $(JAVAC) -bootclasspath $(BOOTCLASSPATH)

EXTRA_DIST = \
	$(srcdir)/runtest \
	$(srcdir)/show \
	$(srcdir)/*.j

CLEANFILES = \
	*.class \
	TESTLOG TESTEXPECT TESTOUT TESTERR TESTSIA

check: build run

build:
	$(JAVACMD) -cp /usr/share/java/jasmin-sable.jar jasmin.Main $(srcdir)/*.j

run:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs JAVA="$(top_builddir)/src/cacao/cacao -Xbootclasspath:$(BOOTCLASSPATH)" $(srcdir)/runtest $(srcdir)/*.j


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
