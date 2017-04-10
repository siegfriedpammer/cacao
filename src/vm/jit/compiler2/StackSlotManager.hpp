/* src/vm/jit/compiler2/StackSlotManager.hpp - StackSlotManager

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

#ifndef _JIT_COMPILER2_STACKSLOTMANAGER
#define _JIT_COMPILER2_STACKSLOTMANAGER

#include "vm/jit/compiler2/Type.hpp"
#include "vm/types.hpp"

#include "vm/jit/compiler2/alloc/map.hpp"
#include "vm/jit/compiler2/alloc/vector.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declaration
class ManagedStackSlot;
class StackSlot;

/**
 * StackSlotManager
 *
 * The StackSlotManger is used to manage slots for spilled registers etc.
 */
class StackSlotManager {
private:
	typedef alloc::vector<ManagedStackSlot*>::type SlotListTy;

	SlotListTy slots;
	SlotListTy argument_slots;

	/**
	 * The number of argument slots at machine-level.
	 */
	u4 number_of_machine_argument_slots;

public:
	StackSlotManager() : number_of_machine_argument_slots(0) {}
	~StackSlotManager();

	/**
	 * Create a ManagedStackSlot.
	 *
	 * @param type The slot's type.
	 *
	 * @return The new stack slot.
	 */
	ManagedStackSlot* create_slot(Type::TypeID type);

	/**
	 * Create a ManagedStackSlot for an invocation argument.
	 *
	 * @param type  The slot's type.
	 * @param index The index of the invocation argument for which this slot is used.
	 *
	 * @return The new stack slot.
	 */
	ManagedStackSlot* create_argument_slot(Type::TypeID type, u4 index);

	/**
	 * Assigns each ManagedStackSlot a position in the virtual frame.
	 */
	void finalize();

	/**
	 * @return The size of the stack frame in bytes.
	 */
	u4 get_frame_size() const;

	/**
	 * @return The number of actual machine-level slots.
	 */
	u4 get_number_of_machine_slots() const;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_STACKSLOTMANAGER */


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
