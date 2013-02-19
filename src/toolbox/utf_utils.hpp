/* src/toolbox/utf_utils.hpp - functions for handling utf8/utf16

   Copyright (C) 2012
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

#ifndef UTF_UTILS_HPP_
#define UTF_UTILS_HPP_ 1

// TODO: get rid of the old utf type and rename utf_utils to utf

// TODO: Maybe rename functions, this is not a transform in the STL sense.
//       It's a fold, like std::accumulate.

namespace utf_utils {
	// what input the transformer functor needs
	enum VisitorType {
		VISIT_NONE,  // Fn is only notified if whole input is valid or not.

		VISIT_UTF8,  // Fn must have a method utf8 that is called for every
		             // input byte.

		VISIT_UTF16, // Fn must have a method utf16 that is called for every
		             // utf-16 codepoint decoded from the input.

		VISIT_BOTH   // combination of VISIT_UTF8 and VISIT_UTF16
	};

	// what the decoder should do when it encounters an error
	enum ErrorAction {
		IGNORE_ERRORS,    // invalid bytes in input are skipped.
		                  // This is the only valid action for utf16::transform

		REPLACE_ON_ERROR, // Fn must have a method replacement() that returns a
		                  // replacement character for invalid input

		ABORT_ON_ERROR    // Fn must have a method abort() that is called if
		                  // an error occurs, transform will return the result
		                  // of abort.
	};

	template<VisitorType VT, ErrorAction EA> struct Tag {};
}

namespace utf8 {
/* utf8::transform *************************************************************

	Iterates over an UTF-8 string and accumulates a result with functor.

	What input is fed to the functor and how errors should be handled can be
	controlled via tags.

	The functor Fn must have at least one method finish() that is called
	when transform completes successfully to compute the final result.
	What other methods the functor must have is controlled by the tag.

	utf8::transform is overloaded for the following tags
		Tag<VISIT_NONE, ABORT_ON_ERROR>

		Tag<VISIT_UTF8, IGNORE_ERRORS>
		Tag<VISIT_UTF8, REPLACE_ON_ERROR>
		Tag<VISIT_UTF8, ABORT_ON_ERROR>

		Tag<VISIT_UTF16, IGNORE_ERRORS>
		Tag<VISIT_UTF16, REPLACE_ON_ERROR>
		Tag<VISIT_UTF16, ABORT_ON_ERROR>

		Tag<VISIT_BOTH, IGNORE_ERRORS>
		Tag<VISIT_BOTH, REPLACE_ON_ERROR>
		Tag<VISIT_BOTH, ABORT_ON_ERROR>

	If you don't explicitly specify a tag the Functor must have a public type
	member called Tag that is used.

*******************************************************************************/

	template<typename T, typename Fn>
	inline T transform(const char*, size_t, Fn);

	template<typename T, typename Fn, typename Tag>
	inline T transform(const char*, size_t, Fn, Tag tag);

/* decode_char *****************************************************************

	Decodes one utf-16 codepoints from input, automatically advances input
	pointer to start of next codepoint.

	Input MUST be valid UTF-8.

*******************************************************************************/

	inline uint16_t decode_char(const char*&);

	// check if char is valid ascii
	inline bool is_ascii(uint8_t c) { return c < 128; }
}

namespace utf16 {

/* utf16::transform ************************************************************

	Iterates over an UTF-16 string and accumulates a result with functor.

	What input is fed to the functor and how errors should be handled can be
	controlled via tags.

	The functor Fn must have at least one method finish() that is called
	when transform completes successfully to compute the final result.

	utf16::transform is overloaded for the following tags
		Tag<VISIT_UTF8, IGNORE_ERRORS>
		Tag<VISIT_BOTH, IGNORE_ERRORS>

		Tag<VISIT_UTF8, REPLACE_ON_ERROR>
		Tag<VISIT_BOTH, REPLACE_ON_ERROR>

		Tag<VISIT_UTF8, ABORT_ON_ERROR>
		Tag<VISIT_BOTH, ABORT_ON_ERROR>

	If you don't explicitly specify a tag the Functor must have a public type
	member called Tag that is used.

	utf16::transform never actually fails, the Tag type is the same as for
	utf8::transform to allow for reuse of functors.

*******************************************************************************/

