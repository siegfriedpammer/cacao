## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1622 2004-11-30 13:18:52Z twisti $

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@

SUBDIRS = tools include vm

noinst_LIBRARIES = libnative.a

libnative_a_SOURCES = \
	jni.c \
	native.c

native.c: nativetable.inc

nativetable.inc:
	CLASSPATH=$(top_srcdir)/src/classpath/lib/bootstrap.zip \
	$(top_srcdir)/src/native/tools/gennativetable \
	$(NO_REGEN_CLASSES) \
	$(CLASSES) \
	> nativetable.inc

CLASSES= \
	java.lang.reflect.Field \
	java.lang.SecurityManager \
	java.net.DatagramPacket \
	java.net.InetAddress \
	java.net.SocketImpl \
	java.util.Properties \
	\
	gnu.classpath.Configuration \
	gnu.classpath.RawData \
	gnu.java.lang.reflect.TypeSignature \
	gnu.java.math.MPN \
	gnu.java.net.PlainDatagramSocketImpl \
	gnu.java.net.PlainSocketImpl \
	gnu.java.nio.FileLockImpl \
	gnu.java.nio.SelectorImpl \
	gnu.java.nio.channels.FileChannelImpl \
	gnu.java.security.x509.X509Certificate  \
	java.beans.PropertyEditor \
	java.io.DataInput \
	java.io.DataInputStream \
	java.io.File \
	java.io.FileDescriptor \
	java.io.FileInputStream \
	java.io.FileWriter \
	java.io.ObjectInputStream \
	java.io.OutputStream \
	java.io.OutputStreamWriter \
	java.io.PrintStream \
	java.io.PrintWriter \
	java.io.RandomAccessFile \
	java.io.VMFile \
	java.io.VMObjectStreamClass \
	java.io.Writer \
	java.lang.Boolean \
	java.lang.Byte \
	java.lang.Character \
	java.lang.Class \
	java.lang.Compiler \
	java.lang.Double \
	java.lang.Float \
	java.lang.Integer \
	java.lang.Long \
	java.lang.Math \
	java.lang.Process \
	java.lang.Runtime \
	java.lang.RuntimePermission \
	java.lang.Short \
	java.lang.StackTraceElement \
	java.lang.StrictMath \
	java.lang.StringBuffer \
	java.lang.System \
	java.lang.UnsatisfiedLinkError \
	java.lang.VMClass \
	java.lang.VMClassLoader \
	java.lang.VMDouble \
	java.lang.VMFloat \
	java.lang.VMRuntime \
	java.lang.VMSecurityManager \
	java.lang.VMString \
	java.lang.VMSystem \
	java.lang.VMThrowable \
	java.lang.reflect.Array \
	java.lang.reflect.Constructor \
	java.lang.reflect.Method \
	java.lang.reflect.Modifier \
	java.lang.reflect.Proxy \
	java.net.DatagramSocketImpl \
	java.net.NetworkInterface \
	java.nio.ByteOrder \
	java.nio.CharBufferImpl \
	java.nio.DirectByteBufferImpl \
	java.nio.DoubleBufferImpl \
	java.nio.FloatBufferImpl \
	java.nio.IntBufferImpl \
	java.nio.LongBufferImpl \
	java.nio.ShortBufferImpl \
	java.security.AccessController \
	java.security.ProtectionDomain \
	java.security.cert.Certificate \
	java.security.cert.X509Certificate \
	java.sql.Connection \
	java.text.Collator \
	java.util.Hashtable \
	java.util.Locale \
	java.util.TimeZone \
	java.util.VMTimeZone \
	java.util.zip.Deflater \
	java.util.zip.Inflater \
	javax.swing.Popup \
	javax.swing.UIManager \
	@AWTPEERS@

NO_REGEN_CLASSES = \
	java.lang.ClassLoader \
	java.lang.Cloneable \
	java.lang.Object \
	java.lang.String \
	java.lang.ThreadGroup \
	java.lang.Throwable \
	java.lang.VMObject \
	java.lang.VMThread


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
