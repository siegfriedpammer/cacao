dnl Process this file with autoconf to produce a configure script.

AC_INIT(cacao, 0.95+cvs, cacao@cacaojvm.org)
AC_CONFIG_SRCDIR(src/cacao/cacao.c)
AC_CANONICAL_HOST
AC_PREREQ(2.59)
AM_INIT_AUTOMAKE([1.9.0])
AM_MAINTAINER_MODE

AM_CONFIG_HEADER([config.h])

AC_PREFIX_DEFAULT(/usr/local/cacao)


dnl set optimization and debugging for all architectures and systems
if test x"$CFLAGS" = "x"; then
    CFLAGS="-O0 -g3"
fi

dnl system type
case "$host_cpu" in
alpha | alphaev56 | alphapca56 )
    ARCH_DIR="alpha"
    CFLAGS="$CFLAGS -mieee -D__ALPHA__"
    ;;

arm | armv4tl | armv5b )
    ARCH_DIR="arm"
    CFLAGS="$CFLAGS -D__ARM__"
    ;;

i386 | i486 | i586 | i686 )
    ARCH_DIR="i386"
    CFLAGS="$CFLAGS -D__I386__"
    ;;

mips )
    ARCH_DIR="mips"
    CFLAGS="$CFLAGS -D__MIPS__"
    ;;

powerpc )
    ARCH_DIR="powerpc"
    CFLAGS="$CFLAGS -D__POWERPC__"
    ;;

powerpc64 )
    ARCH_DIR="powerpc64"
    CFLAGS="$CFLAGS -D__POWERPC64__"
    ;;

x86_64 )
    ARCH_DIR="x86_64"
    CFLAGS="$CFLAGS -D__X86_64__"
    ;;

xdspcore )
    ARCH_DIR="xdspcore"
    ENABLE_STATICVM="yes"
    CFLAGS="$CFLAGS -D__XDSPCORE__"
    USE_SCHEDULER="1"
    ;;

* )
    AC_MSG_ERROR($host_cpu systems are not supported at this time)
    ;;
esac

dnl host type
case "$host_os" in
*darwin* )
    OS_DIR="darwin"
    CFLAGS="$CFLAGS -D__DARWIN__ -ansi -pedantic -Wall -Wno-long-long"
    ;;

*freebsd* )
    OS_DIR="freebsd"
    CFLAGS="$CFLAGS -D__FREEBSD__ -ansi -pedantic -Wall -Wno-long-long -D_XOPEN_SOURCE_EXTENDED"
    ;;

*irix* )
    OS_DIR="irix"
    CFLAGS="$CFLAGS -D__IRIX__"
    ;;

*linux* | *Linux* )
    OS_DIR="linux"
    CFLAGS="$CFLAGS -D__LINUX__ -ansi -pedantic -Wall -Wno-long-long -D_POSIX_C_SOURCE=199506L -D_XOPEN_SOURCE=500 -D_XOPEN_SOURCE_EXTENDED -D_BSD_SOURCE"
    ;;

* )
    ;;
esac


dnl define and substitute some architecture specific variables
AC_DEFINE_UNQUOTED([ARCH_DIR], "${ARCH_DIR}", [architecture directory])
AC_SUBST(ARCH_DIR)
AC_SUBST(OS_DIR)
AC_SUBST(USE_SCHEDULER)


dnl Checks for programs.
AC_PROG_CC
AM_PROG_AS
AC_PROG_LD
AC_PROG_LD_GNU
AC_PROG_RANLIB
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_MKDIR_P

AC_ENABLE_SHARED
AC_DISABLE_STATIC

dnl libtool stuff
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([sys/ioctl.h])

dnl this is for fdlibm
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_HEADERS([sys/config.h])
AC_CHECK_HEADERS([sys/types.h])
AC_EGREP_HEADER(uint32_t, stdint.h, AC_DEFINE(HAVE_INT32_DEFINED, 1, [Define to 1 if you have uint32_t]))
AC_EGREP_HEADER(uint32_t, inttypes.h, AC_DEFINE(HAVE_INT32_DEFINED, 1, [Define to 1 if you have uint32_t]))
AC_EGREP_HEADER(u_int32_t, sys/types.h, AC_DEFINE(HAVE_BSD_INT32_DEFINED, 1, [Define to 1 if you have BSD u_int32_t]))
AC_EGREP_HEADER(u_int32_t, sys/config.h, AC_DEFINE(HAVE_BSD_INT32_DEFINED, 1, [Define to 1 if you have BSD u_int32_t]))

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_ATTRIBUTE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_TYPE_SIGNAL
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([calloc mprotect getpagesize free])
AC_CHECK_FUNCS([getcwd gettimeofday])
AC_CHECK_FUNCS([scandir])
AC_CHECK_FUNCS([mmap])
AC_CHECK_FUNCS([isnan])

dnl Checks for libraries.

dnl Checking sizeof void *
AC_CHECK_SIZEOF(void *)
AC_C_BIGENDIAN


dnl check which ld flags are required to export symbols
if test x"$with_gnu_ld" = "xyes"; then
    EXPORT_SYMBOLS_FLAGS="-Wl,--export-dynamic"
else
    EXPORT_SYMBOLS_FLAGS=""
fi
AC_SUBST(EXPORT_SYMBOLS_FLAGS)


dnl define install prefix
if test "x$prefix" = "xNONE"; then
    CACAO_PREFIX=$ac_default_prefix
else
    CACAO_PREFIX=$prefix
fi
AC_DEFINE_UNQUOTED([CACAO_PREFIX], "${CACAO_PREFIX}", [installation prefix])
AC_SUBST(CACAO_PREFIX)


dnl Features

dnl check arguments

dnl check for JIT compiler
AC_MSG_CHECKING(whether JIT compiler should be compiled)
AC_ARG_ENABLE([jit],
              [AS_HELP_STRING(--disable-jit,disable JIT compiler [[default=yes]])],
              [case "${enableval}" in
                   no) ENABLE_JIT=no;;
                   *) ENABLE_JIT=yes;;
               esac],
              [ENABLE_JIT=yes])
AC_MSG_RESULT(${ENABLE_JIT})
AM_CONDITIONAL([ENABLE_JIT], test x"${ENABLE_JIT}" = "xyes")

if test x"${ENABLE_JIT}" = "xyes"; then
    AC_DEFINE([ENABLE_JIT], 1, [enable JIT compiler])
fi


dnl check if softfloat should be used
AC_MSG_CHECKING(whether softfloat should be used)
AC_ARG_ENABLE([softfloat],
              [AS_HELP_STRING(--enable-softfloat,use softfloat [[default=no]])],
              [case "${enableval}" in
                  yes) ENABLE_SOFTFLOAT=yes;;
                  *) ENABLE_SOFTFLOAT=no;;
               esac],
              [ENABLE_SOFTFLOAT=no])
AC_MSG_RESULT(${ENABLE_SOFTFLOAT})

if test x"${ENABLE_SOFTFLOAT}" = "xyes"; then
    AC_DEFINE([ENABLE_SOFTFLOAT], 1, [enable softfloat])
fi


dnl check for interpreter
AC_ARG_ENABLE([intrp], [AS_HELP_STRING(--enable-intrp,enable interpreter [[default=no]])])

AC_MSG_CHECKING(whether interpreter should be compiled)
if test x"$enable_intrp" = "xyes"; then
    AC_MSG_RESULT(yes)
    AC_DEFINE([ENABLE_INTRP], 1, [enable interpreter])
    AM_CONDITIONAL([ENABLE_INTRP], [true])
    ENABLE_INTRP=yes

    AC_ASM_SKIP16

    dnl check for libffi
    AC_ARG_WITH([ffi],
                [AS_HELP_STRING(--with-ffi,use libffi to call native functions)],
                [WITH_FFI=yes
                 AC_CHECK_HEADERS([ffi.h],, [AC_MSG_ERROR(cannot find ffi.h)])
                 AC_CHECK_LIB(ffi, ffi_call,, [AC_MSG_ERROR(cannot find libffi)])
                 AC_DEFINE([WITH_FFI], 1, [use libffi])],
                [WITH_FFI=no])

    dnl check for libffcall
    AC_ARG_WITH([ffcall],
                [AS_HELP_STRING(--with-ffcall,use libffcall to call native functions)],
                [WITH_FFCALL=yes
                 AC_CHECK_HEADERS([avcall.h],, [AC_MSG_ERROR(cannot find avcall.h)])
                 AC_CHECK_LIB(avcall, __builtin_avcall,, [AC_MSG_ERROR(cannot find libavcall (from libffcall))])
                 AC_DEFINE([WITH_FFCALL], 1, [use libffcall])],
                [WITH_FFCALL=no])

    dnl check for libffi or libffcall
    if test x"${WITH_FFI}" = "xyes" -a x"${WITH_FFCALL}" = "xyes"; then
        AC_ERROR(Enable only libffi or libffcall.)
    fi
    if test x"${WITH_FFI}" = "xno" -a x"${WITH_FFCALL}" = "xno"; then
        AC_ERROR(Enable one of libffi or libffcall.)
    fi

else
    AC_MSG_RESULT(no)
    AM_CONDITIONAL([ENABLE_INTRP], [false])
    ENABLE_INTRP="no"
fi

if test x"${ENABLE_JIT}" = "xno" -a x"${ENABLE_INTRP}" = "xno"; then
    AC_ERROR(You have to enable at least the JIT or the interpreter.)
fi


dnl check for garbage collector usage
AC_MSG_CHECKING(whether GC should be disabled)
AC_ARG_ENABLE([gc],
              [AS_HELP_STRING(--disable-gc,disable garbage collector (for debugging only!) [[default=yes]])],
              [case "${enableval}" in
                   no) DISABLE_GC=yes
                       AC_DEFINE([DISABLE_GC], 1, [disable garbage collector])
                       ;;
                   *) DISABLE_GC=no;;
               esac],
              [DISABLE_GC=no])
AC_MSG_RESULT(${DISABLE_GC})
AM_CONDITIONAL([DISABLE_GC], test x"${DISABLE_GC}" = "xyes")


dnl check for dump memory usage
AC_MSG_CHECKING(whether dump memory should be disabled)
AC_ARG_ENABLE([dump],
              [AS_HELP_STRING(--disable-dump,disable dump memory (for debugging only!) [[default=yes]])],
              [case "${enableval}" in
                   no)
                       DISABLE_DUMP=yes
                       AC_DEFINE([DISABLE_DUMP], 1, [disable dump memory])
                       ;;
                   *) DISABLE_DUMP=no;;
               esac],
               [DISABLE_DUMP=no])
AC_MSG_RESULT(${DISABLE_DUMP})


dnl check for debug
AC_MSG_CHECKING(whether debug code generation should be enabled)
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING(--disable-debug,disable debug code generation [[default=yes]])],
              [case "${enableval}" in
                   no) NDEBUG=yes
                       AC_DEFINE([NDEBUG], 1, [disable debug code])
                       ;;
                   *) NDEBUG=no;;
               esac],
              [NDEBUG=no])

if test x"${NDEBUG}" = "xno"; then
    AC_MSG_RESULT(yes)

    dnl check for binutils headers and libraries on some architectures for the
    dnl disassembler
    case "${ARCH_DIR}" in
        arm | i386 | powerpc | x86_64 )
            AC_CHECK_HEADER([ansidecl.h],, [AC_MSG_ERROR(cannot find ansidecl.h)])
            AC_CHECK_HEADER([symcat.h],, [AC_MSG_ERROR(cannot find symcat.h)])
            AC_CHECK_HEADER([bfd.h],, [AC_MSG_ERROR(cannot find bfd.h)])
            AC_CHECK_HEADER([dis-asm.h],, [AC_MSG_ERROR(cannot find dis-asm.h)])

            case "${OS_DIR}" in
                cygwin | darwin )
                     AC_CHECK_LIB(intl, dcgettext,, [AC_MSG_ERROR(cannot find libintl (from bintuils))])
                     ;;
            esac

            AC_CHECK_LIB(iberty, xstrerror,, [AC_MSG_ERROR(cannot find libiberty (from bintuils))])
            AC_CHECK_LIB(bfd, bfd_get_arch,, [AC_MSG_ERROR(cannot find libbfd (from bintuils))])
            AC_CHECK_LIB(opcodes, init_disassemble_info,, [AC_MSG_ERROR(cannot find libopcodes (from bintuils))])
            AC_DEFINE([WITH_BINUTILS_DISASSEMBLER], 1, [use binutils disassembler])
            AM_CONDITIONAL([WITH_BINUTILS_DISASSEMBLER], [true])
            ;;
        * )
            AM_CONDITIONAL([WITH_BINUTILS_DISASSEMBLER], [false])
            ;;
    esac

else
    AC_MSG_RESULT(no)
    AM_CONDITIONAL([WITH_BINUTILS_DISASSEMBLER], [false])
fi
AM_CONDITIONAL([NDEBUG], test x"${NDEBUG}" = "xyes")


dnl check for statistics
AC_MSG_CHECKING(whether statistics generation should be enabled)
AC_ARG_ENABLE([statistics],
              [AS_HELP_STRING(--disable-statistics,disable statistics generation [[default=yes]])],
              [case "${enableval}" in
                   no) ENABLE_STATISTICS=no;;
                   *) ENABLE_STATISTICS=yes;;
               esac],
              [ENABLE_STATISTICS=yes])
AC_MSG_RESULT(${ENABLE_STATISTICS})
AM_CONDITIONAL([ENABLE_STATISTICS], test x"${ENABLE_STATISTICS}" = "xyes")

if test x"${ENABLE_STATISTICS}" = "xyes"; then
    AC_DEFINE([ENABLE_STATISTICS], 1, [enable statistics])
fi


dnl check for JVMTI
AC_MSG_CHECKING(whether to compile JVMTI support)
AC_ARG_ENABLE([jvmti],
              [AS_HELP_STRING(--enable-jvmti,enable JVMTI [[default=no]])],
              [case "${enableval}" in
                   yes) ENABLE_JVMTI=yes;;
                   *) ENABLE_JVMTI=no;;
               esac],
              [ENABLE_JVMTI=no])
AC_MSG_RESULT(${ENABLE_JVMTI})
AM_CONDITIONAL([ENABLE_JVMTI], test x"${ENABLE_JVMTI}" = "xyes")

if test x"${ENABLE_JVMTI}" = "xyes"; then
    AC_DEFINE([ENABLE_JVMTI], 1, [use JVMTI])
fi


dnl threads
AC_ARG_ENABLE([threads], [AS_HELP_STRING(--enable-threads,enable threads support (none,green,native) [[default=native]])])
AC_ARG_ENABLE([__thread], [AS_HELP_STRING(--enable-__thread,use TLS features)], [use__thread=$enableval], [use__thread=no])

AC_MSG_CHECKING(whether to include threads support)
enable_threads=${enable_threads:-native}
case "$enable_threads" in
no | none | single)
	AC_MSG_RESULT(no)
	dnl no threads for boehm
	ac_configure_args="$ac_configure_args --disable-boehm-threads"
	AM_CONDITIONAL([USE_THREADS], [false])
	AM_CONDITIONAL([NATIVE_THREADS], [false])
	;;

green | native | posix | pthreads)
	AC_DEFINE([USE_THREADS], 1, [use threads])

	AM_CONDITIONAL([USE_THREADS], [true])

	if test x"$enable_threads" != "xgreen"; then
		AC_MSG_RESULT(yes, native)
                AC_CHECK_LIB(pthread, main)
		AC_DEFINE([NATIVE_THREADS], 1, [use pthreads])

		CFLAGS="$CFLAGS -D_REENTRANT"

		dnl tell boehm to support threads as well
		ac_configure_args="$ac_configure_args --enable-boehm-threads=posix"
		AM_CONDITIONAL([NATIVE_THREADS], [true])

	else
		AC_MSG_RESULT(yes, green)

		dnl no threads for boehm
		ac_configure_args="$ac_configure_args --disable-boehm-threads"
		AM_CONDITIONAL([NATIVE_THREADS], [false])
	fi
    ;;
*)
    AC_MSG_ERROR($enable_threads is an unknown thread package)
    ;;
esac


dnl Check whether the compiler supports the __thread keyword.
if test "x$use__thread" != xno; then
  AC_CACHE_CHECK([for __thread], ac_cv_gcc___thread,
  [cat > conftest.c <<\EOF
__thread int a = 42;
EOF
  if AC_TRY_COMMAND([${CC-cc} $CFLAGS -c conftest.c >&AS_MESSAGE_LOG_FD]); then
    ac_cv_gcc___thread=yes 
  else
    ac_cv_gcc___thread=no
  fi
  rm -f conftest*])
  if test "$ac_cv_gcc___thread" = yes; then
    AC_DEFINE([HAVE___THREAD], 1, [have __thread])
  fi
else
  ac_cv_gcc___thread=no
fi


dnl check if if-conversion should be supported
AC_MSG_CHECKING(whether if-conversion should be supported)
AC_ARG_ENABLE([ifconv],
              [AS_HELP_STRING(--disable-ifconv,disable if-conversion [[default=yes]])],
              [case "${enableval}" in
                   no) ENABLE_IFCONV=no;;
                   *) ENABLE_IFCONV=yes;;
               esac],
              [ENABLE_IFCONV=yes])
AC_MSG_RESULT(${ENABLE_IFCONV})
AM_CONDITIONAL([ENABLE_IFCONV], test x"${ENABLE_IFCONV}" = "xyes")
   
if test x"${ENABLE_IFCONV}" = "xyes"; then
    AC_DEFINE([ENABLE_IFCONV], 1, [enable if-conversion])
fi


dnl check if linear scan register allocator(lsra) should be used
AC_MSG_CHECKING(whether lsra should be supported)
AC_ARG_ENABLE([lsra],
              [AS_HELP_STRING(--disable-lsra,disable linear scan register allocator [[default=yes]])],
              [case "${enableval}" in
                   no) ENABLE_LSRA=no;;
                   *) ENABLE_LSRA=yes;;
               esac],
              [ENABLE_LSRA=yes])
AC_MSG_RESULT(${ENABLE_LSRA})
AM_CONDITIONAL([ENABLE_LSRA], test x"${ENABLE_LSRA}" = "xyes")
   
if test x"${ENABLE_LSRA}" = "xyes"; then
    AC_DEFINE([ENABLE_LSRA], 1, [enable lsra])
fi


dnl check for inlining
dnl AC_ARG_ENABLE([inlining], [AS_HELP_STRING(--disable-inlining,disable method inlining [[default=yes]])])

dnl AC_MSG_CHECKING(whether method inlining should be disabled)
dnl if test x"$enable_inlining" = "xno"; then
dnl	AC_MSG_RESULT(yes)
dnl	AM_CONDITIONAL([USE_INLINING], [false])
dnl else
dnl	AC_MSG_RESULT(no)
	AC_DEFINE([USE_INLINING], 1, [use method inlining])
	AM_CONDITIONAL([USE_INLINING], [true])
dnl fi


dnl check for instruction scheduler
AC_ARG_ENABLE([scheduler], [AS_HELP_STRING(--enable-scheduler,enable instruction scheduler [[default=no]])])

AC_MSG_CHECKING(whether instruction scheduler should be supported)
if test x"${USE_SCHEDULER}" = "x1" -o x"$enable_scheduler" = "xyes"; then
	case "${ARCH_DIR}" in
		xdspcore )
			;;
		* )
			AC_MSG_ERROR(instruction scheduler is not available for ${ARCH_DIR} code generator)
			;;
	esac

	AC_MSG_RESULT(yes)
	AC_DEFINE([USE_SCHEDULER], 1, [use scheduler])
	AM_CONDITIONAL([USE_SCHEDULER], [true])
else
	AC_MSG_RESULT(no)
	AM_CONDITIONAL([USE_SCHEDULER], [false])
fi


dnl check if zlib should be used
AC_MSG_CHECKING(whether ZIP/JAR archives should be supported)
AC_ARG_ENABLE([zlib],
              [AS_HELP_STRING(--disable-zlib,disable ZIP/JAR archive support (needs zlib) [[default=yes]])],
              [case "${enableval}" in
                  no) ENABLE_ZLIB=no;;
                  *) ENABLE_ZLIB=yes;;
               esac],
              [ENABLE_ZLIB=yes])
AC_MSG_RESULT(${ENABLE_ZLIB})

