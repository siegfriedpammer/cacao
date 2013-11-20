/* src/toolbox/utf8_transform.inc - implementation of utf8 decoder

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

#ifndef UTF8_TRANSFORM_INC
#define UTF8_TRANSFORM_INC 1

namespace utf8 {
namespace impl {
	// every byte in a UTF-8 sequence falls into one
	// of these categories
	enum utf8_byte_category {
		UTF8_ILLEGAL_BYTE             = 0,
		UTF8_ASCII_BYTE               = 1,
		UTF8_CONTINUATION_BYTE        = 2,
		UTF8_START_OF_2_BYTE_SEQUENCE = 3,
		UTF8_START_OF_3_BYTE_SEQUENCE = 4
	};

	// the decoder is modeled as a finite state machine
	// with these states
	enum utf8_machine_state {
		// done decoding
		UTF8_ERROR       = 0,
		UTF8_ACCEPT      = 1, // also the starting state
		// need more input
		UTF8_READ_1_OF_2 = 2,
		UTF8_READ_1_OF_3 = 3,
		UTF8_READ_2_OF_3 = 4
	};

	static inline utf8_byte_category get_byte_category(uint8_t byte) {
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
		return (utf8_byte_category) byte_to_category[byte];
	}

	static inline utf8_machine_state transition(utf8_machine_state state, utf8_byte_category category) {
		static const uint8_t transition_table[5][5] = {
			{
				// current state: UTF8_ERROR
				/* input: UTF8_ILLEGAL_BYTE             */ UTF8_ERROR,
				/* input: UTF8_ASCII_BYTE               */ UTF8_ERROR,
				/* input: UTF8_CONTINUATION_BYTE        */ UTF8_ERROR,
				/* input: UTF8_START_OF_2_BYTE_SEQUENCE */ UTF8_ERROR,
				/* input: UTF8_START_OF_3_BYTE_SEQUENCE */ UTF8_ERROR
			},
			{
				// current state: UTF8_ACCEPT
				/* input: UTF8_ILLEGAL_BYTE             */ UTF8_ERROR,
				/* input: UTF8_ASCII_BYTE               */ UTF8_ACCEPT,
				/* input: UTF8_CONTINUATION_BYTE        */ UTF8_ERROR,
				/* input: UTF8_START_OF_2_BYTE_SEQUENCE */ UTF8_READ_1_OF_2,
				/* input: UTF8_START_OF_3_BYTE_SEQUENCE */ UTF8_READ_1_OF_3
			},
			{
				// current state: UTF8_READ_1_OF_2
				/* input: UTF8_ILLEGAL_BYTE             */ UTF8_ERROR,
				/* input: UTF8_ASCII_BYTE               */ UTF8_ERROR,
				/* input: UTF8_CONTINUATION_BYTE        */ UTF8_ACCEPT,
				/* input: UTF8_START_OF_2_BYTE_SEQUENCE */ UTF8_ERROR,
				/* input: UTF8_START_OF_3_BYTE_SEQUENCE */ UTF8_ERROR
			},
				{
				// current state: UTF8_READ_1_OF_3
				/* input: UTF8_ILLEGAL_BYTE             */ UTF8_ERROR,
				/* input: UTF8_ASCII_BYTE               */ UTF8_ERROR,
				/* input: UTF8_CONTINUATION_BYTE        */ UTF8_READ_2_OF_3,
				/* input: UTF8_START_OF_2_BYTE_SEQUENCE */ UTF8_ERROR,
				/* input: UTF8_START_OF_3_BYTE_SEQUENCE */ UTF8_ERROR
			},
			{
				// current state: UTF8_READ_1_OF_3
				/* input: UTF8_ILLEGAL_BYTE             */ UTF8_ERROR,
				/* input: UTF8_ASCII_BYTE               */ UTF8_ERROR,
				/* input: UTF8_CONTINUATION_BYTE        */ UTF8_ACCEPT,
				/* input: UTF8_START_OF_2_BYTE_SEQUENCE */ UTF8_ERROR,
				/* input: UTF8_START_OF_3_BYTE_SEQUENCE */ UTF8_ERROR
			},
		};

		return (utf8_machine_state) transition_table[state][category];
	}

	struct CopyUtf8ToUtf16 : utf8::VisitorBase<bool, ABORT_ON_ERROR> {
		typedef bool ReturnType;

		CopyUtf8ToUtf16(uint16_t *dst) : dst(dst) {}

		inline void utf16(uint16_t c) { *dst++ = c; }

		inline bool finish() { return true;  }
		inline bool abort()  { return false; }
	private:
		uint16_t *dst;
	};

} // end namespace impl
} // end namespace utf8

template<typename Iterator, typename Fn>
inline typename Fn::ReturnType utf8::transform(Iterator it, Iterator end, Fn fn) {
	using namespace ::utf8::impl;

	uint16_t           codepoint = 0;
	utf8_machine_state state     = UTF8_ACCEPT;

	for (; it != end; ++it) {
		uint8_t byte = *it;

		// apply callback to input byte
		fn.utf8(byte);

		// get the character category for input byte
		utf8_byte_category category = get_byte_category(byte);

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
				fn.utf16(codepoint);
				codepoint = 0;
				break;
			case UTF8_ERROR:
				// if-cascade seems to constant fold better than a switch

				if (fn.error_action() == ABORT_ON_ERROR) {
					return fn.abort();
				} else if (fn.error_action() == REPLACE_ON_ERROR) {
					fn.utf16(fn.replacement());
					state     = UTF8_ACCEPT;
					codepoint = 0;
				} else {
					// ignore error
					state     = UTF8_ACCEPT;
					codepoint = 0;
				}
				break;
			default:
				break;
		}
	}

	return fn.finish();
}


inline uint16_t utf8::decode_char(const char*& src) {
	uint16_t ch1, ch2, ch3;

	ch1 = src[0];

	switch (((uint8_t) ch1) >> 4) {
	default:  // 1 byte (ASCII)
		src++;
		return ch1;
	case 0xC:
	case 0xD: // 2 bytes
		// mask out non-data bits
		ch1  = ch1    & 0x1F;
		ch2  = src[1] & 0x3F;
		src += 2;

		// stitch together data bits from individual bytes
		return (ch1 << 6) | ch2;
	case 0xE: // 3 bytes
		// mask out non-data bits
		ch1  = ch1    & 0x1F;
		ch2  = src[1] & 0x3F;
		ch3  = src[2] & 0x3F;
		src += 3;

		// stitch together data bits from individual bytes
		return (ch1 << 12) | (ch2 << 6) | ch3;
	}
}

template<typename Utf8Iterator>
inline bool utf8::decode(Utf8Iterator begin, Utf8Iterator end, uint16_t *dst) {
	return ::utf8::transform(begin, end, ::utf8::impl::CopyUtf8ToUtf16(dst));
}

#endif // UTF8_TRANSFORM_INC


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
