## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 470 2003-10-04 18:35:15Z stefan $

@SET_MAKE@

MAINTAINERCLEANFILES = Makefile.in configure
ALLSUBDIRS = toolbox mm jit comp nat threads tst doc narray jvmtst
SUBDIRS = $(ALLSUBDIRS) @SYSDEP_DIR@
DIST_SUBDIRS = $(ALLSUBDIRS) alpha mips i386 powerpc x86_64

EXTRA_DIST = html/cacaoinstall.html html/cacaoman.html html/index.html \
				jni.c natcalls.h nativecalls.h

CLEANFILES = @SYSDEP_DIR@/asmpart.o \
             @SYSDEP_DIR@/offsets.h \
	         nativetable.hh \
	         nativetypes.hh

bin_PROGRAMS = cacao
noinst_PROGRAMS = cacaoh

INCLUDES=-I$(top_srcdir)/@SYSDEP_DIR@ -I$(top_srcdir)/jit -I@SYSDEP_DIR@ -I$(top_srcdir)

cacao_SOURCES = \
	asmpart.h \
	builtin.c \
	builtin.h \
	callargs.h \
	global.h \
	jit.c \
	jit.h \
	loader.c \
	loader.h \
	main.c \
	native.c \
	native.h \
	jni.h \
	tables.c \
	tables.h

EXTRA_cacao_SOURCES = \
	compiler.c \
	compiler.h 

cacao_LDADD = \
	@SYSDEP_DIR@/asmpart.o \
	@COMPILER_OBJECTS@ \
	toolbox/libtoolbox.a \
	@GC_OBJ@ \
	@THREAD_OBJ@

cacao_DEPENDENCIES = \
	@SYSDEP_DIR@/asmpart.o \
	@COMPILER_OBJECTS@ \
	toolbox/libtoolbox.a \
	@GC_OBJ@ \
	@THREAD_OBJ@

cacaoh_SOURCES = headers.c tables.c loader.c builtin.c
cacaoh_LDADD = toolbox/libtoolbox.a @GC_OBJ@ @THREAD_OBJ@
cacaoh_DEPENDENCIES = toolbox/libtoolbox.a @GC_OBJ@ @THREAD_OBJ@

native.c: nativetypes.hh @SYSDEP_DIR@/offsets.h nativetable.hh

nativetypes.hh @SYSDEP_DIR@/offsets.h nativetable.hh: cacaoh
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

@SYSDEP_DIR@/asmpart.o: $(top_srcdir)/@SYSDEP_DIR@/@ASMPART@ @SYSDEP_DIR@/offsets.h
	@CC@ $(ASMFLAGS) $(INCLUDES) -c -g -o $@ $<

checkjvm:
	$(MAKE) -C jvmtst $@

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 4
## compile-command: "automake --add-missing"
## End:
