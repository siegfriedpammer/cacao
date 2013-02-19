/* src/toolbox/output_stream.hpp - output stream classes and helper functions

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

#ifndef __CACAO_OUTPUT_STREAM_HPP__
#define __CACAO_OUTPUT_STREAM_HPP__ 1

#include "vm/utf8.hpp"
#include "vm/string.hpp"

#include <cstring>
#include <stdint.h>
#include <stdarg.h>

/* OutputStream ****************************************************************

	Methods return *this for chaining.

*******************************************************************************/
class OutputStream {
	public:
		virtual ~OutputStream();
		
		inline OutputStream() {}

		// write to buffer byte-by-byte
		virtual OutputStream& write(char c)              = 0;
		virtual OutputStream& write(const char*, size_t) = 0;
		virtual OutputStream& write(const u2*,   size_t) = 0;

		virtual inline OutputStream& write(Utf8String u) {
			return write(u.begin(), u.size());
		}
		virtual inline OutputStream& write(JavaString s) {
			return write(s.get_contents(), s.size());
		}
		virtual OutputStream& write(const char *cs) {
			return write(cs, std::strlen(cs));
		}

		// write to buffer, replacing '/' by '.'
		virtual OutputStream& write_slash_to_dot(Utf8String) = 0;

		// write to buffer, replacing '.' by '/'
		virtual OutputStream& write_dot_to_slash(Utf8String) = 0;

		// like printf
		virtual OutputStream& writef(const char* fmt, ...)         = 0;
		virtual OutputStream& writevf(const char* fmt, va_list ap) = 0;
	private:
		// non-copyable, non-assignable
		OutputStream(const OutputStream&);
		OutputStream& operator=(const OutputStream&);
};

#endif /* __CACAO_OUTPUT_STREAM_HPP__ */

/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
