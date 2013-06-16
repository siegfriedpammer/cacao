/* src/vm/types.hpp - type definitions for CACAO's internal types

   Copyright (C) 1996-2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#ifndef CACAO_TYPES_HPP_
#define CACAO_TYPES_HPP_ 1

#include "config.h"

#include <stdint.h>
#include <climits>

/* In this file we check for unknown pointersizes, so we don't have to
   do this somewhere else. */

/* Define the sizes of the integer types used internally by CACAO. ************/

typedef int8_t            s1;
typedef uint8_t           u1;

typedef int16_t           s2;
typedef uint16_t          u2;

typedef int32_t           s4;
typedef uint32_t          u4;

typedef int64_t           s8;
typedef uint64_t          u8;


/* Define the size of a function pointer used in function pointer casts. ******/

typedef uintptr_t                      ptrint;

namespace cacao {

// numeric limits template
//
// std::numeric_limits<T>::max() can not be used in some cases because it is no
// constant expression.

template <class T> struct numeric_limits {
public:
	static const T min;
	static const T max;
};

/**
 * C++11 INTXX_MIN etc are available in C++11
 */
#if 0
template <> struct numeric_limits<int8_t> {
public:
	static const int8_t min = INT8_MIN;
	static const int8_t max = INT8_MAX;
};
template <> struct numeric_limits<uint8_t> {
public:
	static const uint8_t min = 0;
	static const uint8_t max = UINT8_MAX;
};
template <> struct numeric_limits<int16_t> {
public:
	static const int16_t min = INT16_MIN;
	static const int16_t max = INT16_MAX;
};
template <> struct numeric_limits<uint16_t> {
public:
	static const uint16_t min = 0;
	static const uint16_t max = UINT16_MAX;
};
template <> struct numeric_limits<int32_t> {
public:
	static const int32_t min = INT32_MIN;
	static const int32_t max = INT32_MAX;
};
template <> struct numeric_limits<uint32_t> {
public:
	static const uint32_t min = 0;
	static const uint32_t max = UINT32_MAX;
};
template <> struct numeric_limits<int64_t> {
public:
	static const int64_t min = INT64_MIN;
	static const int64_t max = INT64_MAX;
};
template <> struct numeric_limits<uint64_t> {
public:
	static const uint64_t min = 0;
	static const uint64_t max = UINT64_MAX;
};
#endif
template <> struct numeric_limits<signed char> {
public:
	static const signed char min = SCHAR_MIN;
	static const signed char max = SCHAR_MAX;
};
template <> struct numeric_limits<char> {
public:
	static const char min = CHAR_MIN;
	static const char max = CHAR_MAX;
};
template <> struct numeric_limits<short> {
public:
	static const short min = SHRT_MIN;
	static const short max = SHRT_MAX;
};
template <> struct numeric_limits<int> {
public:
	static const int min = INT_MIN;
	static const int max = INT_MAX;
};
template <> struct numeric_limits<long> {
public:
	static const long min = LONG_MIN;
	static const long max = LONG_MAX;
};

template <> struct numeric_limits<unsigned short> {
public:
	static const unsigned short min = 0;
	static const unsigned short max = USHRT_MAX;
};
template <> struct numeric_limits<unsigned int> {
public:
	static const unsigned int min = 0;
	static const unsigned int max = UINT_MAX;
};
template <> struct numeric_limits<unsigned char> {
public:
	static const unsigned char min = 0;
	static const unsigned char max = UCHAR_MAX;
};
template <> struct numeric_limits<unsigned long int> {
public:
	static const unsigned long int min = 0;
	static const unsigned long int max = ULONG_MAX;
};
}

#endif // CACAO_TYPES_HPP_


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
