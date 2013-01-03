#ifndef __CACAO_BUFFER_HPP__
#define __CACAO_BUFFER_HPP__ 1

#include "vm/utf8.hpp"
#include "mm/memory.hpp"

#include "toolbox/utf_utils.hpp"
#include "toolbox/output_stream.hpp"

/* Buffer **********************************************************************

	An OutputStream that writes to an in memory buffer.

	It performs no checks, bounds or otherwise on input strings.
	The buffer automatically grows to fit it's input.

	When a buffer goes out of scope all memory associated with it will be freed

	Don't forget to zero terminate contents of buffer if you use them directly.

*******************************************************************************/

template<template<typename T> class Allocator = MemoryAllocator>
class Buffer : public OutputStream {
	public:
		// construct a buffer with size
		Buffer(size_t buf_size=64);

		// free content of buffer
		~Buffer();
		
		// write to buffer byte-by-byte
		virtual inline Buffer& write(char);
		virtual inline Buffer& write(Utf8String);
		virtual inline Buffer& write(const char*);
		virtual inline Buffer& write(const char*, size_t);
		virtual inline Buffer& write(const u2*,   size_t);

		// write to buffer, replacing '/' by '.'
		virtual inline Buffer& write_slash_to_dot(const char*);
		virtual inline Buffer& write_slash_to_dot(Utf8String);

		// write to buffer, replacing '.' by '/'
		virtual inline Buffer& write_dot_to_slash(Utf8String);

		// like printf
		virtual inline Buffer& writef(const char* fmt, ...);
		virtual inline Buffer& writevf(const char* fmt, va_list ap);

		// get char contents of buffer
		inline operator char*() {
			zero_terminate();
			return _start; 
		}

		// get utf-8 string contents of buffer
		inline Utf8String build();

		// clear buffer content
		// O(1)
		inline void clear();

		// remove data from the back of this buffer.
		// O(1)
		inline void rewind(size_t bytes_to_drop);

		// ensure string in buffer is zero terminated
		Buffer& zero_terminate();
	private:
		void ensure_capacity(size_t);

		// non-copyable, non-assignable
		Buffer(const Buffer&);
		Buffer& operator=(const Buffer&);

		char*           _start, *_end, *_pos;
		Allocator<char> alloc;

		class Encode {
			public:
				typedef utf_utils::Tag<utf_utils::VISIT_UTF8, utf_utils::IGNORE_ERRORS> Tag;

				inline Encode(Buffer& dst) : _dst(dst) {}

				inline void utf8 (uint8_t c) { _dst.write(c); }

				inline void finish() const {}
			private:
				Buffer& _dst;
		};
};

//****************************************************************************//
//*****          IMPLEMENTATION                                          *****//
//****************************************************************************//

/* Buffer::Buffer **************************************************************

	Construct a new Buffer with a given size

*******************************************************************************/

template<template<typename T> class Allocator>
Buffer<Allocator>::Buffer(size_t buf_size)
{
	_start = alloc.allocate(buf_size);
	_end   = _start + buf_size;
	_pos   = _start;
}

template<template<typename T> class Allocator>
Buffer<Allocator>::~Buffer()
{
	alloc.deallocate(_start, _end - _start);
	_start = _end = _pos = 0;
}

/* Buffer::write(Utf8String) ***************************************************

	Insert a utf-8 string into buffer byte by byte.
	Does NOT inserts a zero terminator.

*******************************************************************************/

template<template<typename T> class Allocator>
Buffer<Allocator>& Buffer<Allocator>::write(Utf8String u)
{
	return write(u.begin(), u.size());
}

/* Buffer::write(const char*) **************************************************

	Insert a zero terminated string into buffer byte by byte
	Does NOT inserts a zero terminator.

*******************************************************************************/

template<template<typename T> class Allocator>
Buffer<Allocator>& Buffer<Allocator>::write(const char *cs)
{
	return write(cs, strlen(cs));
}

/* Buffer::write(const char*, size_t) ********************************************

	Insert string with a given length into buffer byte by byte
	Does NOT inserts a zero terminator.

*******************************************************************************/

template<template<typename T> class Allocator>
Buffer<Allocator>& Buffer<Allocator>::write(const char *cs, size_t sz)
{
	ensure_capacity(sz);

	memcpy(_pos, cs, sizeof(char) * sz);

	_pos += sz;

	return *this;
}

/* Buffer::write(const u2*, size_t) ********************************************

	Encode utf-16 string with a given length into buffer
	Does NOT inserts a zero terminator.

*******************************************************************************/

template<template<typename T> class Allocator>
Buffer<Allocator>& Buffer<Allocator>::write(const u2 *cs, size_t sz)
{
	utf16::transform<void>(cs, sz, Encode(*this));

	return *this;
}

/* Buffer::write(char) ***********************************************************

	Insert a utf-8 string into buffer byte by byte.
	Does NOT inserts a zero terminator.

*******************************************************************************/

