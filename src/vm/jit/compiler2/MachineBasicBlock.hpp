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
#include "toolbox/ordered_list.hpp"


namespace cacao {
namespace jit {
namespace compiler2 {

// forward declarations
class MachineInstruction;

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
	typedef ordered_list<MachineInstruction*>::iterator iterator;
	/// construct an empty MachineBasicBlock
	MachineBasicBlock() {};
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
private:
	ordered_list<MachineInstruction*> list;
};

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
	list.insert_before(pos,value);
}
inline void MachineBasicBlock::insert_after(iterator pos, MachineInstruction* value) {
	list.insert_after(pos,value);
}
inline MachineBasicBlock::iterator MachineBasicBlock::begin() {
	return list.begin();
}
inline MachineBasicBlock::iterator MachineBasicBlock::end() {
	return list.end();
}

typedef MachineBasicBlock::iterator MIIterator;

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
