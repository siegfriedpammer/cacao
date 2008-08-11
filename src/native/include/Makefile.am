## src/native/include/Makefile.am
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


JAVAH    = javah
JAVAHCMD = $(JAVAH) -jni -bootclasspath $(BOOTCLASSPATH)

if WITH_JAVA_RUNTIME_LIBRARY_CLDC1_1
IMPLEMENTED_VM_CLASSES_HEADER_FILES = \
	com_sun_cldc_io_ResourceInputStream.h \
	com_sun_cldc_io_j2me_socket_Protocol.h \
	com_sun_cldchi_io_ConsoleOutputStream.h \
	com_sun_cldchi_jvm_FileDescriptor.h \
	com_sun_cldchi_jvm_JVM.h \
	java_lang_Class.h \
	java_lang_Math.h \
	java_lang_Runtime.h \
	java_lang_System.h
endif

if WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH
IMPLEMENTED_VM_CLASSES_HEADER_FILES = \
	gnu_classpath_VMStackWalker.h \
	gnu_classpath_VMSystemProperties.h \
	gnu_java_lang_VMCPStringBuilder.h \
	gnu_java_lang_management_VMClassLoadingMXBeanImpl.h \
	gnu_java_lang_management_VMMemoryMXBeanImpl.h \
	gnu_java_lang_management_VMRuntimeMXBeanImpl.h \
	gnu_java_lang_management_VMThreadMXBeanImpl.h \
	java_lang_VMClass.h \
	java_lang_VMClassLoader.h \
	java_lang_VMObject.h \
	java_lang_VMRuntime.h \
	java_lang_VMString.h \
	java_lang_VMSystem.h \
	java_lang_VMThread.h \
	java_lang_VMThrowable.h \
	java_lang_management_VMManagementFactory.h \
	java_lang_reflect_VMConstructor.h \
	java_lang_reflect_VMField.h \
	java_lang_reflect_VMMethod.h \
	java_lang_reflect_VMProxy.h \
	java_security_VMAccessController.h \
	java_util_concurrent_atomic_AtomicLong.h \
	sun_misc_Unsafe.h

if ENABLE_ANNOTATIONS
IMPLEMENTED_VM_CLASSES_HEADER_FILES += \
	sun_reflect_ConstantPool.h
endif
endif

if WITH_JAVA_RUNTIME_LIBRARY_OPENJDK
IMPLEMENTED_VM_CLASSES_HEADER_FILES = \
	sun_misc_Unsafe.h
endif

if ENABLE_JVMTI
JVMTI_HEADER_FILES = \
	gnu_classpath_jdwp_VMFrame.h \
	gnu_classpath_jdwp_VMMethod.h \
	gnu_classpath_jdwp_VMVirtualMachine.h \
	gnu_classpath_jdwp_event_EventRequest.h \
	gnu_classpath_jdwp_util_VariableTable.h
endif

CLEANFILES = \
	*.h

DO_HEADER_FILES = \
	$(IMPLEMENTED_VM_CLASSES_HEADER_FILES)

if WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH
if ENABLE_ZLIB
VM_ZIP = $(top_builddir)/src/classes/vm.zip
endif
endif

if WITH_JAVA_RUNTIME_LIBRARY_CLDC1_1
if ENABLE_ZLIB
VM_ZIP = $(top_builddir)/src/classes/vm.zip
endif
endif

noinst_DATA = $(DO_HEADER_FILES)

$(DO_HEADER_FILES): $(CACAOH) $(VM_ZIP) $(CLASSPATH_CLASSES)
	@class=`echo $@ | sed -e 's/\.h$$//' -e 's/_/\./g'`; \
	echo "$(JAVAHCMD) -d . $$class"; \
	$(JAVAHCMD) -d . $$class


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
