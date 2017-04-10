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

#include <algorithm>

#include "vm/jit/compiler2/StackSlotManager.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"
#include "vm/jit/executionstate.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

StackSlotManager::~StackSlotManager() {
	while (!slots.empty()) {
		ManagedStackSlot *slot = slots.back();
		slots.pop_back();
		delete slot;
	}

	while (!argument_slots.empty()) {
		ManagedStackSlot *slot = argument_slots.back();
		argument_slots.pop_back();
		delete slot;
	}
}

ManagedStackSlot* StackSlotManager::create_slot(Type::TypeID type) {
	slots.push_back(new ManagedStackSlot(this, type));
	return slots.back();
}

ManagedStackSlot* StackSlotManager::create_argument_slot(Type::TypeID type, u4 index) {
	number_of_machine_argument_slots = std::max(number_of_machine_argument_slots, index + 1);
	auto slot = new ManagedStackSlot(this, type);
	slot->set_index(index);
	argument_slots.push_back(slot);
	return slot;
}

void StackSlotManager::finalize() {
	u4 next_index = number_of_machine_argument_slots;
	for (auto slot : slots) {
		slot->set_index(next_index++);
	}
}

u4 StackSlotManager::get_frame_size() const {
	return get_number_of_machine_slots() * SIZE_OF_STACKSLOT;
}

u4 StackSlotManager::get_number_of_machine_slots() const {
	return number_of_machine_argument_slots + slots.size();
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