if test x"${ENABLE_ZLIB}" = "xyes"; then
    AC_CHECK_HEADERS([zconf.h],, [AC_MSG_ERROR(cannot find zconf.h)])
    AC_CHECK_HEADERS([zlib.h],, [AC_MSG_ERROR(cannot find zlib.h)])
    AC_CHECK_LIB(z, inflate,, [AC_MSG_ERROR(cannot find libz)])
    AC_DEFINE([ENABLE_ZLIB], 1, [use zlib])
    VM_ZIP_STRING=vm.zip
else
    VM_ZIP_STRING=classes/
fi
AC_DEFINE_UNQUOTED(VM_ZIP_STRING, "${VM_ZIP_STRING}", [define to the string base name of the VM zip file])
AC_SUBST(VM_ZIP_STRING)
AM_CONDITIONAL([ENABLE_ZLIB], test x"${ENABLE_ZLIB}" = "xyes")


dnl check if CACAO should be linked statically
AC_MSG_CHECKING(whether to link CACAO statically)
AC_ARG_ENABLE([staticvm],
              [AS_HELP_STRING(--enable-staticvm,link CACAO statically [[default=no]])],
              [case "${enableval}" in
                  yes) ENABLE_STATICVM=yes;;
                  *) ENABLE_STATICVM=no;;
               esac],
              [ENABLE_STATICVM=no])
AC_MSG_RESULT(${ENABLE_STATICVM})

if test x"${ENABLE_STATICVM}" = "xyes"; then
    AC_DEFINE([ENABLE_STATICVM], 1, [link CACAO statically])
fi
AM_CONDITIONAL([ENABLE_STATICVM], test x"${ENABLE_STATICVM}" = "xyes")
AC_SUBST(ENABLE_STATICVM)


dnl is GNU classpath installed somewhere different
AC_MSG_CHECKING(where GNU Classpath is installed)
AC_ARG_WITH([classpath-prefix],
            [AS_HELP_STRING(--with-classpath-prefix=<dir>,installation directory of GNU Classpath [[default=/usr/local/classpath]])],
            [CLASSPATH_PREFIX=${withval}],
            [CLASSPATH_PREFIX=/usr/local/classpath])
AC_MSG_RESULT(${CLASSPATH_PREFIX})
AC_DEFINE_UNQUOTED([CLASSPATH_PREFIX], "${CLASSPATH_PREFIX}", [GNU Classpath installation directory])
AC_SUBST(CLASSPATH_PREFIX)

AC_MSG_CHECKING(where GNU Classpath libraries are installed)
AC_ARG_WITH([classpath-libdir],
            [AS_HELP_STRING(--with-classpath-libdir=<dir>,installation directory of GNU Classpath libraries [[default=/usr/local/classpath/lib]])],
            [CLASSPATH_LIBDIR=${withval}],
            [CLASSPATH_LIBDIR=${CLASSPATH_PREFIX}/lib])
AC_MSG_RESULT(${CLASSPATH_LIBDIR})

dnl expand CLASSPATH_LIBDIR to something that is usable in C code
AS_AC_EXPAND([CLASSPATH_LIBDIR], ${CLASSPATH_LIBDIR})
AC_DEFINE_UNQUOTED([CLASSPATH_LIBDIR], "${CLASSPATH_LIBDIR}", [GNU Classpath libraries installation directory])
AC_SUBST(CLASSPATH_LIBDIR)

AC_CHECK_HEADER([${CLASSPATH_PREFIX}/include/jni.h],
                [AC_DEFINE_UNQUOTED([CLASSPATH_JNI_H], "${CLASSPATH_PREFIX}/include/jni.h", [GNU Classpath jni.h header])],
                [AC_MSG_ERROR(cannot find jni.h)])


dnl check if GNU Classpath should be linked statically into CACAO
AC_MSG_CHECKING(whether to link GNU Classpath native libraries statically into CACAO)
AC_ARG_WITH([static-classpath],
            [AS_HELP_STRING(--with-static-classpath,link GNU classpath native libraries statically into CACAO [[default=no]])],
            [case "${enableval}" in
                yes) WITH_STATIC_CLASSPATH=yes;;
                *) WITH_STATIC_CLASSPATH=no;;
             esac],
            [WITH_STATIC_CLASSPATH=no])
