## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1429 2004-11-02 08:58:26Z jowenn $

@SET_MAKE@

MAINTAINERCLEANFILES = Makefile.in configure
ALLSUBDIRS = mm toolbox threads jit nat tst doc jvmtst classpath
SUBDIRS = $(ALLSUBDIRS)
DIST_SUBDIRS = $(ALLSUBDIRS)

EXTRA_DIST = \
	html/cacaoinstall.html \
	html/cacaoman.html \
	html/index.html \
	main.h \
	natcalls.h \
	nativecalls.h

CLEANFILES = \
	nativetable.inc

bin_PROGRAMS = cacao cacaoh
noinst_PROGRAMS = gennativetable genoffsets

INCLUDES = -I$(top_srcdir)/jit/@ARCH_DIR@

BOEHM_LIBS = mm/libboehm.a mm/boehm-gc/.libs/libgc.a

cacao_SOURCES = \
	asmpart.h \
	builtin.c \
	builtin.h \
	exceptions.c \
	exceptions.h \
	global.h \
	jni.c \
	jni.h \
	loader.c \
	loader.h \
	main.c \
	native.c \
	native.h \
	options.c \
	options.h \
	statistics.c \
	statistics.h \
	tables.c \
	tables.h \
	typeinfo.c \
	typeinfo.h \
	unzip.c

cacao_LDADD = \
	jit/libjit.a \
	jit/loop/libloop.a \
	jit/@ARCH_DIR@/libarch.a \
	nat/libnat.a \
	$(BOEHM_LIBS) \
	@THREAD_OBJ@ \
	toolbox/libtoolbox.a \
        classpath/native/fdlibm/.libs/libfdlibm.a \
        @STATIC_CLASSPATH_LIBS@ \
        @DYNAMIC_CLASSPATH_LIBS@ \
	@AWT_OBJS@ \
	@ZIP_LIBS@ \
	@THREAD_LIB@


cacao_DEPENDENCIES = \
	jit/libjit.a \
	jit/loop/libloop.a \
	jit/@ARCH_DIR@/libarch.a \
	nat/libnat.a \
	$(BOEHM_LIBS) \
	@THREAD_OBJ@ \
	toolbox/libtoolbox.a \
        @STATIC_CLASSPATH_LIBS@ \
	nativetable.inc \
	@AWT_OBJS@ \
	@ZIP_LIBS@

cacaoh_SOURCES = \
	builtin.c \
	cacaoh.c \
	exceptions.c \
	headers.c \
	loader.c \
	options.c \
	statistics.c \
	tables.c \
	unzip.c

cacaoh_LDADD = \
	$(BOEHM_LIBS) \
	classpath/native/jni/java-lang/.libs/libjavalang.a \
	@THREAD_OBJ@ \
	@ZIP_LIBS@ \
	toolbox/libtoolbox.a \
	@THREAD_LIB@

cacaoh_DEPENDENCIES = \
	$(BOEHM_LIBS) \
	classpath/native/jni/java-lang/.libs/libjavalang.a \
	@THREAD_OBJ@ \
	@ZIP_LIBS@ \
	toolbox/libtoolbox.a

gennativetable_SOURCES = \
	builtin.c \
	exceptions.c \
	gennativetable.c \
	headers.c \
	loader.c \
	options.c \
	tables.c \
	statistics.c \
	unzip.c

gennativetable_LDADD = \
	$(BOEHM_LIBS) \
	classpath/native/jni/java-lang/.libs/libjavalang.a \
	@THREAD_OBJ@ \
	@ZIP_LIBS@ \
	toolbox/libtoolbox.a \
	@THREAD_LIB@

natheaders: cacaoh
	CLASSPATH=$(top_srcdir)/classpath/lib/bootstrap.zip ./cacaoh $(CLASSES)

nativetable.inc: gennativetable nat/implementednatives.data
	CLASSPATH=$(top_srcdir)/classpath/lib/bootstrap.zip $(top_srcdir)/gennativetable $(CLASSES) > nativetable.inc

native.c: nativetable.inc

CLASSES= \
	java.lang.String \
	java.lang.Class \
	java.io.FileDescriptor \
	java.io.VMFile \
	gnu.java.nio.SelectorImpl \
	gnu.java.nio.FileLockImpl \
	gnu.java.lang.reflect.TypeSignature \
	gnu.java.math.MPN \
	gnu.classpath.RawData \
	gnu.classpath.Configuration \
	java.io.OutputStream \
	java.io.PrintWriter \
	java.io.FileInputStream \
	java.io.File \
	java.lang.SecurityManager \
	java.lang.VMSecurityManager \
	java.lang.reflect.Field \
	java.lang.reflect.Constructor \
	java.lang.reflect.Method \
	java.io.VMObjectStreamClass \
	java.io.ObjectInputStream \
	java.io.RandomAccessFile \
	java.io.Writer \
	java.io.DataInput \
	java.io.DataInputStream \
	java.io.OutputStreamWriter \
	java.io.FileWriter \
	java.net.NetworkInterface \
	java.net.SocketImpl \
	gnu.java.net.PlainSocketImpl \
	java.net.DatagramSocketImpl \
	java.net.InetAddress \
	java.net.DatagramPacket \
	gnu.java.net.PlainDatagramSocketImpl \
	java.nio.CharBufferImpl \
	java.nio.DoubleBufferImpl \
	java.nio.LongBufferImpl \
	java.nio.ByteOrder \
	java.nio.ShortBufferImpl \
	java.nio.IntBufferImpl \
	java.nio.DirectByteBufferImpl \
	java.nio.FloatBufferImpl \
	gnu.java.nio.channels.FileChannelImpl \
	java.sql.Connection \
	java.lang.Math \
	java.lang.StringBuffer \
	java.lang.reflect.Array \
	java.lang.reflect.Modifier \
	java.lang.reflect.Proxy \
	java.lang.Double \
	java.lang.Compiler \
	java.lang.Byte \
	java.lang.Character \
	java.lang.Integer \
	java.lang.Long \
	java.lang.Short \
	java.lang.Boolean \
	java.lang.StackTraceElement \
	java.lang.Float \
	java.lang.Object \
	java.lang.RuntimePermission \
	java.lang.System \
	java.lang.StrictMath \
	java.lang.ClassLoader \
	java.lang.Process \
	java.lang.UnsatisfiedLinkError \
	java.text.Collator \
	java.util.zip.Deflater \
	java.util.zip.Inflater \
	java.util.TimeZone \
	java.util.Locale \
	java.util.Hashtable \
	java.beans.PropertyEditor \
	java.security.AccessController \
	java.security.ProtectionDomain \
	java.security.cert.Certificate \
	java.security.cert.X509Certificate \
	javax.swing.Popup \
	javax.swing.UIManager \
	java.lang.VMClass \
	java.lang.ThreadGroup \
	java.lang.VMThread \
	java.lang.VMDouble \
	java.lang.VMFloat \
	java.lang.Cloneable \
	java.lang.VMObject \
	java.lang.VMClassLoader \
	java.io.PrintStream \
	java.lang.VMSystem \
	java.lang.VMString \
	java.util.Properties \
	java.lang.Runtime \
	java.lang.VMRuntime \
	java.lang.Throwable \
	java.lang.VMThrowable \
	gnu.java.security.x509.X509Certificate  \
	java.lang.JOWENNTest1 \
	@AWTPEERS@

checkjvm:
	$(MAKE) -C jvmtst $@


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
