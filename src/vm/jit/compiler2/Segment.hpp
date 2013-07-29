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

#include "toolbox/OStream.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declarations
class CodeMemory;

template <typename Tag, typename RefCategory>
class SegRef;

template <class Type>
class SegmentTag;

template <typename Tag>
struct classcomp {
	bool operator() (const SegmentTag<Tag> *lhs, const SegmentTag<Tag> *rhs) const {
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

struct NormalRefCategory {};
struct ReverseRefCategory {};


/**
 * A segment of the code memory
 */
template <typename Tag, typename RefCategory>
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
		IdxTy operator-(std::size_t i) const {
			return IdxTy(idx - i);
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
	typedef SegRef<Tag,RefCategory> Ref;
private:
	typedef typename std::map<SegmentTag<Tag>*,IdxTy,classcomp<Tag> > EntriesTy;
	CodeMemory *CM;
	std::vector<u1> content;             ///< content of the segment
	EntriesTy entries;                   ///< tagged entries

	/// invalid index
	static inline IdxTy invalid_index() { return IdxTy(-1); }

	/// insert tag
	IdxTy insert_tag(SegmentTag<Tag>* tag, IdxTy o) {
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
		if ( i == entries.end()) return invalid_index();
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

	/// is invalid index
	static inline bool is_invalid(IdxTy idx) { return idx == invalid_index(); }

	/**
	 * Get the start index of next Reference in line.
	 *
	 * Note that the index may or may not be already present. In the later
	 * case the the index of the future Reference will be returned
	 */
	IdxTy get_following_index() const {
		return getFollowingIndex(*this,RefCategory());
	}

	/**
	 * get start address
	 *
	 * @deprecated this should only be evalable in the closed variant
	 */
	u1* get_start() {
		return &content.front();
	}
	/**
	 * get end address
	 *
	 * @deprecated this should only be evalable in the closed variant
	 */
	u1* get_end() {
		return get_start() + size();
	}
	/// write content
	u1& operator[](IdxTy i) {
		return operator[](i.idx);
	}
	u1& operator[](std::size_t i) {
		assert(i >= 0);
		assert(i < content.size());
		return content[i];
	}
	/// get content
	u1 at(std::size_t i) const {
		assert(i < content.size());
		return content[i];
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
		return insert_tag(new Tag2(tag),ref.get_begin());
	}
	/// insert tag
	template<typename Tag2>
	IdxTy insert_tag(Tag2 tag) {
		return insert_tag(new Tag2(tag),get_following_index());
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

	friend class SegRef<Tag,RefCategory>;
};

/**
 * Segment reference
 *
 * Segment references used to write data into the Segment and for linking.
 * Segments are position independant.
 *
 * It can be used like an u1 array.
 */
template <typename Tag, typename RefCategory>
class SegRef {
public:
	typedef RefCategory ref_category;
	typedef typename Segment<Tag,RefCategory>::IdxTy IdxTy;
private:
	Segment<Tag,RefCategory> *parent;
	IdxTy index;
	std::size_t _size;
	SegRef(Segment<Tag,RefCategory>* parent, IdxTy index, std::size_t _size)
		: parent(parent), index(index), _size(_size) {}
public:
	/// Copy Constructor
	SegRef(const SegRef<Tag,RefCategory> &other) : parent(other.parent),
		index(other.index), _size(other.size()) {}
	/// Copy assignment operator
	SegRef& operator=(const SegRef<Tag,RefCategory> &other) {
		parent = other.parent;
		index = other.index;
		_size = other.size();
		return *this;
	}
	/// Get a sub-segment
	SegRef operator+(std::size_t v) {
		assert(v >= 0);
		assert(_size - v >= 0);
		return SegRef(parent, getAddStartIndex(*this,v,RefCategory()), _size - v);
	}
	/// access data
	u1& operator[](std::size_t i) {
		return doAccess(*this,i,RefCategory());
	}

	u1& operator[](IdxTy i) {
		return operator[](i.idx);
	}
	/// Get containing segment
	Segment<Tag,RefCategory>* get_Segment() const {
		return parent;
	}
	Segment<Tag,RefCategory>& get_Segment() {
		return *parent;
	}
	/**
	 * Get the index of the first element.
	 *
	 * Return the index of the first element with respect to RefCategory
	 * (i.e. the element written by operator[0]). Note that get_begin() might
	 * be greater than get_end() depending on RefCategory.
	 *
	 * @see get_end()
	 */
	IdxTy get_begin() const {
		return getBegin(*this,RefCategory());
	}
	/**
	 * Get the index of the first element after the reference.
	 *
	 * Return the index of the first element after the refenrence with respect
	 * to RefCategory  (i.e. the element that would be written by
	 * operator[size()], which is not allowed). Note that get_begin() might
	 * be greater than get_end() depending on RefCategory.
	 *
	 * @see get_begin()
	 */
	IdxTy get_end() const {
		return getEnd(*this,RefCategory());
	}
	/**
	 * Get the raw start index.
	 *
	 * This returns the begin (i.e. the lowest index) of the raw
	 * reference. It does not take the RefCategory into account. Also
	 * the assertion get_index_begin_raw() <= get_index_end_raw() always holds.
	 *
	 * In most cases the use of get_begin() and get_end() is preferable.
	 *
	 * @see get_index_end_raw()
	 * @see get_begin()
	 * @see get_end()
	 */
	IdxTy get_index_begin_raw() const {
		return index;
	}
	/**
	 * Get the raw end index.
	 *
	 * This returns the end (i.e. last index plus one) of the raw
	 * reference. It does not take the RefCategory into account. Also
	 * the assertion get_index_begin_raw() <= get_index_end_raw() always holds.
	 *
	 * In most cases the use of get_begin() and get_end() is preferable.
	 *
	 * @see get_index_begin_raw()
	 * @see get_begin()
	 * @see get_end()
	 */
	IdxTy get_index_end_raw() const {
		return index + _size;
	}
	/// size of the reference
	std::size_t size() const {
		return _size;
	}
	friend class Segment<Tag,RefCategory>;
};

// specialization for Normal Segments
#if 0
template <typename Tag>
inline u1& SegRef<Tag,NormalRefCategory>::operator[](std::size_t i) {
	assert(i < size());
	return (*parent)[index.idx + i];
}
// specialization for Normal Segments
template <typename Tag>
inline u1& SegRef<Tag,ReverseRefCategory>::operator[](std::size_t i) {
	assert(i < size());
	return (*parent)[index.idx + size() - i - 1];
}
#endif
/// access data
template <typename Tag, typename RefCategory>
inline u1&
doAccess(SegRef<Tag,RefCategory> &ref, std::size_t i, NormalRefCategory) {
	assert(i < ref.size());
	return ref.get_Segment()[ref.get_index_begin_raw().idx + i];
}
/// access data
template <typename Tag, typename RefCategory>
inline u1&
doAccess(SegRef<Tag,RefCategory> &ref, std::size_t i, ReverseRefCategory) {
	assert(i < ref.size());
	return ref.get_Segment()[ref.get_index_end_raw().idx - 1 - i];
}

/// get subreference
template <typename Tag, typename RefCategory>
inline typename SegRef<Tag,RefCategory>::IdxTy
getAddStartIndex(SegRef<Tag,RefCategory> &ref, std::size_t v, NormalRefCategory) {
	return ref.get_index_begin_raw() + v;
}
/// get subreference
template <typename Tag, typename RefCategory>
inline typename SegRef<Tag,RefCategory>::IdxTy
getAddStartIndex(SegRef<Tag,RefCategory> &ref, std::size_t v, ReverseRefCategory) {
	return ref.get_index_begin_raw();
}

/// get first index
template <typename Tag, typename RefCategory>
inline typename SegRef<Tag,RefCategory>::IdxTy
getBegin(const SegRef<Tag,RefCategory> &ref, NormalRefCategory) {
	return ref.get_index_begin_raw();
}
/// get first index
template <typename Tag, typename RefCategory>
inline typename SegRef<Tag,RefCategory>::IdxTy
getBegin(const SegRef<Tag,RefCategory> &ref, ReverseRefCategory) {
	return ref.get_index_end_raw() -1;
}

/// get last but one index
template <typename Tag, typename RefCategory>
inline typename SegRef<Tag,RefCategory>::IdxTy
getEnd(const SegRef<Tag,RefCategory> &ref, NormalRefCategory) {
	return ref.get_index_end_raw();
}
/// get last but one index
template <typename Tag, typename RefCategory>
inline typename SegRef<Tag,RefCategory>::IdxTy
getEnd(const SegRef<Tag,RefCategory> &ref, ReverseRefCategory) {
	return ref.get_index_begin_raw() -1;
}

// get last index of a segment
template <typename Tag, typename RefCategory>
inline typename Segment<Tag,RefCategory>::IdxTy
getFollowingIndex(const Segment<Tag,RefCategory> &ref, NormalRefCategory) {
	return typename Segment<Tag,RefCategory>::IdxTy(ref.size());
}
// get last index of a segment
template <typename Tag, typename RefCategory>
inline typename Segment<Tag,RefCategory>::IdxTy
getFollowingIndex(const Segment<Tag,RefCategory> &ref, ReverseRefCategory) {
	return typename Segment<Tag,RefCategory>::IdxTy(ref.size() - 1);
}

template <class Type>
class SegmentTag {
protected:
	SegmentTag(Type type) : type(type) {}
	virtual u8 hash() const = 0;
public:
	bool operator==(const SegmentTag& other) const {
		return type == other.type && hash() == other.hash();
	}
	bool operator<(const SegmentTag& other) const {
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
		return OS << "PointerTag Type: " << t << " ptr: " << ptr;
	}
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
