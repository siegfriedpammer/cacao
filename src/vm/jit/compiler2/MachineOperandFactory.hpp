/* src/vm/jit/compiler2/MachineOperandFactory.hpp - MachineOperandFactory

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

#ifndef _JIT_COMPILER2_MACHINEOPERANDFACTORY
#define _JIT_COMPILER2_MACHINEOPERANDFACTORY

#include <memory>

#include "vm/jit/compiler2/Type.hpp"
#include "vm/jit/compiler2/MachineOperandSet.hpp"
#include "vm/jit/compiler2/memory/Manager.hpp"
#include "vm/types.hpp"

MM_MAKE_NAME(MachineOperandFactory)

namespace cacao {
namespace jit {
namespace compiler2 {

class MachineOperand;
class ManagedStackSlot;
class RegisterClass;
class StackSlot;
class StackSlotManager;
class UnassignedReg;
class VirtualRegister;
class CONSTInst;

/**
 * Factory that owns all operands it creates. Deleting the factory deletes
 * all the owned operands.
 *
 * It is also used to create OperandSet instances.
 */
class MachineOperandFactory {
public:
	/// Do not use directly, instead use StackSlotManager to create instances
	ManagedStackSlot* CreateManagedStackSlot(StackSlotManager*, Type::TypeID);
	StackSlot* CreateStackSlot(int, Type::TypeID, bool);
	UnassignedReg* CreateUnassignedReg(Type::TypeID);
	VirtualRegister* CreateVirtualRegister(Type::TypeID);

	Immediate* CreateImmediate(CONSTInst* instruction);
	Immediate* CreateImmediate(s4 val, Type::IntType type);
	Immediate* CreateImmediate(s8 val, Type::LongType type);
	Immediate* CreateImmediate(float val, Type::FloatType type);
	Immediate* CreateImmediate(double val, Type::DoubleType type);
	Immediate* CreateImmediate(s8 val, Type::ReferenceType type);

	template <class T>
	T* CreateNativeRegister(Type::TypeID, MachineOperand*);

	OperandSet EmptySet() const { return OperandSet(this, operands.size()); }
	
	/// Returns all operands that correspond to the given register class
	OperandSet OperandsInClass(const RegisterClass&) const;

	/// Returns a new ExtendedOperandSet
	template <typename T>
	std::unique_ptr<ExtendedOperandSet<T>> EmptyExtendedSet() const {
		auto set = new ExtendedOperandSet<T>(this, operands.size());
		return std::unique_ptr<ExtendedOperandSet<T>>(set);
	}

	/**
	 * Takes ownership of the provided operand and sets the operands ID to
	 * the index in the operands vector.
	 * Usually this method does not need to be called, since ownership is handled
	 * by the factory methods directly. The only exceptions are "NativeRegister"s,
	 * because their typedefs are tricky.
	 */
	MachineOperand* register_ownership(MachineOperand*);

protected:
	using MachineOperandUPtrTy = std::unique_ptr<MachineOperand>;
	std::vector<MachineOperandUPtrTy> operands;

	friend class OperandSet;
	template <typename T> friend class ExtendedOperandSet;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MACHINEOPERANDFACTORY */

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
