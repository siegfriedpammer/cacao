## src/lib/Makefile.am
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
## $Id: Makefile.am 2035 2005-03-18 12:45:13Z twisti $

## Process this file with automake to produce Makefile.in

CLEANFILES = vm.zip

JAVA_VM_FILES = \
	$(top_srcdir)/src/classpath/vm/reference/gnu/java/nio/VMPipe.java \
	$(top_srcdir)/src/classpath/vm/reference/gnu/java/nio/VMSelector.java \
	$(top_srcdir)/src/classpath/vm/reference/gnu/classpath/VMSystemProperties.java \
	$(top_srcdir)/src/classpath/vm/reference/gnu/classpath/VMStackWalker.java \
	$(top_srcdir)/src/classpath/vm/reference/java/io/VMFile.java \
	$(top_srcdir)/src/classpath/vm/reference/java/io/VMObjectStreamClass.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/reflect/Constructor.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/reflect/Field.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/reflect/Method.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/VMClass.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/VMClassLoader.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/VMCompiler.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/VMDouble.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/VMFloat.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/VMObject.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/VMProcess.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/VMRuntime.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/VMSystem.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/VMString.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/VMThread.java \
	$(top_srcdir)/src/classpath/vm/reference/java/lang/VMThrowable.java \
	$(top_srcdir)/src/classpath/vm/reference/java/security/VMAccessController.java \
	$(top_srcdir)/src/classpath/vm/reference/java/util/VMTimeZone.java \
	$(top_srcdir)/src/classpath/vm/reference/java/nio/channels/VMChannels.java \
	$(top_srcdir)/src/classpath/vm/reference/java/nio/VMDirectByteBuffer.java

#GLIBJ_ZIP = $(EXTERNAL_CLASSPATH_PREFIX)/share/classpath/glibj.zip
GLIBJ_ZIP = ../classpath/lib/rt.jar

vmzipdir = $(prefix)/jre/lib
vmzip_DATA = vm.zip

vm.zip: $(JAVA_VM_FILES)
	jikes -bootclasspath $(GLIBJ_ZIP) -d classes $(JAVA_VM_FILES)
	cd classes && zip -r ../vm.zip .

clean-local:
	-rm -rf classes


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
