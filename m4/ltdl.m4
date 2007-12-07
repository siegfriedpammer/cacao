dnl m4/ltdl.m4
dnl
dnl Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
dnl C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
dnl E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
dnl J. Wenninger, Institut f. Computersprachen - TU Wien
dnl 
dnl This file is part of CACAO.
dnl 
dnl This program is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU General Public License as
dnl published by the Free Software Foundation; either version 2, or (at
dnl your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
dnl 02110-1301, USA.


dnl check if ltdl should be used

AC_DEFUN([AC_CHECK_ENABLE_LTDL],[
AC_MSG_CHECKING(whether ltdl should be used)
AC_ARG_ENABLE([ltdl],
              [AS_HELP_STRING(--disable-ltdl,disable ltdl support [[default=enabled]])],
              [case "${enableval}" in
                  no)
                      ENABLE_LTDL=no
                      ;;
                  *)
                      ENABLE_LTDL=yes
                      ;;
               esac],
              [ENABLE_LTDL=yes])
AC_MSG_RESULT(${ENABLE_LTDL})

if test x"${ENABLE_LTDL}" = "xyes"; then
    dnl we need this check for --enable-staticvm, otherwise ltdl can't find dlopen
    if test x"${ENABLE_STATICVM}" = "xyes"; then
        AC_CHECK_LIB(dl, dlopen,, [AC_MSG_ERROR(cannot find libdl)])
    fi

    AC_CHECK_HEADERS([ltdl.h],, [AC_MSG_ERROR(cannot find ltdl.h)])
    AC_CHECK_LIB(ltdl, lt_dlopen,, [AC_MSG_ERROR(cannot find libltdl)])
    AC_DEFINE([ENABLE_LTDL], 1, [use ltdl])
fi

AM_CONDITIONAL([ENABLE_LTDL], test x"${ENABLE_LTDL}" = "xyes")
])
