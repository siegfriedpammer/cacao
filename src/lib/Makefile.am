## src/lib/Makefile.am
##
## Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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
## $Id: Makefile.am 8299 2007-08-13 08:41:18Z michi $

## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	$(VM_JAVA_FILES)

CLEANFILES = vm.zip

VM_JAVA_FILES = \
	$(top_srcdir)/src/lib/gnu/gnu/classpath/VMStackWalker.java \
	$(top_srcdir)/src/lib/gnu/gnu/classpath/VMSystemProperties.java \
	$(top_srcdir)/src/lib/gnu/gnu/java/lang/management/VMMemoryMXBeanImpl.java \
	$(top_srcdir)/src/lib/gnu/gnu/java/lang/management/VMRuntimeMXBeanImpl.java \
	$(top_srcdir)/src/lib/gnu/java/lang/VMClassLoader.java \
	$(top_srcdir)/src/lib/gnu/java/lang/VMString.java \
	$(top_srcdir)/src/lib/gnu/java/lang/VMThread.java \
	$(top_srcdir)/src/lib/gnu/java/lang/VMThrowable.java \
	$(top_srcdir)/src/lib/gnu/java/lang/reflect/Constructor.java \
	$(top_srcdir)/src/lib/gnu/java/lang/reflect/Field.java \
	$(top_srcdir)/src/lib/gnu/java/lang/reflect/Method.java \
	$(top_srcdir)/src/lib/gnu/java/security/VMAccessController.java \
	$(top_srcdir)/src/lib/gnu/sun/misc/Unsafe.java

VM_CLASS_FILES = \
	classes/gnu/classpath/VMStackWalker.class \
	classes/gnu/classpath/VMSystemProperties.class \
	classes/gnu/java/lang/management/VMMemoryMXBeanImpl.class \
	classes/gnu/java/lang/management/VMRuntimeMXBeanImpl.class \
	classes/java/lang/VMClassLoader.class \
	classes/java/lang/VMString.class \
	classes/java/lang/VMThread.class \
	classes/java/lang/VMThrowable.class \
	classes/java/lang/reflect/Constructor.class \
	classes/java/lang/reflect/Field.class \
	classes/java/lang/reflect/Method.class \
	classes/java/security/VMAccessController.class \
	classes/sun/misc/Unsafe.class

if WITH_CLASSPATH_GNU
if ENABLE_ANNOTATIONS
VM_JAVA_FILES += \
	$(top_srcdir)/src/lib/gnu/sun/reflect/annotation/TypeNotPresentExceptionProxy.java \
	$(top_srcdir)/src/lib/gnu/sun/reflect/annotation/AnnotationTypeMismatchExceptionProxy.java \
	$(top_srcdir)/src/lib/gnu/sun/reflect/ConstantPool.java \
	$(top_srcdir)/src/lib/gnu/sun/reflect/annotation/AnnotationType.java \
	$(top_srcdir)/src/lib/gnu/sun/reflect/annotation/AnnotationParser.java

VM_CLASS_FILES += \
	classes/sun/reflect/annotation/TypeNotPresentExceptionProxy.class \
	classes/sun/reflect/annotation/AnnotationTypeMismatchExceptionProxy.class \
	classes/sun/reflect/ConstantPool.class \
	classes/sun/reflect/annotation/AnnotationType.class \
	classes/sun/reflect/annotation/AnnotationParser.class
endif
endif


if ENABLE_ZLIB
pkgdata_DATA = vm.zip

VM_ZIP = ../vm.zip

vm.zip: $(VM_JAVA_FILES)
	$(mkdir_p) classes
	$(JAVAC) -source 1.5 -target 1.5 -d classes $(VM_JAVA_FILES)
	@if test "$(JAR)" = "zip" -o "$(JAR)" = "zip.exe"; then \
	    cd classes && $(JAR) -r -D $(VM_ZIP) .; \
	else \
	    cd classes && $(JAR) cvf $(VM_ZIP) .; \
	fi
else
pkgdata_DATA = nozip

nozip: $(VM_JAVA_FILES)
	$(mkdir_p) classes
	$(JAVAC) -d classes $(VM_JAVA_FILES)
endif

clean-local:
	-rm -rf classes


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
