/* src/vm/jit/compiler2/MachineInstructionSchedule.hpp - MachineInstructionSchedule

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

#ifndef _JIT_COMPILER2_MACHINEINSTRUCTIONSCHEDULE
#define _JIT_COMPILER2_MACHINEINSTRUCTIONSCHEDULE

#include "toolbox/ordered_list.hpp"

#include <map>
#include <list>
#include <vector>
#include <cassert>

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declarations
class MachineBasicBlock;
class MachineInstructionSchedule;
class MIIterator;

class MBBIterator {
	typedef ordered_list<MachineBasicBlock*>::iterator iterator;
	MachineInstructionSchedule *parent;
	iterator it;
	/// empty constructor
	MBBIterator() {}
public:
	typedef iterator::reference reference;
	typedef iterator::pointer pointer;
	typedef iterator::iterator_category iterator_category;
	typedef iterator::value_type value_type;
	typedef iterator::difference_type difference_type;

	MBBIterator(MachineInstructionSchedule *parent, const iterator &it)
		: parent(parent), it(it) {}
	MBBIterator(const MBBIterator& other) : parent(other.parent), it(other.it) {}
	MBBIterator& operator++() {
		++it;
		return *this;
	}
	MachineInstructionSchedule* get_parent() const { return parent; }
	MBBIterator operator++(int) {
		MBBIterator tmp(*this);
		operator++();
		return tmp;
	}
	MBBIterator& operator--() {
		--it;
		return *this;
	}
	MBBIterator operator--(int) {
		MBBIterator tmp(*this);
		operator--();
		return tmp;
	}
	bool operator==(const MBBIterator& rhs) const {
		assert(parent == rhs.parent);
		return it == rhs.it;
	}
	bool operator<( const MBBIterator& rhs) const {
		assert(parent == rhs.parent);
		return it < rhs.it;
	}
	bool operator!=(const MBBIterator& rhs) const { return !(*this == rhs); }
	bool operator>( const MBBIterator& rhs) const { return rhs < *this; }
	reference       operator*()        { return *it; }
	const reference operator*()  const { return *it; }
	pointer         operator->()       { return &*it; }
	const pointer   operator->() const { return &*it; }

	friend class MachineInstructionSchedule;
	friend class MachineBasicBlock;
	friend class const_MBBIterator;
};

class const_MBBIterator {
	typedef ordered_list<MachineBasicBlock*>::const_iterator const_iterator;
	const MachineInstructionSchedule *parent;
	const_iterator it;
	/// empty constructor
	const_MBBIterator() {}
public:
	typedef const_iterator::reference reference;
	typedef const_iterator::pointer pointer;
	typedef const_iterator::iterator_category iterator_category;
	typedef const_iterator::value_type value_type;
	typedef const_iterator::difference_type difference_type;

	const_MBBIterator(const MachineInstructionSchedule *parent,
		const const_iterator &it) : parent(parent), it(it) {}
	const_MBBIterator(const const_MBBIterator& other) : parent(other.parent),
		it(other.it) {}
	const_MBBIterator(const MBBIterator& other) : parent(other.parent),
		it(other.it) {}
	const_MBBIterator& operator++() {
		++it;
		return *this;
	}
	const MachineInstructionSchedule* get_parent() const { return parent; }
	const_MBBIterator operator++(int) {
		const_MBBIterator tmp(*this);
		operator++();
		return tmp;
	}
	const_MBBIterator& operator--() {
		--it;
		return *this;
	}
	const_MBBIterator operator--(int) {
		const_MBBIterator tmp(*this);
		operator--();
		return tmp;
	}
	bool operator==(const const_MBBIterator& rhs) const {
		assert(parent == rhs.parent);
		return it == rhs.it;
	}
	bool operator<( const const_MBBIterator& rhs) const {
		assert(parent == rhs.parent);
		return it < rhs.it;
	}
	bool operator!=(const const_MBBIterator& rhs) const { return !(*this == rhs); }
	bool operator>( const const_MBBIterator& rhs) const { return rhs < *this; }
	reference       operator*()        { return *it; }
	const reference operator*()  const { return *it; }
	pointer         operator->()       { return &*it; }
	const pointer   operator->() const { return &*it; }

	friend class MachineInstructionSchedule;
	friend class MachineBasicBlock;
};

class MBBBuilder {
private:
	MachineBasicBlock *MBB;
public:
	MBBBuilder();
	friend class MachineInstructionSchedule;
};

class MachineInstructionSchedule {
public:
	typedef MBBIterator iterator;
	typedef const_MBBIterator const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	typedef iterator::reference reference;
	/// construct an empty MachineInstructionSchedule
	MachineInstructionSchedule() {};
	/// checks if the schedule has no elements.
	bool empty() const;
	/// returns the number of elements
	std::size_t size() const;
	/// Appends the given element value to the end of the container.
	MachineInstructionSchedule::iterator push_back(const MBBBuilder& value);
	/// inserts value to the beginning
	MachineInstructionSchedule::iterator push_front(const MBBBuilder& value);
	/// inserts value before the element pointed to by pos
	MachineInstructionSchedule::iterator insert_before(iterator pos, const MBBBuilder& value);
	/// inserts value after the element pointed to by pos
	MachineInstructionSchedule::iterator insert_after(iterator pos, const MBBBuilder& value);
	/// returns an iterator to the beginning
	iterator begin();
	/// returns an iterator to the end
	iterator end();
	/// returns an const iterator to the beginning
	const_iterator begin() const;
	/// returns an const iterator to the end
	const_iterator end() const;

	/// returns an reverse_iterator to the beginning
	reverse_iterator rbegin();
	/// returns an reverse_iterator to the end
	reverse_iterator rend();
	/// returns an const reverse_iterator to the beginning
	const_reverse_iterator rbegin() const;
	/// returns an const reverse_iterator to the end
	const_reverse_iterator rend() const;
	/// access the first element
	reference front();
	/// access the last element
	reference back();

	/// returns an const MIIterator to the beginning
	MIIterator mi_begin();
	/// returns an const MIIterator to the end
	MIIterator mi_end();
private:
	ordered_list<MachineBasicBlock*> list;
};

inline bool MachineInstructionSchedule::empty() const {
	return list.empty();
}
inline std::size_t MachineInstructionSchedule::size() const {
	return list.size();
}
inline MachineInstructionSchedule::iterator MachineInstructionSchedule::insert_after(iterator pos, const MBBBuilder& value) {
	return insert_before(++pos,value);
}
inline MachineInstructionSchedule::iterator
MachineInstructionSchedule::begin() {
	return iterator(this,list.begin());
}
inline MachineInstructionSchedule::iterator
MachineInstructionSchedule::end() {
	return iterator(this,list.end());
}
inline MachineInstructionSchedule::reference MachineInstructionSchedule::front() {
	return list.front();
}
inline MachineInstructionSchedule::reference MachineInstructionSchedule::back() {
	return list.back();
}
inline MachineInstructionSchedule::const_iterator
MachineInstructionSchedule::begin() const {
	return const_iterator(this,list.begin());
}
inline MachineInstructionSchedule::const_iterator
MachineInstructionSchedule::end() const {
	return const_iterator(this,list.end());
}

inline MachineInstructionSchedule::reverse_iterator
MachineInstructionSchedule::rbegin() {
	return reverse_iterator(iterator(this,list.end()));
}
inline MachineInstructionSchedule::reverse_iterator
MachineInstructionSchedule::rend() {
	return reverse_iterator(iterator(this,list.begin()));
}
inline MachineInstructionSchedule::const_reverse_iterator
MachineInstructionSchedule::rbegin() const {
	return const_reverse_iterator(const_iterator(this,list.end()));
}
inline MachineInstructionSchedule::const_reverse_iterator
MachineInstructionSchedule::rend() const {
	return const_reverse_iterator(const_iterator(this,list.begin()));
}

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MACHINEINSTRUCTIONSCHEDULE */


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
