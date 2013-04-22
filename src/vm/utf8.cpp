/* src/vm/utf8.cpp - utf8 string functions

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

#include "vm/utf8.hpp"
#include "mm/memory.hpp"
#include "toolbox/intern_table.hpp"
#include "toolbox/OStream.hpp"
#include "toolbox/utf_utils.hpp"
#include "vm/options.hpp"


//****************************************************************************//
//*****          GLOBAL UTF8-STRING INTERN TABLE                         *****//
//****************************************************************************//

struct Utf8Key {
	Utf8Key(const char *text, size_t size, size_t hash)
	 : text(text), size(size), hash(hash) {}

	const char* const text;
	const size_t      size;
	const size_t      hash;
};
struct Utf8Hash {
	inline size_t operator()(Utf8String     u) const { return u.hash(); }
	inline size_t operator()(const Utf8Key& k) const { return k.hash;   }
};
struct Utf8Eq {
	inline bool operator()(Utf8String a, Utf8String b) const
	{
		return eq(a.size(), a.hash(), a.begin(),
		          b.size(), b.hash(), b.begin());
	}
	inline bool operator()(Utf8String a, const Utf8Key& b) const
	{
		return eq(a.size(), a.hash(), a.begin(),
		          b.size,   b.hash,   b.text);
	}

	static inline bool eq(size_t a_sz, size_t a_hash, const char *a_cs, 
	                      size_t b_sz, size_t b_hash, const char *b_cs) {
		return (a_sz   == b_sz)   && 
		       (a_hash == b_hash) &&
		       (memcmp(a_cs, b_cs, a_sz) == 0);
	}
};

typedef InternTable<Utf8String, Utf8Hash, Utf8Eq, 1> Utf8InternTable;

static Utf8InternTable *intern_table;

// initial size of intern table
#define HASHTABLE_UTF_SIZE 16384

void Utf8String::initialize(void)
{
	TRACESUBSYSTEMINITIALIZATION("utf8_init");

	/* create utf8 intern table */

	intern_table = new Utf8InternTable(HASHTABLE_UTF_SIZE);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_utf_len += sizeof(utf*) * HASHTABLE_UTF_SIZE;
#endif

	/* create utf-symbols for pointer comparison of frequently used strings */

#define UTF8( NAME, STR ) utf8::NAME = Utf8String::from_utf8( STR );
#include "vm/utf8.inc"
}

/* Utf8String::initialize ******************************************************

   Check if utf8 subsytem is initialized

*******************************************************************************/

bool Utf8String::is_initialized(void)
{
	return intern_table != NULL;
}

//****************************************************************************//
//*****          INTERNAL DATA REPRESENTATION                            *****//
//****************************************************************************//

// TODO: move definition of struct utf here

inline Utf8String Utf8String::alloc(size_t sz) {
	Utf* str = (Utf*) mem_alloc(offsetof(Utf,text) + sz + 1);

	#if STATISTICS_ENABLED
		if (opt_stat) count_utf_new++;
	#endif

	str->blength = sz;

	return Utf8String((utf*) str);
}
inline void Utf8String::free(Utf8String u) {
	mem_free(u._data, offsetof(Utf,text) + u.size() + 1);
}

//****************************************************************************//
//*****          HASHING                                                 *****//
//****************************************************************************//

/* init/update/finish_hash *****************************************************
	
	These routines are used to compute the hash for a utf-8 string byte by byte.

	Use like this:
		size_t hash = 0;

		for each byte in string:
			hash = update_hash( hash, byte );

		hash = finish_hash(hash);
		
	The algorithm is the "One-at-a-time" algorithm as published
	by Bob Jenkins on http://burtleburtle.net/bob/hash/doobs.html.

*******************************************************************************/

static inline size_t update_hash(size_t hash, uint8_t byte)
{
	hash += byte;
	hash += (hash << 10);
	hash ^= (hash >> 6);

	return hash;
}

static inline size_t finish_hash(size_t hash)
{
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}

static inline size_t compute_hash(const char *cs, size_t sz) {
	size_t hash = 0;

	for (const char *end = cs + sz; cs != end; cs++) {
		hash = update_hash(hash, *cs);
	}

	return finish_hash(hash);
}

//****************************************************************************//
//*****          UTF-8 STRING                                            *****//
//****************************************************************************//

// create & intern string

struct StringBuilderBase {
	public:
		inline StringBuilderBase(size_t sz) : _hash(0), _codepoints(0) {}

