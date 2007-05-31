dnl m4/ac_pthreads_implementation.m4
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
dnl 
dnl $Id: configure.ac 7228 2007-01-19 01:13:48Z edwin $


dnl checks if the pthreads implementation is NPTL or LinuxThreads

AC_DEFUN([AC_CHECK_PTHREADS_IMPLEMENTATION],
[
  AC_MSG_CHECKING(pthread implementation)
  AC_TRY_RUN(
[
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
  char *pathbuf;
  size_t n;

  /* _CS_GNU_LIBPTHREAD_VERSION (GNU C library only; since glibc 2.3.2) */
  /* If the glibc is a pre-2.3.2 version, the compilation fails and we
     fall back to linuxthreads. */

  n = confstr(_CS_GNU_LIBPTHREAD_VERSION, NULL, (size_t) 0);

  pathbuf = malloc(n);
  if ((pathbuf = malloc(n)) == NULL) abort();
  (void) confstr(_CS_GNU_LIBPTHREAD_VERSION, pathbuf, n);

  if (strstr(pathbuf, "linuxthreads") != NULL)
     return 1;

  return 0;
}
],
[
  AC_MSG_RESULT(NPTL)
  AC_DEFINE(PTHREADS_IS_NPTL, 1, [NPTL pthreads implementation])
],
[
  AC_MSG_RESULT(linuxthreads)
  AC_DEFINE(PTHREADS_IS_LINUXTHREADS, 1, [LinuxThreads pthread implementation])
])
])
