/* src/vm/jit/compiler2/MachineOperandFactory.cpp - MachineOperandFactory

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

#include "vm/jit/compiler2/MachineOperandFactory.hpp"

#include "vm/jit/compiler2/Backend.hpp"
#include "vm/jit/compiler2/MachineRegister.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"
#include "vm/jit/compiler2/MachineOperandSet.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

MachineOperand* MachineOperandFactory::register_ownership(MachineOperand* operand)
{
	operands.emplace_back(operand);
	operand->set_id(operands.size() - 1);
	return operand;
}

ManagedStackSlot* MachineOperandFactory::CreateManagedStackSlot(StackSlotManager* SSM,
                                                                Type::TypeID type)
{
	auto operand = new ManagedStackSlot(SSM, type);
	return register_ownership(operand)->to_ManagedStackSlot();
}

StackSlot* MachineOperandFactory::CreateStackSlot(int index, Type::TypeID type, bool is_leaf)
{
	auto operand = new StackSlot(index, type, is_leaf);
	return register_ownership(operand)->to_StackSlot();
}

UnassignedReg* MachineOperandFactory::CreateUnassignedReg(Type::TypeID type)
{
	auto operand = new UnassignedReg(type);
	return register_ownership(operand)->to_Register()->to_UnassignedReg();
}

VirtualRegister* MachineOperandFactory::CreateVirtualRegister(Type::TypeID type)
{
	auto operand = new VirtualRegister(type);
	return register_ownership(operand)->to_Register()->to_VirtualRegister();
}

template <>
NativeRegister* MachineOperandFactory::CreateNativeRegister(Type::TypeID type, MachineOperand* reg)
{
	auto operand = new NativeRegister(type, reg);
	return register_ownership(operand)->to_Register()->to_MachineRegister()->to_NativeRegister();
}

OperandSet MachineOperandFactory::OperandsInClass(const RegisterClass& rc) const
{
	auto result = EmptySet();

	for (const auto& operand : operands) {
		if (rc.handles_type(operand->get_type()))
			result.add(operand.get());
	}

	return result;
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
