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
## $Id: Makefile.am 1984 2005-03-04 16:30:45Z twisti $

## Process this file with automake to produce Makefile.in

CLEANFILES = \
	headers-generated \
	awtheaders-generated \
	\
	gnu_classpath_Configuration.h \
	gnu_classpath_RawData.h \
	gnu_classpath_VMSystemProperties.h \
	\
	gnu_java_awt_image_GdkPixbufDecoder.h \
	gnu_java_awt_peer_gtk_GThreadMutex.h \
	gnu_java_awt_peer_gtk_GThreadNativeMethodRunner.h \
	gnu_java_awt_peer_gtk_GdkFontMetrics.h \
	gnu_java_awt_peer_gtk_GdkFontPeer.h \
	gnu_java_awt_peer_gtk_GdkGraphics.h \
	gnu_java_awt_peer_gtk_GdkGraphicsEnvironment.h \
	gnu_java_awt_peer_gtk_GdkPixbufDecoder.h \
	gnu_java_awt_peer_gtk_GdkTextLayout.h \
	gnu_java_awt_peer_gtk_GtkButtonPeer.h \
	gnu_java_awt_peer_gtk_GtkCanvasPeer.h \
	gnu_java_awt_peer_gtk_GtkCheckboxGroupPeer.h \
	gnu_java_awt_peer_gtk_GtkCheckboxMenuItemPeer.h \
	gnu_java_awt_peer_gtk_GtkCheckboxPeer.h \
	gnu_java_awt_peer_gtk_GtkChoicePeer.h \
	gnu_java_awt_peer_gtk_GtkClipboard.h \
	gnu_java_awt_peer_gtk_GtkComponentPeer.h \
	gnu_java_awt_peer_gtk_GtkContainerPeer.h \
	gnu_java_awt_peer_gtk_GtkDialogPeer.h \
	gnu_java_awt_peer_gtk_GtkEmbeddedWindowPeer.h \
	gnu_java_awt_peer_gtk_GtkFileDialogPeer.h \
	gnu_java_awt_peer_gtk_GtkFramePeer.h \
	gnu_java_awt_peer_gtk_GtkFontPeer.h \
	gnu_java_awt_peer_gtk_GtkGenericPeer.h \
	gnu_java_awt_peer_gtk_GtkImage.h \
	gnu_java_awt_peer_gtk_GtkImagePainter.h \
	gnu_java_awt_peer_gtk_GtkLabelPeer.h \
	gnu_java_awt_peer_gtk_GtkListPeer.h \
	gnu_java_awt_peer_gtk_GtkMenuBarPeer.h \
	gnu_java_awt_peer_gtk_GtkMenuComponentPeer.h \
	gnu_java_awt_peer_gtk_GtkMenuItemPeer.h \
	gnu_java_awt_peer_gtk_GtkMenuPeer.h \
	gnu_java_awt_peer_gtk_GtkOffScreenImage.h \
	gnu_java_awt_peer_gtk_GtkPanelPeer.h \
	gnu_java_awt_peer_gtk_GtkPopupMenuPeer.h \
	gnu_java_awt_peer_gtk_GtkScrollPanePeer.h \
	gnu_java_awt_peer_gtk_GtkScrollbarPeer.h \
	gnu_java_awt_peer_gtk_GtkTextAreaPeer.h \
	gnu_java_awt_peer_gtk_GtkTextComponentPeer.h \
	gnu_java_awt_peer_gtk_GtkTextFieldPeer.h \
	gnu_java_awt_peer_gtk_GtkToolkit.h \
	gnu_java_awt_peer_gtk_GtkWindowPeer.h \
	\
	gnu_java_lang_reflect_TypeSignature.h \
	gnu_java_math_MPN.h \
	gnu_java_net_PlainDatagramSocketImpl.h \
	gnu_java_net_PlainSocketImpl.h \
	gnu_java_nio_FileLockImpl.h \
	gnu_java_nio_SelectorImpl.h \
	gnu_java_nio_channels_FileChannelImpl.h \
	gnu_java_security_x509_X509Certificate.h \
	\
	java_awt_Graphics.h \
	java_awt_Window.h \
	java_awt_peer_ComponentPeer.h \
	java_awt_peer_MenuItemPeer.h \
	java_awt_peer_MenuPeer.h \
	\
	java_beans_PropertyEditor.h \
	java_io_DataInput.h \
	java_io_DataInputStream.h \
	java_io_File.h \
	java_io_FileDescriptor.h \
	java_io_FileInputStream.h \
	java_io_FileWriter.h \
	java_io_ObjectInputStream.h \
	java_io_OutputStream.h \
	java_io_OutputStreamWriter.h \
	java_io_PrintStream.h \
	java_io_PrintWriter.h \
	java_io_RandomAccessFile.h \
	java_io_VMFile.h \
	java_io_VMObjectStreamClass.h \
	java_io_Writer.h \
	java_lang_Boolean.h \
	java_lang_Byte.h \
	java_lang_Character.h \
	java_lang_Class.h \
	java_lang_Compiler.h \
	java_lang_Double.h \
	java_lang_Float.h \
	java_lang_Integer.h \
	java_lang_Long.h \
	java_lang_Math.h \
	java_lang_Process.h \
	java_lang_Runtime.h \
	java_lang_RuntimePermission.h \
	java_lang_SecurityManager.h \
	java_lang_Short.h \
	java_lang_StackTraceElement.h \
	java_lang_StrictMath.h \
	java_lang_StringBuffer.h \
	java_lang_System.h \
	java_lang_UnsatisfiedLinkError.h \
	java_lang_VMClass.h \
	java_lang_VMClassLoader.h \
	java_lang_VMDouble.h \
	java_lang_VMFloat.h \
	java_lang_VMRuntime.h \
	java_lang_VMString.h \
	java_lang_VMSystem.h \
	java_lang_VMThrowable.h \
	java_lang_reflect_Array.h \
	java_lang_reflect_Constructor.h \
	java_lang_reflect_Field.h \
	java_lang_reflect_Method.h \
	java_lang_reflect_Modifier.h \
	java_lang_reflect_Proxy.h \
	java_net_DatagramPacket.h \
	java_net_DatagramSocketImpl.h \
	java_net_InetAddress.h \
	java_net_NetworkInterface.h \
	java_net_PlainDatagramSocketImpl.h \
	java_net_PlainSocketImpl.h \
	java_net_SocketImpl.h \
	java_net_URL.h \
	java_nio_ByteOrder.h \
	java_nio_CharBufferImpl.h \
	java_nio_DirectByteBufferImpl.h \
	java_nio_DoubleBufferImpl.h \
	java_nio_FloatBufferImpl.h \
	java_nio_IntBufferImpl.h \
	java_nio_LongBufferImpl.h \
	java_nio_ShortBufferImpl.h \
	java_nio_channels_FileChannelImpl.h \
	java_security_AccessController.h \
	java_security_ProtectionDomain.h \
	java_security_cert_Certificate.h \
	java_security_cert_X509Certificate.h \
	java_sql_Connection.h \
	java_text_Collator.h \
	java_util_Hashtable.h \
	java_util_Locale.h \
	java_util_Properties.h \
	java_util_TimeZone.h \
	java_util_VMTimeZone.h \
	java_util_zip_Deflater.h \
	java_util_zip_Inflater.h \
	javax_swing_Popup.h \
	javax_swing_UIManager.h

# include NO_REGEN_CLASSES, CLASSES and AWT_CLASSES
include $(srcdir)/../classes.am

noinst_HEADERS = \
	java_lang_ClassLoader.h \
	java_lang_Cloneable.h \
	java_lang_Object.h \
	java_lang_String.h \
	java_lang_Thread.h \
	java_lang_ThreadGroup.h \
	java_lang_Throwable.h \
	java_lang_VMObject.h \
	java_lang_VMThread.h

noinst_DATA = headers-generated

if USE_GTK_PEER
noinst_DATA += awtheaders-generated

awtheaders-generated: $(top_srcdir)/src/classpath/lib/bootstrap.zip $(top_builddir)/config.h
	BOOTCLASSPATH=$(top_builddir)/src/classpath/lib/ \
	$(top_builddir)/src/cacaoh/cacaoh -d . \
	$(AWT_CLASSES)
	touch awtheaders-generated
endif

headers-generated: $(top_srcdir)/src/classpath/lib/bootstrap.zip $(top_builddir)/config.h
	BOOTCLASSPATH=$(top_builddir)/src/classpath/lib/ \
	$(top_builddir)/src/cacaoh/cacaoh -d . \
	$(CLASSES)
	touch headers-generated


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