		inline void utf8 (uint8_t  c) { _hash = update_hash(_hash, c); }
		inline void utf16(uint16_t c) { _codepoints++; }

		inline void finish() {
			_hash = finish_hash(_hash);
		}
	protected:
		size_t _hash;
		size_t _codepoints;
};

// Builds a new utf8 string, always allocates a new string.
// If the string was already interned, throw it away
template<uint8_t (*Fn)(uint8_t)>
struct EagerStringBuilder : private StringBuilderBase {
	public:
		typedef utf_utils::Tag<utf_utils::VISIT_BOTH, utf_utils::ABORT_ON_ERROR> Tag;

		inline EagerStringBuilder(size_t sz) : StringBuilderBase(sz) {
			_out  = Utf8String::alloc(sz);
			_text = (char*) _out.begin();
		}

		inline void utf8(uint8_t c) {
			c = Fn(c);

			StringBuilderBase::utf8(c);

			*_text++ = c;
		}

		using StringBuilderBase::utf16;

		inline Utf8String finish() {
			StringBuilderBase::finish();

			*_text                 = '\0';
			_out._data->utf16_size = _codepoints;
			_out._data->hash       = _hash;

			Utf8String intern = intern_table->intern(_out);

			if (intern != _out) Utf8String::free(_out);

			return intern;
		}

		inline Utf8String abort() {
			Utf8String::free(_out);
			return 0;
		}
	private:
		Utf8String _out;
		char      *_text;
};


// Builds a new utf8 string.
// Only allocates a new string if the string was not already intern_table.
struct LazyStringBuilder : private StringBuilderBase {
	public:
		typedef utf_utils::Tag<utf_utils::VISIT_BOTH, utf_utils::ABORT_ON_ERROR> Tag;

		inline LazyStringBuilder(const char *src, size_t sz)
		 : StringBuilderBase(sz), src(src), sz(sz) {}

		using StringBuilderBase::utf8;
		using StringBuilderBase::utf16;

		inline Utf8String finish() {
			StringBuilderBase::finish();

			return intern_table->intern(Utf8Key(src, sz, _hash), *this);
		}

		// lazily construct an utf8-string
		operator Utf8String() const {
			Utf8String str = Utf8String::alloc(sz);

			str._data->utf16_size = _codepoints;
			str._data->hash       = _hash;

			char *text = (char*) str.begin();

			memcpy(text, src, sz);
			text[sz] = '\0';

			return str;
		}

		inline Utf8String abort() { return 0; }
	private:
		const char *src;
		size_t      sz;
};

namespace {
	inline uint8_t identity(uint8_t c)     { return c; }
	inline uint8_t slash_to_dot(uint8_t c) { return (c == '/') ? '.' : c; }
	inline uint8_t dot_to_slash(uint8_t c) { return (c == '.') ? '/' : c; }
}

Utf8String Utf8String::from_utf8(const char *cs, size_t sz) {
	return utf8::transform<Utf8String>(cs, sz, LazyStringBuilder(cs, sz));
}

Utf8String Utf8String::from_utf8_dot_to_slash(const char *cs, size_t sz) {
	return utf8::transform<Utf8String>(cs, sz, 
	                                   EagerStringBuilder<dot_to_slash>(sz));
}

Utf8String Utf8String::from_utf16(const u2 *cs, size_t sz) {
	size_t blength = utf8::num_bytes(cs, sz);

	return utf16::transform<Utf8String>(cs, sz, 
	                                    EagerStringBuilder<identity>(blength));
}

Utf8String Utf8String::from_utf16_dot_to_slash(const u2 *cs, size_t sz) {
	size_t blength = utf8::num_bytes(cs, sz);

	return utf16::transform<Utf8String>(cs, sz, 
	                                    EagerStringBuilder<dot_to_slash>(blength));
}

Utf8String Utf8String::from_utf8_slash_to_dot(Utf8String u) {
	size_t sz = u.size();

	return utf8::transform<Utf8String>(u.begin(), sz,
	                                   EagerStringBuilder<slash_to_dot>(sz));
}

/* Utf8String::utf16_iterator **************************************************

	A forward iterator over the utf16 codepoints in a Utf8String

*******************************************************************************/

Utf8String::utf16_iterator::utf16_iterator(byte_iterator bs, size_t sz)
: codepoint(0), bytes(bs), end(bs + sz) {
	this->operator++();
}

