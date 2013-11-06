dnl m4/future.m4
dnl
dnl Copyright (C) 1996-2013
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


dnl check for future library support

AC_DEFUN([AC_CHECK_FUTURE],[
  AC_LANG_PUSH([C++])
  dnl shared_ptr
  AC_CHECK_HEADER([tr1/memory], [
    AC_DEFINE([HAVE_STD_TR1_SHARED_PTR], 1, [std::tr1::shared_ptr is available])
  ])
  AC_CHECK_HEADER([boost/shared_ptr.hpp], [
    AC_DEFINE([HAVE_BOOST_SHARED_PTR], 1, [boost::shared_ptr is available])
  ])
  dnl unordered set
  AC_CHECK_HEADER([tr1/unordered_set], [
    AC_DEFINE([HAVE_STD_TR1_UNORDERED_SET], 1, [std::tr1::unordered_set is available])
  ])
  AC_CHECK_HEADER([boost/unordered_set.hpp], [
    AC_DEFINE([HAVE_BOOST_UNORDERED_SET], 1, [boost::unordered_set is available])
  ])
  dnl unordered map
  AC_CHECK_HEADER([tr1/unordered_map], [
    AC_DEFINE([HAVE_STD_TR1_UNORDERED_MAP], 1, [std::tr1::unordered_map is available])
  ])
  AC_CHECK_HEADER([boost/unordered_map.hpp], [
    AC_DEFINE([HAVE_BOOST_UNORDERED_MAP], 1, [boost::unordered_map is available])
  ])
  dnl all_of any_of none_of
  AC_MSG_CHECKING([whether standard all_of, any_of, none_of are available])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
  #include <algorithm>
  #include <set>
  bool pred(int x) { return true; }
  ]],[[
  std::set<int> s;
  std::all_of(s.begin(),s.end(),pred);
  std::any_of(s.begin(),s.end(),pred);
  std::none_of(s.begin(),s.end(),pred);
  ]])
  ],[
    AC_MSG_RESULT([yes])
    AC_DEFINE([HAVE_STD_ALL_ANY_NONE_OF], 1, [std::all_of/any_of/none_of is available])
  ], [
    AC_MSG_RESULT([no])
  ])

  AC_CHECK_HEADER([boost/algorithm/cxx11/all_of.hpp], [
  AC_CHECK_HEADER([boost/algorithm/cxx11/any_of.hpp], [
  AC_CHECK_HEADER([boost/algorithm/cxx11/none_of.hpp], [
    AC_DEFINE([HAVE_BOOST_ALL_ANY_NONE_OF], 1, [boost::algorithm::all_of/any_of/none_of is available])
  ])
  ])
  ])
  AC_LANG_POP([C++])
])
