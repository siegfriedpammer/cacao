## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1572 2004-11-23 16:05:40Z twisti $

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

if USE_ZLIB
ZLIB_OBJ = unzip.c
endif

cacao_SOURCES = \
	builtin.c \
	exceptions.c \
	jni.c \
	loader.c \
	main.c \
	native.c \
	options.c \
	statistics.c \
	tables.c \
	typeinfo.c \
	$(ZLIB_OBJ)

cacao_LDADD = \
	jit/libjit.a \
	jit/loop/libloop.a \
	jit/@ARCH_DIR@/libarch.a \
	nat/libnat.a \
	$(BOEHM_LIBS) \
	@THREAD_OBJ@ \
	toolbox/libtoolbox.a \
        @STATIC_CLASSPATH_LIBS@ \
        @DYNAMIC_CLASSPATH_LIBS@ \
	@AWT_OBJS@


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
	@AWT_OBJS@

cacaoh_SOURCES = \
	builtin.c \
	cacaoh.c \
	exceptions.c \
	headers.c \
	loader.c \
	options.c \
	statistics.c \
	tables.c \
	$(ZLIB_OBJ)

cacaoh_LDADD = \
	$(BOEHM_LIBS) \
	classpath/native/jni/java-lang/.libs/libjavalang.a \
	@THREAD_OBJ@ \
	toolbox/libtoolbox.a

cacaoh_DEPENDENCIES = \
	$(BOEHM_LIBS) \
	classpath/native/jni/java-lang/.libs/libjavalang.a \
	@THREAD_OBJ@ \
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
	$(ZLIB_OBJ)

gennativetable_LDADD = \
	$(BOEHM_LIBS) \
	classpath/native/jni/java-lang/.libs/libjavalang.a \
	@THREAD_OBJ@ \
	toolbox/libtoolbox.a

gennativetable_DEPENDENCIES = \
	$(BOEHM_LIBS) \
	classpath/native/jni/java-lang/.libs/libjavalang.a \
	@THREAD_OBJ@ \
	toolbox/libtoolbox.a

all: cacaoh genoffsets gennativetable cacao

mm/libboehm.a:
	make -C mm

classpath/native/jni/java-lang/.libs/libjavalang.a:
	make -C classpath/native/

threads/libthreads.a:
	make -C threads

toolbox/libtoolbox.a:
	make -C toolbox

jit/libjit.a:
	make -C jit

nat/libnat.a:
	make -C nat

natheaders: cacaoh
	CLASSPATH=$(top_srcdir)/classpath/lib/bootstrap.zip \
	./cacaoh -d nat/ \
	$(CLASSES)

nativetable.inc: gennativetable nat/implementednatives.data
	CLASSPATH=$(top_srcdir)/classpath/lib/bootstrap.zip \
	$(top_srcdir)/gennativetable \
	$(NOGEN_CLASSES) \
	$(CLASSES) \
	> nativetable.inc

nat/implementednatives.data:
	make -C nat implementednatives

native.c: nativetable.inc

CLASSES= \
	java.lang.JOWENNTest1 \
	\
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

NOGEN_CLASSES = \
	java.lang.ClassLoader \
	java.lang.Cloneable \
	java.lang.Object \
	java.lang.String \
	java.lang.ThreadGroup \
	java.lang.Throwable \
	java.lang.VMObject \
	java.lang.VMThread

checkjvm:
	$(MAKE) -C jvmtst $@


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
