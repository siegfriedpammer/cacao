## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 118 1999-01-20 14:58:16Z andi $

MAINTAINERCLEANFILES = Makefile.in configure
SUBDIRS = toolbox mm alpha jit comp nat threads mips

EXTRA_DIST = html/cacaoinstall.html html/cacaoman.html html/index.html \
			 doc/cacao.1

CLEANFILES = @SYSDEP_DIR@/asmpart.o \
			 @SYSDEP_DIR@/asmpart.s \
             @SYSDEP_DIR@/offsets.h \
	         nativetable.hh \
	         nativetypes.hh

bin_PROGRAMS = cacao
noinst_PROGRAMS = cacaoh
man_MANS = doc/cacao.1

INCLUDES=-I/usr/include -I$(top_srcdir)/@SYSDEP_DIR@ -I$(top_srcdir)/jit -I@SYSDEP_DIR@ -I$(top_srcdir)

cacao_SOURCES = \
	asmpart.h \
	builtin.c \
	builtin.h \
	callargs.h \
	@COMPILER_SOURCES@ \
	global.h \
	jit.c \
	jit.h \
	loader.c \
	loader.h \
	main.c \
	native.c \
	native.h \
	tables.c \
	tables.h

EXTRA_cacao_SOURCES = compiler.c compiler.h

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
		java.lang.Class \
		java.lang.ClassLoader \
		java.lang.Compiler \
		java.lang.Double \
		java.lang.Float \
		java.lang.Math \
		java.lang.Runtime \
		java.lang.SecurityManager \
		java.lang.System \
		java.lang.Thread \
		java.lang.ThreadGroup \
		java.lang.Throwable \
		java.io.File \
		java.io.FileDescriptor \
		java.io.FileInputStream \
		java.io.FileOutputStream \
		java.io.PrintStream \
		java.io.RandomAccessFile \
		java.util.Properties \
		java.util.Date

@SYSDEP_DIR@/asmpart.o: $(top_srcdir)/@SYSDEP_DIR@/asmpart.c @SYSDEP_DIR@/offsets.h
	rm -f @SYSDEP_DIR@/asmpart.s
	@CPP@ $(INCLUDES) $(top_srcdir)/@SYSDEP_DIR@/asmpart.c \
			> @SYSDEP_DIR@/asmpart.s
	@CC@ $(INCLUDES) -c -o @SYSDEP_DIR@/asmpart.o @SYSDEP_DIR@/asmpart.s
	rm -f asmpart.s

compiler.o: builtin.h compiler.h global.h loader.h tables.h native.h \
            asmpart.h compiler.c comp/*.c \
            @SYSDEP_DIR@/gen.c @SYSDEP_DIR@/disass.c
	@CC@ $(CFLAGS) $(INCLUDES) -c compiler.c


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 4
## compile-command: "automake --add-missing"
## End:
