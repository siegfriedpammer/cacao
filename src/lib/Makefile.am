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
## $Id: Makefile.am 3303 2005-09-29 13:07:57Z twisti $

## Process this file with automake to produce Makefile.in

CLEANFILES = vm.zip

JAVA_VM_FILES = \
	vm/reference/gnu/classpath/VMSystemProperties.java \
	vm/reference/gnu/classpath/VMStackWalker.java \
	vm/reference/gnu/java/nio/VMPipe.java \
	vm/reference/gnu/java/nio/VMSelector.java \
	vm/reference/java/io/VMFile.java \
	vm/reference/java/io/VMObjectStreamClass.java \
	vm/reference/java/io/VMObjectInputStream.java \
	vm/reference/java/lang/VMClass.java \
	vm/reference/java/lang/VMClassLoader.java \
	vm/reference/java/lang/VMCompiler.java \
	vm/reference/java/lang/VMDouble.java \
	vm/reference/java/lang/VMFloat.java \
	vm/reference/java/lang/VMObject.java \
	vm/reference/java/lang/VMProcess.java \
	vm/reference/java/lang/VMRuntime.java \
	vm/reference/java/lang/VMSystem.java \
	vm/reference/java/lang/VMString.java \
	vm/reference/java/lang/VMThread.java \
	vm/reference/java/lang/VMThrowable.java \
	vm/reference/java/lang/reflect/Constructor.java \
	vm/reference/java/lang/reflect/Field.java \
	vm/reference/java/lang/reflect/Method.java \
	vm/reference/java/lang/reflect/VMProxy.java \
	vm/reference/java/net/VMInetAddress.java \
	vm/reference/java/nio/channels/VMChannels.java \
	vm/reference/java/nio/VMDirectByteBuffer.java \
	vm/reference/java/security/VMAccessController.java \
	vm/reference/java/util/VMTimeZone.java

# currently disabled by GNU classpath
#	vm/reference/gnu/classpath/jdwp/VMFrame.java \
#	vm/reference/gnu/classpath/jdwp/VMIdManager.java \
#	vm/reference/gnu/classpath/jdwp/VMVirtualMachine.java \
#

GLIBJ_ZIP = $(CLASSPATH_INSTALL_DIR)/share/classpath/glibj.zip

pkgdata_DATA = vm.zip

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
