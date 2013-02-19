/* src/toolbox/utf8_transform.inc - implementation of utf8 decoder

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

// this header is used to define specializations of the UTF8 decoder
// for different use cases

#ifndef __UTF8_TRANSFORM_INC__
#define __UTF8_TRANSFORM_INC__ 1

/*******************************************************************************
	this section defines the data common to all UTF-8 decoder functions
*******************************************************************************/

namespace utf8_impl {
	// common data for all utf-8/16 transfomer functions

	// every byte in a UTF-8 sequence falls into one
	// of these categories
	enum {
		UTF8_ASCII_BYTE               = 0,
		UTF8_ILLEGAL_BYTE             = 1,
		UTF8_CONTINUATION_BYTE        = 2,
		UTF8_START_OF_2_BYTE_SEQUENCE = 3,
		UTF8_START_OF_3_BYTE_SEQUENCE = 4
	};

	// the decoder is modeled as a finite state machine
	// with these states
	enum {
		// done decoding
		UTF8_ACCEPT      = 0,
		UTF8_ERROR       = 1,
		// need more input
		UTF8_READ_1_OF_2 = 2,
		UTF8_READ_1_OF_3 = 3,
		UTF8_READ_2_OF_3 = 4
	};

	static inline uint8_t get_byte_category(uint8_t byte) {
		static const uint8_t byte_to_category[256] = {
#define   _2_TIMES( X ) X, X
#define   _4_TIMES( X )  _2_TIMES( X ),  _2_TIMES( X )
#define   _8_TIMES( X )  _4_TIMES( X ),  _4_TIMES( X )
#define  _16_TIMES( X )  _8_TIMES( X ),  _8_TIMES( X )
#define  _32_TIMES( X ) _16_TIMES( X ), _16_TIMES( X )
#define  _64_TIMES( X ) _32_TIMES( X ), _32_TIMES( X )
#define _127_TIMES( X ) _64_TIMES( X ), _32_TIMES( X ), _16_TIMES( X ), \
	                     _8_TIMES( X ),  _4_TIMES( X ),  _2_TIMES( X ), X
			// java's modified utf8 forbids 0
			UTF8_ILLEGAL_BYTE,

			// all ascii characters except 0 are valid
			_127_TIMES( UTF8_ASCII_BYTE ),

			// bytes from 0x80 to 0xBF are the 2nd or 3rd byte in a multi byte
			// sequence
			_64_TIMES( UTF8_CONTINUATION_BYTE ),

			// bytes from 0xC0 to 0xDF begin a two byte sequence
			_32_TIMES( UTF8_START_OF_2_BYTE_SEQUENCE ),

			// bytes from 0xE0 to 0xEF begin a three byte sequence
			_16_TIMES( UTF8_START_OF_3_BYTE_SEQUENCE ),

			// byte from 0xF0 to 0xFF are forbidden
			_16_TIMES( UTF8_ILLEGAL_BYTE )
#undef   _2_TIMES
#undef   _4_TIMES
#undef   _8_TIMES
#undef  _16_TIMES
#undef  _32_TIMES
#undef  _64_TIMES
#undef _127_TIMES
		};
		return byte_to_category[byte];
	}

	static inline uint8_t transition(unsigned state, unsigned category) {
		static const uint8_t transition_table[5][5] = {
			{
				// current state: UTF8_ACCEPT
				/* input: UTF8_ASCII_BYTE               */ UTF8_ACCEPT,
				/* input: UTF8_ILLEGAL_BYTE             */ UTF8_ERROR,
				/* input: UTF8_CONTINUATION_BYTE        */ UTF8_ERROR,
				/* input: UTF8_START_OF_2_BYTE_SEQUENCE */ UTF8_READ_1_OF_2,
				/* input: UTF8_START_OF_3_BYTE_SEQUENCE */ UTF8_READ_1_OF_3
			},
			{
				// current state: UTF8_ERROR
				/* input: UTF8_ASCII_BYTE               */ UTF8_ERROR,
				/* input: UTF8_ILLEGAL_BYTE             */ UTF8_ERROR,
				/* input: UTF8_CONTINUATION_BYTE        */ UTF8_ERROR,
				/* input: UTF8_START_OF_2_BYTE_SEQUENCE */ UTF8_ERROR,
				/* input: UTF8_START_OF_3_BYTE_SEQUENCE */ UTF8_ERROR
			},
			{
				// current state: UTF8_READ_1_OF_2
				/* input: UTF8_ASCII_BYTE               */ UTF8_ERROR,
				/* input: UTF8_ILLEGAL_BYTE             */ UTF8_ERROR,
				/* input: UTF8_CONTINUATION_BYTE        */ UTF8_ACCEPT,
				/* input: UTF8_START_OF_2_BYTE_SEQUENCE */ UTF8_ERROR,
				/* input: UTF8_START_OF_3_BYTE_SEQUENCE */ UTF8_ERROR
			},
				{
				// current state: UTF8_READ_1_OF_3
				/* input: UTF8_ASCII_BYTE               */ UTF8_ERROR,
				/* input: UTF8_ILLEGAL_BYTE             */ UTF8_ERROR,
				/* input: UTF8_CONTINUATION_BYTE        */ UTF8_READ_2_OF_3,
				/* input: UTF8_START_OF_2_BYTE_SEQUENCE */ UTF8_ERROR,
				/* input: UTF8_START_OF_3_BYTE_SEQUENCE */ UTF8_ERROR
			},
			{
				// current state: UTF8_READ_1_OF_3
				/* input: UTF8_ASCII_BYTE               */ UTF8_ERROR,
				/* input: UTF8_ILLEGAL_BYTE             */ UTF8_ERROR,
				/* input: UTF8_CONTINUATION_BYTE        */ UTF8_ACCEPT,
				/* input: UTF8_START_OF_2_BYTE_SEQUENCE */ UTF8_ERROR,
					/* input: UTF8_START_OF_3_BYTE_SEQUENCE */ UTF8_ERROR
			},
		};

		return transition_table[state][category];
	}
}

