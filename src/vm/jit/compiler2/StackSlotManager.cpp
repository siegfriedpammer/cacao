/* src/vm/jit/compiler2/StackSlotManager.cpp - StackSlotManager

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

#include "vm/jit/compiler2/StackSlotManager.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

StackSlotManager::~StackSlotManager() {
	for( StackSlotListTy::iterator i = slots.begin(),
			e = slots.begin(); i != e; ) {
		delete i->first;
		delete i->second;
	}
}

ManagedStackSlot* StackSlotManager::create_ManagedStackSlot() {
	static s4 counter = 1;
	ManagedStackSlot* mslot = new ManagedStackSlot(this);
	slots.insert(std::make_pair(mslot, new StackSlot(-counter)));
	counter++;
	return mslot;
}

u4 StackSlotManager::get_frame_size() const {
	// TODO compress stack
	// for now we only us 8 byte slots
	return slots.size() * 8;
}

StackSlot* StackSlotManager::get_StackSlot(ManagedStackSlot* MSS) {
	StackSlotListTy::const_iterator i = slots.find(MSS);
	if (i == slots.end()) {
		return NULL;
	}
	return i->second;
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
