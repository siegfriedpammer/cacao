dnl modified AC_C_INLINE from autoconf/c.m4 by TWISTI

AN_IDENTIFIER([attribute], [AC_C_ATTRIBUTE])
AC_DEFUN([AC_C_ATTRIBUTE],
[AC_CACHE_CHECK([for __attribute__], ac_cv_c_attribute,
[
AC_COMPILE_IFELSE([AC_LANG_SOURCE(
[void foo(void) __attribute__ ((__noreturn__));]
)],
[ac_cv_c_attribute=yes],
[ac_cv_c_attribute=no]
)
])
AH_VERBATIM([attribute],
[/* Define to `__attribute__' to nothing if it's not supported.  */
#ifndef __cplusplus
#undef __attribute__
#endif])
case $ac_cv_c_attribute in
  yes) ;;
  no)
    cat >>confdefs.h <<_ACEOF
#ifndef __cplusplus
#define __attribute__(x)    /* nothing */
#endif
_ACEOF
    ;;
esac
])# AC_C_ATTRIBUTE
