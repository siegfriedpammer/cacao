/* src/vm/jit/compiler2/MachineInstructionSchedule.cpp - MachineInstructionSchedule

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

#include "vm/jit/compiler2/MachineInstructionSchedule.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

MBBBuilder::MBBBuilder() : MBB(new MachineBasicBlock()) {}

MachineInstructionSchedule::iterator
MachineInstructionSchedule::push_back(const MBBBuilder& value) {
	list.push_back(value.MBB);
	return value.MBB->my_it = --end();
}
MachineInstructionSchedule::iterator
MachineInstructionSchedule::push_front(const MBBBuilder& value) {
	list.push_front(value.MBB);
	return value.MBB->my_it = begin();
}
MachineInstructionSchedule::iterator
MachineInstructionSchedule::insert_before(iterator pos,
		const MBBBuilder& value) {
	list.insert(pos.it,value.MBB);
	return value.MBB->my_it = --pos;
}

MIIterator MachineInstructionSchedule::mi_begin() {
	if (empty()) return mi_end();
	// Note: by convention MachineBasicBlock blocks are not
	// allowed to be empty. Therefor we can safely assume
	// that front()->begin() != front()->end().
	return MIIterator(begin(),front()->begin());
}
MIIterator MachineInstructionSchedule::mi_end() {
  return MIIterator(end());
}

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


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
