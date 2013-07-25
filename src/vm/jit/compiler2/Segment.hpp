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
#include <algorithm>

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
	/**
	 * Prevent mixing indices.
	 *
	 * See Item 18 in "Efficient C++" @cite Meyers2005.
	 */
	struct IdxTy {
		std::size_t idx;

		/// default constructor
		IdxTy() : idx(-1) {}
		/// explicit constructor
		explicit IdxTy(std::size_t idx) : idx(idx) {}
		/// copy assignment operator
		IdxTy& operator=(const IdxTy &other) {
			idx = other.idx;
			return *this;
		}
		IdxTy operator+(std::size_t i) const {
			return IdxTy(idx + i);
		}
		bool operator==(const IdxTy &other) const {
			return idx == other.idx;
		}
		#if 0
		bool operator==(std::size_t other) const {
			return idx == other;
		}
		#endif
	};
	typedef SegRef<Tag> Ref;
private:
	typedef typename std::map<SegmentTag<Tag>*,IdxTy,classcomp<Tag> > EntriesTy;
	CodeMemory *CM;
	std::vector<u1> content;             ///< content of the segment
	EntriesTy entries;                   ///< tagged entries

	u1& operator[](IdxTy i) {
		return operator[](i.idx);
	}
	u1& operator[](std::size_t i) {
		assert(i >= 0);
		assert(i < content.size());
		return content[i];
	}
	/// insert tag
	IdxTy insert_tag(SegmentTag<Tag>* tag, IdxTy o) {
		LOG2("Segment: insert " << *tag << nl);
		entries[tag] = o;
		return o;
	}
	/// contains tag
	bool contains_tag_intern(SegmentTag<Tag>* tag) const {
		return entries.find(tag) != entries.end();
	}
	/// get index
	IdxTy get_index_intern(SegmentTag<Tag>* tag) const {
		typename EntriesTy::const_iterator i = entries.find(tag);
		if ( i != entries.end()) return end();
		return i->second;
	}
public:
	/// Constructor
	Segment(CodeMemory *CM) : CM(CM), content() {}
	/// Constructor with default capacity
	Segment(CodeMemory *CM, IdxTy capacity) : CM(CM), content() {
		content.reserve(capacity);
	}
	/// Get containing CodeMemory
	CodeMemory* get_CodeMemory() const { return CM; }
	CodeMemory& get_CodeMemory() { return *CM; }

	/**
	 * get size
	 *
	 * @deprecated this should only be evalable in the closed variant
	 */
	std::size_t size() const { return content.size(); }
	/// get end
	IdxTy end() const { return IdxTy(-1); }

	/**
	 * get start address
	 *
	 * @deprecated this should only be evalable in the closed variant
	 */
	u1* get_start() {
		return &content.front();
	}
	/**
	 * reverse content
	 *
	 * @deprecated this should only be evalable in the closed variant
	 */
	void reverse() {
		std::reverse(content.begin(), content.end());
	}
	

	/// get a reference to the segment
	Ref get_Ref(std::size_t t) {
		std::size_t start = content.size();
		content.resize(start + t);
		return Ref(this,IdxTy(start),t);
	}

	/// insert tag
	template<typename Tag2>
	IdxTy insert_tag(Tag2 tag, const Ref &ref) {
		return insert_tag(new Tag2(tag),ref.get_index());
	}
	/// insert tag
	template<typename Tag2>
	IdxTy insert_tag(Tag2 tag) {
		return insert_tag(new Tag2(tag),IdxTy(content.size()));
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
	 * @return the index of the tag or end() if not found
	 */
	template<typename Tag2>
	IdxTy get_index(Tag2 tag) const {
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
public:
	typedef typename Segment<Tag>::IdxTy IdxTy;
private:
	Segment<Tag> *parent;
	IdxTy index;
	std::size_t _size;
	SegRef(Segment<Tag>* parent, IdxTy index, std::size_t _size)
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
	SegRef operator+(IdxTy v) {
		return operator+(v.idx);
	}
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
		return (*parent)[index.idx + i];
	}
	u1& operator[](IdxTy i) {
		return operator[](i.idx);
	}
	/// Get containing segment
	Segment<Tag>* get_Segment() const {
		return parent;
	}
	Segment<Tag>& get_Segment() {
		return *parent;
	}
	/// Get index
	IdxTy get_index() const {
		return index;
	}
	/// size of the reference
	std::size_t size() const {
		return _size;
	}
	friend class Segment<Tag>;
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


template <class Type, class ConstTy, Type t>
class ConstTag : public SegmentTag<Type> {
protected:
	virtual u8 hash() const {
		if (sizeof(ConstTy) == sizeof(u1))
			return u8(*reinterpret_cast<const u1*>(&c));
		if (sizeof(ConstTy) == sizeof(u2))
			return u8(*reinterpret_cast<const u2*>(&c));
		if (sizeof(ConstTy) == sizeof(u4))
			return u8(*reinterpret_cast<const u4*>(&c));
		return *reinterpret_cast<const u8*>(&c);
	}
public:
	ConstTag(ConstTy c) : SegmentTag<Type>(t), c(c) {}
	virtual OStream& print(OStream &OS) const {
		return OS << "ConstTag Type: " << t << " ptr: " << c;
	}
private:
	ConstTy c;
};

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