	template<typename T, typename Fn>
	inline T transform(const u2*, size_t, Fn);

	template<typename T, typename Fn, typename Tag>
	inline T transform(const u2*, size_t, Fn, Tag);

	// check if char is valid ascii
	inline bool is_ascii(uint16_t c) { return c < 128; }
}

/*******************************************************************************
	IMPLEMENTATION
*******************************************************************************/

// specializations for utf8::transform functions
#define _ABORT_ON_ERROR
#include "toolbox/utf8_transform.inc"

#define _VISIT_UTF8
#define _IGNORE_ERRORS
#include "toolbox/utf8_transform.inc"

#define _VISIT_UTF8
#define _REPLACE_ON_ERROR
#include "toolbox/utf8_transform.inc"

#define _VISIT_UTF8
#define _ABORT_ON_ERROR
#include "toolbox/utf8_transform.inc"

#define _VISIT_UTF16
#define _IGNORE_ERRORS
#include "toolbox/utf8_transform.inc"

#define _VISIT_UTF16
#define _REPLACE_ON_ERROR
#include "toolbox/utf8_transform.inc"

#define _VISIT_UTF16
#define _ABORT_ON_ERROR
#include "toolbox/utf8_transform.inc"

#define _VISIT_UTF8
#define _VISIT_UTF16
#define _IGNORE_ERRORS
#include "toolbox/utf8_transform.inc"

#define _VISIT_UTF8
#define _VISIT_UTF16
#define _REPLACE_ON_ERROR
#include "toolbox/utf8_transform.inc"

#define _VISIT_UTF8
#define _VISIT_UTF16
#define _ABORT_ON_ERROR
#include "toolbox/utf8_transform.inc"

// specializations for utf16::transform functions
#define _IGNORE_ERRORS
#include "toolbox/utf16_transform.inc"

#define _REPLACE_ON_ERROR
#include "toolbox/utf16_transform.inc"

#define _ABORT_ON_ERROR
#include "toolbox/utf16_transform.inc"

#define _VISIT_UTF16
#define _IGNORE_ERRORS
#include "toolbox/utf16_transform.inc"

#define _VISIT_UTF16
#define _REPLACE_ON_ERROR
#include "toolbox/utf16_transform.inc"

#define _VISIT_UTF16
#define _ABORT_ON_ERROR
#include "toolbox/utf16_transform.inc"

template<typename T, typename Fn>
inline T utf8::transform(const char *cs, size_t sz, Fn fn) {
	typedef typename Fn::Tag Tag;

	return utf8_impl::transform<T, Fn>(cs, sz, fn, Tag());
}

template<typename T, typename Fn, typename Tag>
inline T utf8::transform(const char *cs, size_t sz, Fn fn, Tag tag) {
	return utf8_impl::transform<T, Fn>(cs, sz, fn, tag);
}

template<typename T, typename Fn>
inline T utf16::transform(const u2 *cs, size_t sz, Fn fn) {
	typedef typename Fn::Tag Tag;

	return utf16_impl::transform<T, Fn>(cs, sz, fn, Tag());
}

template<typename T, typename Fn, typename Tag>
inline T utf16::transform(const u2 *cs, size_t sz, Fn fn, Tag tag) {
	return utf16_impl::transform<T, Fn>(cs, sz, fn, tag);
}

inline uint16_t utf8::decode_char(const char*& src) {
    uint8_t  ch1, ch2, ch3;

	ch1 = *src++;

	switch (ch1 >> 4) {
	default: // 1 byte
		return (uint16_t) ch1;
	case 0xC:
	case 0xD: // 2 bytes
		ch2 = *src++;

		assert((ch2 & 0xC0) == 0x80);

		ch1 = ch1 & 0x1F;
		ch2 = ch1 & 0x3F;

		return (ch1 << 6) + ch2;
	case 0xE: // 3 bytes
		ch2 = *src++;
		ch3 = *src++;

		assert((ch2 & 0xC0) == 0x80);
		assert((ch3 & 0xC0) == 0x80);

		ch1 = ch1 & 0x3F;
		ch2 = ch1 & 0x3F;
		ch3 = ch1 & 0x0F;

		return (((ch1 << 6) + ch2) << 6) + ch3;
    }
}

#endif // UTF_UTILS_HPP_


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