#endif /* __UTF8_TRANSFORM_INC__ */

/*******************************************************************************
	this section defines a specialized UTF-8 decoder function

	The overloading is achieved via tag dispatch with the empty template type
	utf8::Tag.

*******************************************************************************/

#if defined(_VISIT_UTF8) && defined(_VISIT_UTF16)
	#define _VISITOR_TYPE utf_utils::VISIT_BOTH
#elif defined(_VISIT_UTF8)
	#define _VISITOR_TYPE utf_utils::VISIT_UTF8
#elif defined(_VISIT_UTF16)
	#define _VISITOR_TYPE utf_utils::VISIT_UTF16
#else
	#define _VISITOR_TYPE utf_utils::VISIT_NONE
#endif

#if   defined(_IGNORE_ERRORS)
	#define _ERROR_ACTION utf_utils::IGNORE_ERRORS
#elif defined(_REPLACE_ON_ERROR)
	#define _ERROR_ACTION utf_utils::REPLACE_ON_ERROR
#elif defined(_ABORT_ON_ERROR)
	#define _ERROR_ACTION utf_utils::ABORT_ON_ERROR
#else
	#error "No error handling strategy was specified"
#endif

#define _TAG utf_utils::Tag<_VISITOR_TYPE, _ERROR_ACTION>

namespace utf8_impl {
	template<typename T, typename Fn>
	inline T transform(const char *cs, size_t sz, Fn fn, _TAG tag ) {

		uint16_t codepoint = 0;
		unsigned state     = UTF8_ACCEPT;

		for (const char *const end = cs + sz; cs != end; ++cs) {
			uint8_t byte = *cs;

			// apply callback to input byte
			#ifdef _VISIT_UTF8
				fn.utf8(byte);
			#endif

			// get the character category for input byte
			unsigned category = get_byte_category(byte);

			// get the new state the decoder is in after reading input byte
			state = transition(state, category);

			// mask out non-data bits of byte
			byte = byte & (0xFF >> category);

			// make room in codepoint for new data bits
			if (category) codepoint <<= 8 - category;

			// add new bits to codepoint
			codepoint = codepoint | byte;

			switch (state) {
				case UTF8_ACCEPT:
					// apply callback to decoded codepoint
					#ifdef _VISIT_UTF16
						fn.utf16(codepoint);
					#endif
					codepoint = 0;
					break;
				case UTF8_ERROR:
					#if  defined(_IGNORE_ERRORS)
						state     = UTF8_ACCEPT;
						codepoint = 0;
						continue;
					#elif defined(_REPLACE_ON_ERROR)
						fn.utf16(fn.replacement());
						state     = UTF8_ACCEPT;
						codepoint = 0;
						break;
					#elif defined(_ABORT_ON_ERROR)
						return fn.abort();
					#endif
			}
		}

		return fn.finish();
	}
}

#undef _TAG
#undef _VISITOR_TYPE
#undef _ERROR_ACTION

#ifdef _VISIT_UTF8
	#undef _VISIT_UTF8
#endif
#ifdef _VISIT_UTF16
	#undef _VISIT_UTF16
#endif
#ifdef _IGNORE_ERRORS
	#undef _IGNORE_ERRORS
#endif
#ifdef _REPLACE_ON_ERROR
	#undef _REPLACE_ON_ERROR
#endif
#ifdef _ABORT_ON_ERROR
	#undef _ABORT_ON_ERROR
#endif


