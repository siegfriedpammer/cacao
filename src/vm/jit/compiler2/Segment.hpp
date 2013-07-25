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
#include <map>

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/Segment"

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declarations
class CodeMemory;

template <typename Tag>
class SegRef;

template <class Type>
class SegmentTag;

template <typename Tag>
struct classcomp {
	bool operator() (const SegmentTag<Tag> *lhs, const SegmentTag<Tag> *rhs) const {
		LOG2("classcomp"<<nl);
		return *lhs<*rhs;
	}
};

#if 0
template <typename Tag>
bool operator<(SegmentTag<Tag> *lhs, SegmentTag<Tag> *rhs) {
	LOG2("non-member Segment::operator<"<<nl);
	return (*lhs) < (*rhs);
}

template <typename Tag>
bool operator==(SegmentTag<Tag> *lhs, SegmentTag<Tag> *rhs) {
	LOG2("non-member Segment::operator=="<<nl);
	return (*lhs) == (*rhs);
}
#endif

/**
 * A segment of the code memory
 */
template <typename Tag>
class Segment {
public:
	typedef SegRef<Tag> Ref;
private:
	typedef typename std::map<SegmentTag<Tag>*,std::size_t,classcomp<Tag> > EntriesTy;
	CodeMemory *CM;
	std::vector<u1> content;             ///< content of the segment
	EntriesTy entries;                   ///< tagged entries

	u1& operator[](std::size_t i) {
		assert(i >= 0);
		assert(i < content.size());
		return content[i];
	}
	/// insert tag
	void insert_tag(SegmentTag<Tag>* tag, std::size_t o) {
		LOG2("Segment: insert " << *tag << nl);
		entries[tag] = o;
	}
	/// contains tag
	bool contains_tag_intern(SegmentTag<Tag>* tag) const {
		return entries.find(tag) != entries.end();
	}
	/// get index
	bool get_index_intern(SegmentTag<Tag>* tag) const {
		typename EntriesTy::const_iterator i = entries.find(tag);
		if ( i != entries.end()) return entries.size();
		return i->second;
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

	/// insert tag
	template<typename Tag2>
	void insert_tag(Tag2 tag, const Ref &ref) {
		insert_tag(new Tag2(tag),ref.get_index());
	}
	/// insert tag
	template<typename Tag2>
	void insert_tag(Tag2 tag) {
		insert_tag(new Tag2(tag),content.size());
	}
	#if 0
	/// insert tag
	void insert_tag(SegmentTag<Tag>* tag, const Ref &ref) {
		insert_tag(tag,ref.get_index());
	}
	/// insert tag
	void insert_tag(SegmentTag<Tag>* tag) {
		insert_tag(tag,content.size());
	}
	#endif
	/// contains tag
	template<typename Tag2>
	bool contains_tag(Tag2 tag) const {
		return contains_tag_intern(&tag);
	}
	/**
	 * get the index of a tag
	 *
	 * @return the index of the tag or size() if not found
	 */
	template<typename Tag2>
	std::size_t get_index(Tag2 tag) const {
		return get_index_intern(&tag);
	}
	#if 0
	bool contains_tag(const SegmentTag<Tag> &tag) const {
		return entries.find(&tag) != entries.end();
	}
	#endif

	OStream& print(OStream &OS) const {
		for(typename EntriesTy::const_iterator i = entries.begin(), e = entries.end() ;
				i != e ; ++i) {
			OS << "key: ";
			i->first->print(OS);
			OS << " value: " << i->second << nl;
		}
		return OS;
	}

	virtual ~Segment() {
		for (typename EntriesTy::iterator i= entries.begin(), e = entries.end();
				i != e; ++i) {
			delete i->first;
		}
	}

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
	std::size_t index;
	std::size_t _size;
	SegRef(Segment<Tag>* parent, std::size_t index, std::size_t _size)
		: parent(parent), index(index), _size(_size) {}
public:
	/// Copy Constructor
	SegRef(const SegRef<Tag> &other) : parent(other.parent),
		index(other.index), _size(other.size()) {}
	/// Copy assignment operator
	SegRef& operator=(const SegRef<Tag> &other) {
		parent = other.parent;
		index = other.index;
		_size = other.size();
		return *this;
	}
	/// Get a sub-segment
	SegRef operator+(std::size_t v) {
		assert(v >= 0);
		assert(_size - v >= 0);
		SegRef CF(parent, index + v, _size - v);
		return CF;
	}
	/// access data
	u1& operator[](std::size_t i) {
		assert(i >= 0);
		assert(i < size());
		return (*parent)[index + i];
	}
	/// Get containing segment
	Segment<Tag>* get_Segment() const {
		return parent;
	}
	/// Get index
	std::size_t get_index() const {
		return index;
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
		LOG2("Segment::operator=="<<nl);
		return type == other.type && hash() == other.hash();
	}
	bool operator<(const SegmentTag& other) const {
		LOG2("Segment::operator<"<<nl);
		if (type == other.type)
			return hash() < other.hash();
		return type < other.type;
	}
	virtual OStream& print(OStream &OS) const = 0;
	virtual ~SegmentTag() {};
private:
	Type type;
};

template <class Type>
OStream& operator<<(OStream &OS, SegmentTag<Type> &tag) {
	return tag.print(OS);
}
template <class Type>
inline OStream& operator<<(OStream &OS, SegmentTag<Type> *tag) {
	return OS << *tag;
}


template <class Type, class Ptr, Type t>
class PointerTag : public SegmentTag<Type> {
protected:
	virtual u8 hash() const {
		return u8(ptr);
	}
public:
	PointerTag(Ptr *ptr) : SegmentTag<Type>(t), ptr(ptr) {}
	virtual OStream& print(OStream &OS) const {
		return OS << "PointerTag Type: " << t << " ptr: " << *ptr;
	}
private:
	Ptr *ptr;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#undef DEBUG_NAME

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