AC_MSG_RESULT(${WITH_STATIC_CLASSPATH})

if test x"${WITH_STATIC_CLASSPATH}" = "xyes"; then
    AC_DEFINE([WITH_STATIC_CLASSPATH], 1, [link GNU classpath native libraries statically into CACAO])

    dnl check for some classpath related stuff when we link statically
    AM_ICONV_LINK
else
    AC_CHECK_HEADERS([ltdl.h],, [AC_MSG_ERROR(cannot find ltdl.h)])
    AC_CHECK_LIB(ltdl, lt_dlopen,, [AC_MSG_ERROR(cannot find libltdl)])
fi
AM_CONDITIONAL([WITH_STATIC_CLASSPATH], test x"${WITH_STATIC_CLASSPATH}" = "xyes")
AC_SUBST(WITH_STATIC_CLASSPATH)


dnl Maybe the user has Classpath installed 'flat'.
AC_ARG_WITH([flat-classpath], [AS_HELP_STRING(--with-flat-classpath, GNU Classpath is not zipped in the install directory)])
if test x"$with_flat_classpath" = "x"; then
	GLIBJ_ZIP_STRING=glibj.zip
else
	GLIBJ_ZIP_STRING=
fi
AC_SUBST(GLIBJ_ZIP_STRING)
AC_DEFINE_UNQUOTED(GLIBJ_ZIP_STRING, "$GLIBJ_ZIP_STRING", [define to the string base name of the classpath zip file])


dnl check for some programs we need
AC_PROG_JAVAC
AC_PROG_JAR


dnl now configure subpackages
dnl the export is not my favorite (is there another way?)
export CFLAGS
AC_CONFIG_SUBDIRS(src/boehm-gc)

AC_CONFIG_FILES([Makefile]
		[contrib/Makefile]
		[contrib/setenvinstalled]
		[contrib/setenvsource]
		[doc/Makefile]
		[doc/handbook/Makefile]
		[man/Makefile]
		[src/Makefile]
		[src/cacao/Makefile]
		[src/cacaoh/Makefile]
		[src/fdlibm/Makefile]
		[src/lib/Makefile]
		[src/mm/Makefile]
		[src/native/Makefile]
		[src/native/include/Makefile]
		[src/native/jvmti/Makefile]
		[src/native/tools/Makefile]
		[src/native/vm/Makefile]
		[src/scripts/Makefile]
		[src/scripts/java]
		[src/scripts/rmiregistry]
		[src/threads/Makefile]
		[src/threads/green/Makefile]
		[src/threads/native/Makefile]
		[src/toolbox/Makefile]
		[src/vm/Makefile]
		[src/vm/jit/Makefile]
		[src/vm/jit/allocator/Makefile]
		[src/vm/jit/alpha/Makefile]
		[src/vm/jit/alpha/freebsd/Makefile]
		[src/vm/jit/alpha/linux/Makefile]
		[src/vm/jit/i386/Makefile]
		[src/vm/jit/i386/freebsd/Makefile]
		[src/vm/jit/i386/linux/Makefile]
		[src/vm/jit/ifconv/Makefile]
		[src/vm/jit/intrp/Makefile]
		[src/vm/jit/inline/Makefile]
		[src/vm/jit/loop/Makefile]
		[src/vm/jit/mips/Makefile]
		[src/vm/jit/mips/irix/Makefile]
		[src/vm/jit/mips/linux/Makefile]
		[src/vm/jit/powerpc/Makefile]
		[src/vm/jit/powerpc/darwin/Makefile]
		[src/vm/jit/powerpc/linux/Makefile]
		[src/vm/jit/profile/Makefile]
		[src/vm/jit/schedule/Makefile]
		[src/vm/jit/tools/Makefile]
		[src/vm/jit/verify/Makefile]
		[src/vm/jit/x86_64/Makefile]
		[tests/Makefile]
		[tests/regression/Makefile]
		[tests/regression/codepatching/Makefile]
		[tests/regression/native/Makefile])

AC_OUTPUT


## Local variables:
## mode: m4
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
