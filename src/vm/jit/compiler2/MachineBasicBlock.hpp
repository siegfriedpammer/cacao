/* src/vm/jit/compiler2/MachineBasicBlock.hpp - MachineBasicBlock

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

#ifndef _JIT_COMPILER2_MACHINEBASICBLOCK
#define _JIT_COMPILER2_MACHINEBASICBLOCK

#include "vm/jit/compiler2/MachineInstructionSchedule.hpp"
#include "toolbox/ordered_list.hpp"

namespace cacao {

// forward declarations
class OStream;

namespace jit {
namespace compiler2 {

// forward declarations
class MachineInstruction;
class MachineInstructionSchedule;

class MIIterator {
	typedef ordered_list<MachineInstruction*>::iterator iterator;
	typedef MBBIterator block_iterator;
	block_iterator block_it;
	iterator it;
public:
	typedef iterator::reference reference;
	typedef iterator::pointer pointer;

	MIIterator(const block_iterator &block_it, const iterator& it)
		: block_it(block_it), it(it) {}
	MIIterator(const MIIterator& other)
		: block_it(other.block_it), it(other.it) {}
	MIIterator& operator++();
	MIIterator& operator--();
	MIIterator operator++(int) {
		MIIterator tmp(*this);
		operator++();
		return tmp;
	}
	MIIterator operator--(int) {
		MIIterator tmp(*this);
		operator--();
		return tmp;
	}
	bool operator==(const MIIterator& rhs) const {
		if (block_it == rhs.block_it)
			return it == rhs.it;
		return false;
	}
	bool operator<( const MIIterator& rhs) const {
		if (block_it == rhs.block_it)
			return it < rhs.it;
		return block_it < rhs.block_it;
	}
	bool operator!=(const MIIterator& rhs) const { return !(*this == rhs); }
	bool operator>( const MIIterator& rhs) const { return rhs < *this; }
	reference       operator*()        { return *it; }
	const reference operator*()  const { return *it; }
	pointer         operator->()       { return &*it; }
	const pointer   operator->() const { return &*it; }

	friend class MachineBasicBlock;
};

/**
 * A basic block of (scheduled) machine instructions.
 *
 * A MachineBasicBlock contains an ordered collection of
 * MachineInstructions. These MachineInstructions can be accessed via
 * MIIterators. These "smart-iterators" are not only used for access
 * but also for ordering MachineInstructions.
 *
 * Once a MachineInstruction is added, the MachineBasicBlock takes over
 * the responsability for deleting it.
 *
 * @ingroup low-level-ir
 */
class MachineBasicBlock {
private:
	static std::size_t id_counter;
	std::size_t id;
	MBBIterator my_it;
	/// empty constructor
	MachineBasicBlock() : id(id_counter++) {}
public:
	typedef ordered_list<MachineInstruction*>::iterator iterator;
	typedef ordered_list<MachineInstruction*>::const_iterator const_iterator;
	typedef ordered_list<MachineInstruction*>::reference reference;
	typedef ordered_list<MachineInstruction*>::const_reference const_reference;
	typedef ordered_list<MachineInstruction*>::pointer pointer;
	typedef ordered_list<MachineInstruction*>::const_pointer const_pointer;
	/// construct an empty MachineBasicBlock
	MachineBasicBlock(const MBBIterator &my_it)
		: id(id_counter++),  my_it(my_it) {};
	/// returns the number of elements
	std::size_t size() const;
	/// Appends the given element value to the end of the container.
	void push_back(MachineInstruction* value);
	/// inserts value to the beginning
	void push_front(MachineInstruction* value);
	/// inserts value before the element pointed to by pos
	void insert_before(iterator pos, MachineInstruction* value);
	/// inserts value after the element pointed to by pos
	void insert_after(iterator pos, MachineInstruction* value);
	/// returns an iterator to the beginning
	iterator begin();
	/// returns an iterator to the end
	iterator end();
	/// returns an const iterator to the beginning
	const_iterator begin() const;
	/// returns an const iterator to the end
	const_iterator end() const;
	/// access the first element
	const_reference front() const;
	/// access the last element
	const_reference back() const;
	/// access the first element
	reference front();
	/// access the last element
	reference back();

	/// get a MIIterator form a interator
	MIIterator convert(iterator pos);
	/// get self iterator
	MBBIterator self_iterator() const;
	/// print
	OStream& print(OStream& OS) const;
private:
	ordered_list<MachineInstruction*> list;
	friend class MBBBuilder;
	friend class MachineInstructionSchedule;
};

inline MIIterator& MIIterator::operator++() {
	++it;
	if (it == (*block_it)->end()) {
		// end of basic block
		++block_it;
		if (block_it != block_it.get_parent()->end()) {
			it = (*block_it)->begin();
		}
	}
	return *this;
}
inline MIIterator& MIIterator::operator--() {
	if (it == (*block_it)->begin()) {
		// begin of basic block
		if (block_it == block_it.get_parent()->begin()) {
			return *this;
		}
		--block_it;
		it = (*block_it)->end();
	}
	--it;
	return *this;
}


inline std::size_t MachineBasicBlock::size() const {
	return list.size();
}
inline void MachineBasicBlock::push_back(MachineInstruction* value) {
	list.push_back(value);
}
inline void MachineBasicBlock::push_front(MachineInstruction* value) {
	list.push_front(value);
}
inline void MachineBasicBlock::insert_before(iterator pos, MachineInstruction* value) {
	list.insert(pos,value);
}
inline void MachineBasicBlock::insert_after(iterator pos, MachineInstruction* value) {
	list.insert(++pos,value);
}
inline MachineBasicBlock::iterator MachineBasicBlock::begin() {
	return list.begin();
}
inline MachineBasicBlock::iterator MachineBasicBlock::end() {
	return list.end();
}
inline MachineBasicBlock::const_iterator MachineBasicBlock::begin() const {
	return list.begin();
}
inline MachineBasicBlock::const_iterator MachineBasicBlock::end() const {
	return list.end();
}
inline MachineBasicBlock::const_reference MachineBasicBlock::front() const {
	return list.front();
}
inline MachineBasicBlock::const_reference MachineBasicBlock::back() const {
	return list.back();
}
inline MachineBasicBlock::reference MachineBasicBlock::front() {
	return list.front();
}
inline MachineBasicBlock::reference MachineBasicBlock::back() {
	return list.back();
}
inline MIIterator MachineBasicBlock::convert(iterator pos) {
	if (pos == end()) {
		return ++convert(--pos);
	}
	return MIIterator(my_it,pos);
}
inline MBBIterator MachineBasicBlock::self_iterator() const {
	return my_it;
}

inline OStream& operator<<(OStream& OS, MachineBasicBlock& MBB) {
	return MBB.print(OS);
}

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MACHINEBASICBLOCK */


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
