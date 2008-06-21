dnl m4/vmlog.m4
dnl
dnl Copyright (C) 2008
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


dnl check for vmlog support

AC_DEFUN([AC_CHECK_ENABLE_VMLOG],[
AC_MSG_CHECKING(whether vmlog tracing should be enabled)
AC_ARG_ENABLE([vmlog],
              [AS_HELP_STRING(--enable-vmlog,enable vmlog tracing [[default=no]])],
              [case "${enableval}" in
                   yes) ENABLE_VMLOG=yes;;
                   *) ENABLE_VMLOG=no;;
               esac],
              [ENABLE_VMLOG=no])
AC_MSG_RESULT(${ENABLE_VMLOG})
AM_CONDITIONAL([ENABLE_VMLOG], test x"${ENABLE_VMLOG}" = "xyes")

if test x"${ENABLE_VMLOG}" = "xyes"; then
    AC_DEFINE([ENABLE_VMLOG], 1, [enable vmlog tracing code])
fi
])
