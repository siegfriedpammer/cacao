## src/lib/Makefile.am
##
## Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
## C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
## E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
## J. Wenninger, Institut f. Computersprachen - TU Wien
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
##
## Contact: cacao@cacaojvm.org
##
## Authors: Christian Thalinger
##
## Changes:
##
## $Id: Makefile.am 4357 2006-01-22 23:33:38Z twisti $

## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	$(JAVA_VM_FILES)

CLEANFILES = vm.zip

JAVA_VM_FILES = \
	$(top_srcdir)/src/lib/vm/reference/gnu/classpath/VMStackWalker.java \
	$(top_srcdir)/src/lib/vm/reference/java/lang/VMClassLoader.java \
	$(top_srcdir)/src/lib/vm/reference/java/lang/VMString.java \
	$(top_srcdir)/src/lib/vm/reference/java/lang/VMThread.java \
	$(top_srcdir)/src/lib/vm/reference/java/lang/VMThrowable.java \
	$(top_srcdir)/src/lib/vm/reference/java/security/VMAccessController.java

GLIBJ_ZIP = $(CLASSPATH_PREFIX)/share/classpath/$(GLIBJZ)

pkgdata_DATA = vm.zip

vm.zip: $(JAVA_VM_FILES)
	$(mkdir_p) classes
	$(JAVAC) -bootclasspath $(GLIBJ_ZIP) -d classes $(JAVA_VM_FILES)
	cd classes && $(ZIP) -r ../vm.zip .

clean-local:
	-rm -rf classes


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
