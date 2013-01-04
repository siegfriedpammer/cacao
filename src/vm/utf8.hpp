/* src/vm/utf8.h - utf8 string functions

   Copyright (C) 1996-2005, 2006, 2007, 2008
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


#ifndef _UTF_H
#define _UTF_H

/* forward typedefs ***********************************************************/

typedef struct utf utf;

#include "config.h"

#include <stdio.h>
#include <string.h>

#include "vm/types.h"
#include "vm/global.h"

/* data structure for utf8 symbols ********************************************/

struct utf {
	uint32_t hash;       // cached hash of the string
	size_t   blength;    // text length in bytes
	                     // (does NOT include zero terminator)
	size_t   utf16_size; // number of utf16 codepoints in string
			
	char  text[sizeof(void*)]; // string content
	                           // directly embedded in struct utf
	                           // aligned to pointer size    
};

#ifdef __cplusplus

/* Utf8String ******************************************************************

	A container for strings in Java's modified UTF-8 encoding.

	A Utf8String always contains either a valid (possibly empty) UTF-8 string
	or NULL.
	You can check for NULL like you would with any normal pointer.
	Invoking any method except operator utf*() on a NULL string leads to 
	undefined behaviour.

	Use a Utf8String like a pointer, i.e. always pass by value.

	The contents of a Utf8String are zero terminated, and it never contains any
	zero bytes except the one at the end, so any C string processing functions
	work properly.

*******************************************************************************/

class Utf8String {
	public:
		/*** GLOBAL INITIALIZATION **********************************/

		// initialize the utf8 subsystem
		// MUST be called before any Utf8String can be constructed
		static void initialize();

		// check if utf8 subsytem is initialized
		static bool is_initialized();

		/*** CONSTRUCTORS  ******************************************/

		// constructs a null string
		inline Utf8String() : _data(0) {}

		// construct from a buffer with a given length
		// validates that input is really UTF-8
		// constructs a null string on error
		static Utf8String from_utf8(const char*, size_t);
		static Utf8String from_utf8_dot_to_slash(const char*, size_t);

		inline static Utf8String from_utf8(const char *cs) {
			return from_utf8(cs, strlen(cs));
		}
		inline static Utf8String from_utf8_dot_to_slash(const char *cs) {
			return from_utf8_dot_to_slash(cs, strlen(cs));
		}

		// construct from a UTF8String
		static Utf8String from_utf8_slash_to_dot(Utf8String);

		// construct from a UTF-16 string with a given length
		static Utf8String from_utf16(const u2*, size_t);
		static Utf8String from_utf16_dot_to_slash(const u2*, size_t);

		// constructs a Utf8String with a given content
		// is only public for interop with legacy C code
		// NOTE: does NOT perform any checks
		inline Utf8String(utf *u) : _data(u) {} 

		/*** ITERATION     ******************************************/

		// iterator over the bytes in a string
		typedef const char* byte_iterator;

		byte_iterator begin() const;
		byte_iterator end()   const;

		// iterator over UTF-16 codepoints in a string
		class utf16_iterator {
			public:
				inline uint32_t operator*() const { return codepoint; }

				void operator++();

				inline operator void*() { return bytes == end ? this : 0; }
			private:
				utf16_iterator(byte_iterator,size_t);

				uint32_t      codepoint;
				byte_iterator bytes;
				byte_iterator end;

			friend class Utf8String;
		};

		utf16_iterator utf16_begin() const;

		/*** HASHING       ******************************************/

		size_t hash() const;

		/*** COMPARISONS   ******************************************/

		/*** ACCESSORS     ******************************************/

		// access first element
		char front() const;

		// access last element
		char back() const;

		char operator[]( size_t idx ) const;

		// get the number of bytes in string, excluding zero terminator.
		size_t size() const;

		// get the number of utf16 codepoints in string,
		// excluding zero terminator.
		size_t utf16_size() const;

		// for checking against NULL,
		// also allows interop with legacy C code
		inline operator utf*() const { return _data; }

		// create substring
		Utf8String substring(size_t from ) const;
		Utf8String substring(size_t from, size_t to ) const;

		/*** JAVA STUFF    ******************************************/

		bool is_valid_name() const;
	private:
		utf* _data;
};

// ***** UTF-8 HELPER FUNCTIONS

namespace utf8 {
	// count UTF-16 codepoints, -1 on error
	extern ssize_t num_codepoints(const char*, size_t);

	// count how many bytes a utf-8 version would need
	extern size_t num_bytes(const u2*, size_t);

	// named constants for common utf8 strings
	#define UTF8(NAME, STR) extern Utf8String NAME;
	#include "vm/utf8.inc"
}

#endif /* __cplusplus */

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// LEGACY C API
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define UTF_AT(u, IDX) (utf8_begin(u)[IDX])
#define UTF_BEGIN(u)   utf8_begin(u)
#define UTF_END(u)     utf8_end(u)
#define UTF_SIZE(u)    utf8_size(u)
#define UTF_HASH(u)    utf8_hash(u)

#define SIZEOF_UTF sizeof_utf // only used in loader.cpp

#ifdef __cplusplus
extern "C" {
#endif

extern const char *utf8_begin(utf*);
extern const char *utf8_end(utf*);
extern size_t      utf8_size(utf*);
extern size_t      utf8_hash(utf*);

extern const size_t sizeof_utf;

// these are only used in old logging code in the jit & jvmti

void utf_display_printable_ascii(utf *u);
void utf_display_printable_ascii_classname(utf *u);

void utf_sprint_convert_to_latin1(char *buffer, utf *u);
void utf_sprint_convert_to_latin1_classname(char *buffer, utf *u);

void utf_strcat_convert_to_latin1(char *buffer, utf *u);
void utf_strcat_convert_to_latin1_classname(char *buffer, utf *u);

void utf_fprint_printable_ascii(FILE *file, utf *u);
void utf_fprint_printable_ascii_classname(FILE *file, utf *u);

#ifdef __cplusplus
}
#endif

#endif /* _UTF_H */

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
 * vim:noexpandtab:sw=4:ts=4:
 */
