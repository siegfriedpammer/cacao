dnl m4/compiler2.m4
dnl
dnl Copyright (C) 2013
dnl CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
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


dnl enable the optimization framework

AC_DEFUN([AC_CHECK_ENABLE_COMPILER2],[
AC_MSG_CHECKING(enable optimization framework)
AC_ARG_ENABLE([compiler2],
              [AS_HELP_STRING(--enable-compiler2,enable new second stage compiler [[default=no]])],
              [case "${enableval}" in
                   yes) ENABLE_COMPILER2=yes;;
                   *) ENABLE_COMPILER2=no;;
               esac],
              [ENABLE_COMPILER2=no])
AC_MSG_RESULT(${ENABLE_COMPILER2})
AM_CONDITIONAL([ENABLE_COMPILER2], test x"${ENABLE_COMPILER2}" = "xyes")

if test x"${ENABLE_COMPILER2}" = "xyes"; then
    AC_DEFINE([ENABLE_COMPILER2], 1, [enable new second stage compiler (compiler2)])
fi
])
