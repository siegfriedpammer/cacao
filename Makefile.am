## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 884 2004-01-15 21:39:01Z jowenn $

@SET_MAKE@

MAINTAINERCLEANFILES = Makefile.in configure
ALLSUBDIRS = mm toolbox threads jit nat tst doc jvmtst
SUBDIRS = gnuclasspathnat $(ALLSUBDIRS)
DIST_SUBDIRS = $(ALLSUBDIRS)

EXTRA_DIST = \
	html/cacaoinstall.html \
	html/cacaoman.html \
	html/index.html \
	main.h \
	natcalls.h \
	nativecalls.h

CLEANFILES = \
	jit/@ARCH_DIR@/offsets.h \
	nativetable.hh \
	nativetypes.hh

bin_PROGRAMS = cacao
noinst_PROGRAMS = cacaoh

INCLUDES = -I$(top_srcdir)/jit/@ARCH_DIR@ -I$(top_srcdir)/jit

BOEHM_LIBS = mm/libboehm.a mm/boehm-gc/.libs/libgc.a

cacao_SOURCES = \
	asmpart.h \
	builtin.c \
	builtin.h \
	global.h \
	loader.c \
	loader.h \
	main.c \
	native.c \
	native.h \
	jni.c \
	jni.h \
	tables.c \
	tables.h \
	unzip.c \
	typeinfo.c \
	typeinfo.h

cacao_LDADD = \
	jit/libjit.a \
	jit/loop/libloop.a \
	jit/@ARCH_DIR@/libarch.a \
	toolbox/libtoolbox.a \
	nat/libnat.a \
	$(BOEHM_LIBS) \
	@THREAD_OBJ@ \
	classpathbin/@ARCH_DIR@/native/jni/java-io/.libs/libjavaio.a \
	classpathbin/@ARCH_DIR@/native/jni/java-nio/.libs/libjavanio.a \
	classpathbin/@ARCH_DIR@/native/jni/java-net/.libs/libjavanet.a \
	classpathbin/@ARCH_DIR@/native/jni/java-util/.libs/libjavautil.a \
	classpathbin/@ARCH_DIR@/native/jni/java-lang/.libs/libjavalang.a \
	classpathbin/@ARCH_DIR@/native/jni/java-lang/.libs/libjavalangreflect.a \
	@AWT_OBJS@ \
	@ZIP_LIBS@


cacao_DEPENDENCIES = \
	jit/libjit.a \
	jit/loop/libloop.a \
	jit/@ARCH_DIR@/libarch.a \
	toolbox/libtoolbox.a \
	nat/libnat.a \
	$(BOEHM_LIBS) \
	@THREAD_OBJ@ \
	classpathbin/@ARCH_DIR@/native/jni/java-io/.libs/libjavaio.a \
	classpathbin/@ARCH_DIR@/native/jni/java-nio/.libs/libjavanio.a \
	classpathbin/@ARCH_DIR@/native/jni/java-net/.libs/libjavanet.a \
	classpathbin/@ARCH_DIR@/native/jni/java-util/.libs/libjavautil.a \
	classpathbin/@ARCH_DIR@/native/jni/java-lang/.libs/libjavalang.a \
	classpathbin/@ARCH_DIR@/native/jni/java-lang/.libs/libjavalangreflect.a \
	@AWT_OBJS@ \
	@ZIP_LIBS@

cacaoh_SOURCES = \
	headers.c \
	tables.c \
	loader.c \
	builtin.c \
	unzip.c

cacaoh_LDADD = \
	toolbox/libtoolbox.a \
	$(BOEHM_LIBS) \
	@THREAD_OBJ@  @ZIP_LIBS@

cacaoh_DEPENDENCIES = \
	toolbox/libtoolbox.a \
	$(BOEHM_LIBS) \
	@THREAD_OBJ@


nativetypes.hh jit/@ARCH_DIR@/offsets.h nativetable.hh: cacaoh Makefile.am Makefile
	test -f stamp-nat || \
	CLASSPATH=./classpathbin/@ARCH_DIR@/lib ./cacaoh \
	java.lang.String \
	java.lang.Class \
	java.io.FileDescriptor \
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
	java.net.PlainSocketImpl \
	java.net.DatagramSocketImpl \
	java.net.InetAddress \
	java.net.DatagramPacket \
	java.net.PlainDatagramSocketImpl \
	java.nio.CharBufferImpl \
	java.nio.channels.FileChannelImpl \
	java.nio.DoubleBufferImpl \
	java.nio.LongBufferImpl \
	java.nio.ByteOrder \
	java.nio.ShortBufferImpl \
	java.nio.IntBufferImpl \
	java.nio.DirectByteBufferImpl \
	java.nio.FloatBufferImpl \
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
	java.security.cert.Certificate \
	java.security.cert.X509Certificate \
	java.security.AccessController \
	javax.swing.Popup \
	javax.swing.UIManager \
	java.lang.VMClass \
	java.lang.reflect.Field \
	java.lang.reflect.Constructor \
	java.lang.reflect.Method \
	java.lang.ThreadGroup \
	java.lang.Thread \
	java.lang.VMDouble \
	java.lang.VMFloat \
	java.lang.Cloneable \
	java.lang.VMObject \
	java.lang.VMClassLoader \
	java.io.PrintStream \
	java.lang.VMSystem \
	java.util.Properties \
	java.lang.Runtime \
	java.lang.Throwable \
	gnu.java.security.x509.X509Certificate  \
	java.lang.JOWENNTest1 \
	@AWTPEERS@
	touch stamp-nat nativetypes.hh jit/@ARCH_DIR@/offsets.h nativetable.hh

native.c: nativetypes.hh jit/@ARCH_DIR@/offsets.h nativetable.hh

checkjvm:
	$(MAKE) -C jvmtst $@

clean:
	rm -f stamp-nat


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
