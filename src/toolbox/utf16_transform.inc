/* src/toolbox/utf16_transform.inc - implementation of utf16 encoder

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

/*******************************************************************************
	this section defines a specialized UTF-16 decoder function

	The overloading is achieved via tag dispatch with the empty template type
	utf16::Tag.

*******************************************************************************/

#ifdef _VISIT_UTF16
	#define _VISITOR_TYPE utf_utils::VISIT_BOTH
#else
	#define _VISITOR_TYPE utf_utils::VISIT_UTF8
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

namespace utf16_impl {
	template<typename T, typename Fn>
	inline T transform(const uint16_t *cs, size_t sz, Fn fn, _TAG tag) {

		for (const uint16_t *const end = cs + sz; cs != end; ++cs) {
			uint16_t c = *cs;

			#ifdef _VISIT_UTF16
				fn.utf16(c);
			#endif

			if ((c != 0) && (c < 0x80)) {
				// 1 character
				fn.utf8((char) c);
			} else if (c < 0x800) {
				// 2 characters
	    		unsigned char high = c >> 6;
	    		unsigned char low  = c & 0x3F;

				fn.utf8(high | 0xC0);
				fn.utf8(low  | 0x80);
			} else {
		    	// 3 characters
		    	char low  = c & 0x3f;
		    	char mid  = (c >> 6) & 0x3F;
		    	char high = c >> 12;

				fn.utf8(high | 0xE0);
				fn.utf8(mid  | 0x80);
				fn.utf8(low  | 0x80);
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