template<template<typename T> class Allocator>
Buffer<Allocator>& Buffer<Allocator>::write(char c) 
{
	ensure_capacity(1);

	*_pos++ = c;

	return *this;
}

/* Buffer::write_slash_to_dot(Utf8String) **************************************

	Insert a utf-8 string into buffer byte by byte, replacing '/' by '.'.
	Does NOT inserts a zero terminator.

*******************************************************************************/

template<template<typename T> class Allocator>
Buffer<Allocator>& Buffer<Allocator>::write_slash_to_dot(const char *cs) {
	size_t sz = std::strlen(cs);

	ensure_capacity(sz);

	const char* src = cs;
	const char* end = cs + sz;

	for ( ; src != end; ++_pos, ++src ) {
		char c = *src;
		
		*_pos = (c == '/') ? '.' : c;
	}
	
	return *this;
}

template<template<typename T> class Allocator>
Buffer<Allocator>& Buffer<Allocator>::write_slash_to_dot(Utf8String u) {
	ensure_capacity(u.size());

	const char* src = u.begin();
	const char* end = u.end();

	for ( ; src != end; ++_pos, ++src ) {
		char c = *src;
		
		*_pos = (c == '/') ? '.' : c;
	}
	
	return *this;
}

/* Buffer::write_dot_to_slash(Utf8String) **************************************

	Insert a utf-8 string into buffer byte by byte, replacing '.' by '/'.
	Does NOT inserts a zero terminator.

*******************************************************************************/

template<template<typename T> class Allocator>
Buffer<Allocator>& Buffer<Allocator>::write_dot_to_slash(Utf8String u) {
	ensure_capacity(u.size());

	const char* src = u.begin();
	const char* end = u.end();

	for ( ; src != end; ++_pos, ++src ) {
		char c = *src;
		
		*_pos = (c == '.') ? '/' : c;
	}
	
	return *this;
}

/* Buffer::writef/writevf ******************************************************

	Like (v)snprintf but for buffers.

*******************************************************************************/

template<template<typename T> class Allocator>
Buffer<Allocator>& Buffer<Allocator>::writef(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	  writevf(fmt,ap);
	va_end(ap);

	return *this;
}

template<template<typename T> class Allocator>
Buffer<Allocator>& Buffer<Allocator>::writevf(const char *fmt, va_list ap)
{
	size_t size    = _end - _pos;
	size_t written = vsnprintf(_pos, size, fmt, ap);

	if (written > size) {
		// buffer was too small
		ensure_capacity(written);

		size    = _end - _pos;
		written = vsnprintf(_pos, size, fmt, ap);
		_pos   += written;
		assert(written <= size);
	}

	return *this;
}

/* Buffer::build ***************************************************************

	Create a new Utf8String whose contents are equal to the contents of this
	buffer.

	The contents of the buffer must be zero terminated.

*******************************************************************************/

template<template<typename T> class Allocator>
Utf8String Buffer<Allocator>::build()
{
	zero_terminate();

	return Utf8String::from_utf8(_start,_pos - _start);
}

/* Buffer::clear ***************************************************************

	Clear buffer content

*******************************************************************************/

template<template<typename T> class Allocator>
void Buffer<Allocator>::clear()
{
	_pos = _start;
}

/* Buffer::rewind **************************************************************

	Undo write operations by removing data of the back of this buffer.
	Does not perform a bounds check.

	IN:
		bytes_to_drop ... how many bytes of content should be removed from the
		                  back of the buffer.
	
	NOTE:
		The content of the buffer is not necesserily valid utf-8 or
		null terminated after calling this.

*******************************************************************************/

template<template<typename T> class Allocator>
void Buffer<Allocator>::rewind(size_t bytes_to_drop)
{
	_pos -= bytes_to_drop;
}

/* Buffer::zero_terminate ******************************************************

	Ensure content of buffer is a zero terminated string.

*******************************************************************************/

template<template<typename T> class Allocator>
Buffer<Allocator>& Buffer<Allocator>::zero_terminate()
{
	*_pos = '\0';

	return *this;
}

/* Buffer::ensure_capacity *****************************************************

	Automatically grows buffer if doesn't have enough space.

	IN:
		write_size ... the number of bytes that will be written by the next
		               write operation. Buffer will be resized if it doesn't
		               have enough space to satisfy that write.

*******************************************************************************/

template<template<typename T> class Allocator>
void Buffer<Allocator>::ensure_capacity(size_t write_size)
{
	size_t free_space = _end - _pos;

	if (free_space < write_size) {
		// increase capacity
		size_t old_size     = _pos - _start;
		size_t old_capacity = _end - _start;
		size_t new_capacity = (std::max(old_capacity, write_size) * 2) + 1;

		assert(new_capacity > (old_capacity + write_size));

		// enlarge buffer
		_start = alloc.reallocate(_start, old_capacity, new_capacity);
		_end   = _start + new_capacity;
		_pos   = _start + old_size;
	}
}


#endif /* __CACAO_BUFFER_HPP__ */

