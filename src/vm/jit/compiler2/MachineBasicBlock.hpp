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

#include "toolbox/future.hpp"

#include <algorithm>
#include <list>

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declarations
class MachineInstruction;
class MachineBasicBlockImpl;

struct MIEntry {
	MIEntry(MachineInstruction* inst, std::size_t index) : inst(inst), index(index) {}
	shared_ptr<MachineInstruction> inst;
	std::size_t index;
};

/**
 * A MachineInstruction which is part of a schedule.
 * @ingroup low-level-ir
 */
class MIIterator : public std::iterator<std::bidirectional_iterator_tag,
															MachineInstruction*> {
private:
	typedef std::list<MIEntry>::iterator intern_iterator;
public:
	MIIterator() {}
	MIIterator(intern_iterator it) : it(it) {}
	MIIterator(const MIIterator& other) : it(other.it) {}
	MIIterator& operator++() {
		++it;
		return *this;
	}
	MIIterator operator++(int) {
		MIIterator tmp(*this);
		operator++();
		return tmp;
	}
	MIIterator& operator--() {
		--it;
		return *this;
	}
	MIIterator operator--(int) {
		MIIterator tmp(*this);
		operator--();
		return tmp;
	}
	bool operator==(const MIIterator& rhs) const { return it == rhs.it; }
	bool operator!=(const MIIterator& rhs) const { return it != rhs.it; }
	bool operator<( const MIIterator& rhs) const { return it->index < rhs.it->index; }
	bool operator>( const MIIterator& rhs) const { return rhs < *this; }
	MachineInstruction* operator*()        { return it->inst.get(); }
	MachineInstruction* operator*()  const { return it->inst.get(); }
	MachineInstruction* operator->()       { return it->inst.get(); }
	MachineInstruction* operator->() const { return it->inst.get(); }

private:
	intern_iterator it;
	friend class MachineBasicBlockImpl;
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
public:
	/// construct an empty MachineBasicBlock
	MachineBasicBlock();
	/// returns the number of elements
	std::size_t size() const;
	/// Appends the given element value to the end of the container.
	void push_back(MachineInstruction* value);
	/// inserts value to the beginning
	void push_front(MachineInstruction* value);
	/// inserts value before the element pointed to by pos
	void insert_before(MIIterator pos, MachineInstruction* value);
	/// inserts value after the element pointed to by pos
	void insert_after(MIIterator pos, MachineInstruction* value);
	/// returns an iterator to the beginning
	MIIterator begin();
	/// returns an iterator to the end
	MIIterator end();
private:
	MachineBasicBlock(MachineBasicBlockImpl* pImpl) : pImpl(pImpl) {}
	shared_ptr<MachineBasicBlockImpl> pImpl;
};

class MachineBasicBlockImpl {
private:
	std::list<MIEntry> list;

	struct IncrementMIEntry {
		void operator()(MIEntry& MIE) {
			++MIE.index;;
		}
	};
public:
	/// returns the number of elements
	std::size_t size() const {
		return list.size();
	}
	/// Appends the given element value to the end of the container.
	void push_back(MachineInstruction* value) {
		list.push_back(MIEntry(value,size()));
	}
	/// inserts value to the beginning
	void push_front(MachineInstruction* value) {
		std::for_each(list.begin(),list.end(),IncrementMIEntry());
		list.push_front(MIEntry(value,0));
	}

	/// inserts value before the element pointed to by pos
	void insert_before(MIIterator pos, MachineInstruction* value) {
		list.insert(pos.it,MIEntry(value,pos.it->index));
		std::for_each(pos.it,list.end(),IncrementMIEntry());
	}
	/// inserts value after the element pointed to by pos
	void insert_after(MIIterator pos, MachineInstruction* value) {
		insert_before(++pos,value);
	}
	/// returns an iterator to the beginning
	MIIterator begin() {
		return list.begin();
	}
	/// returns an iterator to the end
	MIIterator end() {
		return list.end();
	}
};

// MachineBasicBlock definitions

inline MachineBasicBlock::MachineBasicBlock() : pImpl(new MachineBasicBlockImpl) {}

inline std::size_t MachineBasicBlock::size() const {
	return pImpl->size();
}
inline void MachineBasicBlock::push_back(MachineInstruction* value) {
	pImpl->push_back(value);
}
inline void MachineBasicBlock::push_front(MachineInstruction* value) {
	pImpl->push_front(value);
}
inline void MachineBasicBlock::insert_before(MIIterator pos, MachineInstruction* value) {
	pImpl->insert_before(pos,value);
}
inline void MachineBasicBlock::insert_after(MIIterator pos, MachineInstruction* value) {
	pImpl->insert_after(pos,value);
}
inline MIIterator MachineBasicBlock::begin() {
	return pImpl->begin();
}
inline MIIterator MachineBasicBlock::end() {
	return pImpl->end();
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
