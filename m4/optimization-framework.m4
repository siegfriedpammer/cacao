dnl m4/optimization-framework.m4
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

AC_DEFUN([AC_CHECK_ENABLE_OPTIMIZATION_FRAMEWORK],[
AC_MSG_CHECKING(enable optimization framework)
AC_ARG_ENABLE([optimization-framework],
              [AS_HELP_STRING(--enable-optimization-framework,enable experimental optimization framework [[default=no]])],
              [case "${enableval}" in
                   yes) ENABLE_OPTIMIZATION_FRAMEWORK=yes;;
                   *) ENABLE_OPTIMIZATION_FRAMEWORK=no;;
               esac],
              [ENABLE_OPTIMIZATION_FRAMEWORK=no])
AC_MSG_RESULT(${ENABLE_OPTIMIZATION_FRAMEWORK})
AM_CONDITIONAL([ENABLE_OPTIMIZATION_FRAMEWORK], test x"${ENABLE_OPTIMIZATION_FRAMEWORK}" = "xyes")

if test x"${ENABLE_OPTIMIZATION_FRAMEWORK}" = "xyes"; then
    AC_DEFINE([ENABLE_OPTIMIZATION_FRAMEWORK], 1, [enable optimization framework])
fi
])
