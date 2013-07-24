/* src/vm/jit/compiler2/Segment.hpp - Segment

   Copyright (C) 2013
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

#ifndef _JIT_COMPILER2_SEGMENT
#define _JIT_COMPILER2_SEGMENT

#include "vm/types.hpp"
#include <cassert>
#include <vector>

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declarations
class CodeMemory;
template <typename Tag>
class SegRef;

/**
 * A segment of the code memory
 */
template <typename Tag>
class Segment {
public:
	typedef SegRef<Tag> Ref;
private:
	CodeMemory *CM;
	std::vector<u1> content; //< content of the segment

	u1& operator[](std::size_t i) {
		assert(i >= 0);
		assert(i < content.size());
		return content[i];
	}
public:
	/// Constructor
	Segment(CodeMemory *CM) : CM(CM), content() {}
	/// Constructor with default capacity
	Segment(CodeMemory *CM, std::size_t capacity) : CM(CM), content() {
		content.reserve(capacity);
	}
	/// Get containing CodeMemory
	CodeMemory* get_CodeMemory() const { return CM; }

	/// get a reference to the segment
	Ref get_Ref(std::size_t t) {
		std::size_t start = content.size();
		content.resize(start + t);
		return Ref(this,start,t);
	}

	virtual ~Segment() {}

	friend class SegRef<Tag>;
};

/**
 * Segment reference
 *
 * Segment references used to write data into the Segment and for linking.
 * Segments are position independant.
 *
 * It can be used like an u1 array.
 */
template <typename Tag>
class SegRef {
private:
	Segment<Tag> *parent;
	std::size_t offset;
	std::size_t _size;
	SegRef(Segment<Tag>* parent, std::size_t offset, std::size_t _size)
		: parent(parent), offset(offset), _size(_size) {}
public:
	/// Copy Constructor
	SegRef(const SegRef<Tag> &other) : parent(other.parent),
		offset(other.offset), _size(other.size()) {}
	/// Copy assignment operator
	SegRef& operator=(const SegRef<Tag> &other) {
		parent = other.parent;
		offset = other.offset;
		_size = other.size();
		return *this;
	}
	/// Get a sub-segment
	SegRef operator+(std::size_t v) {
		assert(v >= 0);
		assert(_size - v >= 0);
		SegRef CF(parent, offset + v, _size - v);
		return CF;
	}
	/// access data
	u1& operator[](std::size_t i) {
		assert(i >= 0);
		assert(i < size());
		return (*parent)[offset + i];
	}
	/// Get containing segment
	Segment<Tag>* get_Segment() const {
		return parent;
	}
	/// size of the reference
	std::size_t size() const {
		return _size;
	}
};

template <class Type>
class SegmentTag {
protected:
	SegmentTag(Type type) : type(type) {}
	virtual u8 hash() const = 0;
public:
	bool operator==(const SegmentTag& other) const {
		return type == other.type && hash() == other.hash();
	}
private:
	Type type;
};

template <class Type, class Ptr, Type t>
class PointerTag : public SegmentTag<Type> {
protected:
	virtual u8 hash() const {
		return u8(ptr);
	}
public:
	PointerTag(Ptr *ptr) : SegmentTag<Type>(t), ptr(ptr) {}
private:
	Ptr *ptr;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_SEGMENT */


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