void Utf8String::utf16_iterator::operator++()
{
	if (bytes != end)
		codepoint = utf8::decode_char(bytes);
	else
		codepoint = -1;
}

Utf8String::utf16_iterator Utf8String::utf16_begin() const {
	assert(_data);

	return utf16_iterator(_data->text, _data->blength);
}

/* Utf8String::substring *******************************************************

	Access last element, accessing a null or empty string leads to 
	undefined behaviour

*******************************************************************************/

Utf8String Utf8String::substring(size_t from) const
{
	return substring(from, size());
}

Utf8String Utf8String::substring(size_t from, size_t to) const
{
	assert(_data);
	assert(from >  0);
	assert(from <= to);
	assert(to   <= size());

	return Utf8String::from_utf8(begin() + from, to - from);
}

bool Utf8String::is_valid_name() const {
	Utf8String::byte_iterator it  = this->begin();
	Utf8String::byte_iterator end = this->end();

	for (; it != end; it++) {
		unsigned char c = *it;

		if (c < 0x20)                   return false; // disallow control characters
		if (c == 0xc0 && it[1] == 0x80) return false; // disallow zero
	}

	return true;
}

//****************************************************************************//
//*****          PUBLIC UTF-8 FUNCTIONS                                  *****//
//****************************************************************************//

/* Utf8String::initialize ******************************************************

   Initializes the utf8 subsystem.

*******************************************************************************/

/* utf8::num_codepoints ********************************************************

	Count number of UTF-16 code points in UTF-8 string.

	Returns -1 on error

*******************************************************************************/

struct SafeCodePointCounter {
	public:
		typedef utf_utils::Tag<utf_utils::VISIT_UTF16, utf_utils::ABORT_ON_ERROR> Tag;

		SafeCodePointCounter() : count(0) {}

		inline void utf8(uint8_t) const {}
		inline void utf16(uint16_t) { count++; }
	
		inline long finish() { return count; }
		inline long abort()  { return -1;    }
	private:
		long count;
};

long utf8::num_codepoints(const char *cs, size_t sz) {
	return utf8::transform<long>(cs, sz, SafeCodePointCounter());
}

/* utf8::num_bytes *************************************************************

	Calculate how many bytes a UTF-8 encoded version of a UTF-16 string 
	would need.

*******************************************************************************/

struct ByteCounter {
	public:
		typedef utf_utils::Tag<utf_utils::VISIT_UTF8, utf_utils::IGNORE_ERRORS> Tag;

		ByteCounter() : count(0) {}

		inline void utf8(uint8_t) { count++; }
		inline void utf16(uint16_t) const {}
	
		inline size_t finish() { return count; }
	private:
		size_t count;
};

size_t utf8::num_bytes(const u2 *cs, size_t sz)
{
	return utf16::transform<size_t>(cs, sz, ByteCounter());
}

//****************************************************************************//
//*****          GLOBAL UTF8-STRING CONSTANTS                            *****//
//****************************************************************************//

#define UTF8( NAME, STR ) Utf8String utf8::NAME;
#include "vm/utf8.inc"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// LEGACY C API
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

extern const char *utf8_text(utf *u) { return Utf8String(u).begin(); }
extern const char *utf8_end (utf *u) { return Utf8String(u).end();   }
   
extern size_t utf8_size(utf *u) { return Utf8String(u).size(); }
extern size_t utf8_hash(utf *u) { return Utf8String(u).hash(); }

/* utf_display_printable_ascii *************************************************

   Write utf symbol to stdout (for debugging purposes).
   Non-printable and non-ASCII characters are printed as '?'.

*******************************************************************************/

template<uint8_t (*Fn)(uint8_t)>
class DisplayPrintableAscii {
	public:
		typedef utf_utils::Tag<utf_utils::VISIT_UTF16, utf_utils::REPLACE_ON_ERROR> Tag;

		inline DisplayPrintableAscii(FILE *dst) : _dst(dst) {}

		inline void utf8 (uint8_t c) const {}
		inline void utf16(uint16_t c) {
			char out;

			out = (c >= 32 && c <= 127) ? c : '?';
			out = Fn(c);
			
			fputc(out, _dst);
		}

		inline uint16_t replacement() const { return '?'; }
		
		inline void finish() {fflush(_dst);}
		inline void abort()  const {}
	private:
		FILE* _dst;
};

void utf_display_printable_ascii(Utf8String u)
{
	if (u == NULL) {
		printf("NULL");
		fflush(stdout);
		return;
	}

	utf8::transform<void>(u.begin(), u.size(),
	                      DisplayPrintableAscii<identity>(stdout));
}


