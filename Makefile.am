## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 557 2003-11-02 22:51:59Z twisti $

@SET_MAKE@

MAINTAINERCLEANFILES = Makefile.in configure
ALLSUBDIRS = mm toolbox threads jit nat tst doc jvmtst
SUBDIRS = $(ALLSUBDIRS)
DIST_SUBDIRS = $(ALLSUBDIRS)

EXTRA_DIST = \
	html/cacaoinstall.html \
	html/cacaoman.html \
	html/index.html \
	jni.c \
	natcalls.h \
	nativecalls.h

CLEANFILES = \
	jit/@ARCH_DIR@/asmpart.o \
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
	jni.h \
	tables.c \
	tables.h

cacao_LDADD = \
	jit/libjit.a \
	jit/loop/libloop.a \
	jit/@ARCH_DIR@/libarch.a \
	toolbox/libtoolbox.a \
	$(BOEHM_LIBS) \
	@THREAD_OBJ@

cacao_DEPENDENCIES = \
	jit/libjit.a \
	jit/loop/libloop.a \
	jit/@ARCH_DIR@/libarch.a \
	toolbox/libtoolbox.a \
	$(BOEHM_LIBS) \
	@THREAD_OBJ@

cacaoh_SOURCES = \
	headers.c \
	tables.c \
	loader.c \
	builtin.c

cacaoh_LDADD = \
	toolbox/libtoolbox.a \
	$(BOEHM_LIBS) \
	@THREAD_OBJ@

cacaoh_DEPENDENCIES = \
	toolbox/libtoolbox.a \
	$(BOEHM_LIBS) \
	@THREAD_OBJ@

native.c: nativetypes.hh jit/@ARCH_DIR@/offsets.h nativetable.hh

nativetypes.hh jit/@ARCH_DIR@/offsets.h nativetable.hh: cacaoh
	./cacaoh \
		java.lang.Object \
		java.lang.String \
		java.lang.ClassLoader \
		java.lang.Class \
		java.lang.Compiler \
		java.lang.Integer \
		java.lang.Double \
		java.lang.Float \
		java.lang.Math \
		java.lang.Package \
		java.lang.Runtime \
		java.lang.SecurityManager \
		java.lang.Exception \
		java.io.PrintStream \
		java.io.InputStream \
		java.lang.System \
		java.lang.Thread \
		java.lang.ThreadGroup \
		java.lang.Throwable \
		java.lang.reflect.Array \
		java.lang.reflect.Constructor \
		java.lang.reflect.Field \
		java.lang.reflect.Method \
		java.lang.reflect.Modifier \
		java.io.FileDescriptor \
		java.io.FileInputStream \
		java.io.FileOutputStream \
		java.io.FileSystem \
		java.io.ObjectInputStream \
		java.io.ObjectStreamClass \
		java.io.RandomAccessFile \
		java.util.ResourceBundle \
		java.util.jar.JarFile \
		java.util.zip.Adler32 \
		java.util.zip.CRC32 \
		java.util.zip.Deflater \
		java.util.zip.Inflater \
		java.util.zip.ZipEntry \
		java.util.zip.ZipFile \
		java.util.Properties \
		java.util.Date \
		java.math.BigInteger \
		java.net.InetAddress \
		java.net.InetAddressImpl \
		java.net.DatagramPacket \
		java.net.PlainDatagramSocketImpl \
		java.net.SocketImpl \
		java.net.PlainSocketImpl \
		java.net.SocketInputStream \
		java.net.SocketOutputStream \
		java.security.PrivilegedActionException \
		java.security.PrivilegedAction \
		java.security.PrivilegedExceptionAction \
		java.security.AccessController \
		java.io.File \
		java.io.UnixFileSystem \
		java.lang.ClassLoader_NativeLibrary

checkjvm:
	$(MAKE) -C jvmtst $@


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
