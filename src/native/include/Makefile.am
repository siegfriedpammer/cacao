## src/native/include/Makefile.am
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
## $Id: Makefile.am 3389 2005-10-07 15:35:38Z twisti $

## Process this file with automake to produce Makefile.in

NO_GEN_HEADER_FILES = \
	java_lang_ClassLoader.h \
	java_lang_Cloneable.h \
	java_lang_Object.h \
	java_lang_String.h \
	java_lang_Thread.h \
	java_lang_ThreadGroup.h \
	java_lang_Throwable.h \
	java_lang_VMObject.h \
	java_lang_VMThread.h

GEN_HEADER_FILES = \
	java_util_Properties.h \
	java_io_InputStream.h \
	java_io_PrintStream.h \
	\
	gnu_classpath_Pointer.h \
	gnu_classpath_Pointer32.h \
	gnu_classpath_Pointer64.h \
	java_io_File.h \
	java_lang_Boolean.h \
	java_lang_Byte.h \
	java_lang_Character.h \
	java_lang_Class.h \
	java_lang_Double.h \
	java_lang_Float.h \
	java_lang_Integer.h \
	java_lang_Long.h \
	java_lang_Process.h \
	java_lang_Short.h \
	java_lang_StackTraceElement.h \
	java_lang_VMThrowable.h \
	java_lang_reflect_Constructor.h \
	java_lang_reflect_Field.h \
	java_lang_reflect_Method.h \
	java_nio_Buffer.h \
	java_nio_DirectByteBufferImpl.h \
	java_security_ProtectionDomain.h \
	java_util_Vector.h

ADDITIONAL_IMPLEMENTED_VM_CLASSES_HEADER_FILES = \
	gnu_classpath_VMStackWalker.h \
	gnu_classpath_VMSystemProperties.h \
	java_lang_VMClass.h \
	java_lang_VMClassLoader.h \
	java_lang_VMRuntime.h \
	java_lang_VMString.h \
	java_lang_VMSystem.h \
	java_lang_reflect_VMProxy.h \
	java_security_VMAccessController.h

ADDITIONAL_STATIC_CLASSPATH_HEADER_FILES = \
	java_net_DatagramPacket.h \
	java_net_InetAddress.h \
	\
	gnu_java_net_PlainDatagramSocketImpl.h \
	gnu_java_net_PlainSocketImpl.h \
	gnu_java_nio_PipeImpl.h \
	gnu_java_nio_channels_FileChannelImpl.h \
	gnu_java_nio_charset_iconv_IconvEncoder.h \
	gnu_java_nio_charset_iconv_IconvDecoder.h \
	java_lang_VMProcess.h \
	java_net_SocketImpl.h \
	java_nio_MappedByteBufferImpl.h \
	java_nio_channels_spi_SelectorProvider.h

CLEANFILES = \
	$(GEN_HEADER_FILES) \
	$(ADDITIONAL_IMPLEMENTED_VM_CLASSES_HEADER_FILES) \
	$(ADDITIONAL_STATIC_CLASSPATH_HEADER_FILES)

noinst_HEADERS = \
	$(NO_GEN_HEADER_FILES)

DO_HEADER_FILES = \
	$(GEN_HEADER_FILES) \
	$(ADDITIONAL_IMPLEMENTED_VM_CLASSES_HEADER_FILES)

if ENABLE_STATICVM
DO_HEADER_FILES += \
	$(ADDITIONAL_STATIC_CLASSPATH_HEADER_FILES)
endif

CACAOH = $(top_builddir)/src/cacaoh/cacaoh

VM_ZIP = $(top_builddir)/src/lib/vm.zip
GLIBJ_ZIP = $(CLASSPATH_INSTALL_DIR)/share/classpath/glibj.zip

CLASSPATH = $(VM_ZIP):$(GLIBJ_ZIP)

all: $(DO_HEADER_FILES)

$(NO_GEN_HEADER_FILES):
	@class=`echo $@ | sed -e 's/\.h$$//' -e 's/_/\./g'`; \
	echo "$(CACAOH) -bootclasspath $(CLASSPATH) -d . $$class"; \
	$(CACAOH) -bootclasspath $(CLASSPATH) -d . $$class

$(DO_HEADER_FILES): $(VM_ZIP) $(GLIBJ_ZIP)
	@class=`echo $@ | sed -e 's/\.h$$//' -e 's/_/\./g'`; \
	echo "$(CACAOH) -bootclasspath $(CLASSPATH) -d . $$class"; \
	$(CACAOH) -bootclasspath $(CLASSPATH) -d . $$class


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