/* utf_display_printable_ascii_classname ***************************************

   Write utf symbol to stdout with `/' converted to `.' (for debugging
   purposes).
   Non-printable and non-ASCII characters are printed as '?'.

*******************************************************************************/

void utf_display_printable_ascii_classname(Utf8String u)
{
	if (u == NULL) {
		printf("NULL");
		fflush(stdout);
		return;
	}

	utf8::transform<void>(u.begin(), u.size(),
	                      DisplayPrintableAscii<slash_to_dot>(stdout));
}


/* utf_sprint_convert_to_latin1 ************************************************
	
   Write utf symbol into c-string (for debugging purposes).
   Characters are converted to 8-bit Latin-1, non-Latin-1 characters yield
   invalid results.

*******************************************************************************/

template<uint8_t (*Fn)(uint8_t)>
class SprintConvertToLatin1 {
	public:
		typedef utf_utils::Tag<utf_utils::VISIT_UTF16, utf_utils::IGNORE_ERRORS> Tag;

		inline SprintConvertToLatin1(char* dst) : _dst(dst) {}

		inline void utf8 (uint8_t c) const {}
		inline void utf16(uint16_t c) { *_dst++ = Fn(c); }
		
		inline void finish() { *_dst = '\0'; }
		inline void abort() const {}
	private:
		char* _dst;
};

void utf_sprint_convert_to_latin1(char *buffer, utf *u)
{
	if (!u) {
		strcpy(buffer, "NULL");
		return;
	}

	utf8::transform<void>(UTF_TEXT(u), UTF_SIZE(u),
	                      SprintConvertToLatin1<identity>(buffer));
}


/* utf_sprint_convert_to_latin1_classname **************************************
	
   Write utf symbol into c-string with `/' converted to `.' (for debugging
   purposes).
   Characters are converted to 8-bit Latin-1, non-Latin-1 characters yield
   invalid results.

*******************************************************************************/

void utf_sprint_convert_to_latin1_classname(char *buffer, utf *u)
{
	if (!u) {
		strcpy(buffer, "NULL");
		return;
	}

	utf8::transform<void>(UTF_TEXT(u), UTF_SIZE(u),
	                      SprintConvertToLatin1<slash_to_dot>(buffer));
}


/* utf_strcat_convert_to_latin1 ************************************************
	
   Like libc strcat, but uses an utf8 string.
   Characters are converted to 8-bit Latin-1, non-Latin-1 characters yield
   invalid results.

*******************************************************************************/

void utf_strcat_convert_to_latin1(char *buffer, utf *u)
{
	utf_sprint_convert_to_latin1(buffer + strlen(buffer), u);
}


/* utf_strcat_convert_to_latin1_classname **************************************
	
   Like libc strcat, but uses an utf8 string.
   Characters are converted to 8-bit Latin-1, non-Latin-1 characters yield
   invalid results.

*******************************************************************************/

void utf_strcat_convert_to_latin1_classname(char *buffer, utf *u)
{
	utf_sprint_convert_to_latin1_classname(buffer + strlen(buffer), u);
}


/* utf_fprint_printable_ascii **************************************************
	
   Write utf symbol into file.
   Non-printable and non-ASCII characters are printed as '?'.

*******************************************************************************/

void utf_fprint_printable_ascii(FILE *file, Utf8String u)
{
	if (!u) return;

	utf8::transform<void>(u.begin(), u.size(),
	                      DisplayPrintableAscii<identity>(file));
}


/* utf_fprint_printable_ascii_classname ****************************************
	
   Write utf symbol into file with `/' converted to `.'.
   Non-printable and non-ASCII characters are printed as '?'.

*******************************************************************************/

void utf_fprint_printable_ascii_classname(FILE *file, Utf8String u)
{
	if (!u) return;

	utf8::transform<void>(u.begin(), u.size(),
	                      DisplayPrintableAscii<slash_to_dot>(file));
}

struct Utf8Validator {
	typedef utf_utils::Tag<utf_utils::VISIT_NONE, utf_utils::ABORT_ON_ERROR> Tag;

	inline bool finish() { return true;  }
	inline bool abort()  { return false; }
};

const size_t Utf8String::sizeof_utf = sizeof(Utf8String::Utf);

namespace cacao {

// OStream operators
OStream& operator<<(OStream& os, const Utf8String &u) {
  os << u.begin();
  return os;
}

}

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
