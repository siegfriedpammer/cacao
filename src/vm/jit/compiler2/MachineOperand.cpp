/* src/vm/jit/compiler2/MachineOperand.cpp - MachineOperand

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

#include "vm/jit/compiler2/MachineOperand.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/StackSlotManager.hpp"
#include "toolbox/logging.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

VoidOperand NoOperand;

unsigned VirtualRegister::vreg_counter = 0;
#if 0
OStream& operator<<(OStream &OS, const MachineOperandType &MO) {
	OS << "MachineOperandType [ ";
	if (MO.takes(MachineOperandType::REGISTER_VALUE))
		OS << "RV ";
	if (MO.takes(MachineOperandType::REGISTER_MEM))
		OS << "RM ";
	if (MO.takes(MachineOperandType::IMMEDIATE))
		OS << "IM ";
	if (MO.takes(MachineOperandType::ABSOLUTE_ADDR))
		OS << "AA ";
	if (MO.takes(MachineOperandType::PIC_ADDR))
		OS << "PA ";
	if (MO.takes(MachineOperandType::PC_REL_ADDR))
		OS << "RA ";
	return OS << ']';
}
#endif

StackSlot* ManagedStackSlot::to_StackSlot() {
	return parent->get_StackSlot(this);
}

Immediate::Immediate(CONSTInst *I)
		: MachineOperand(ImmediateID,I->get_type()) {
	switch (get_type()) {
	case Type::IntTypeID:
		value.i = I->get_Int();
		break;
	case Type::LongTypeID:
		value.l = I->get_Long();
		break;
	case Type::FloatTypeID:
		value.f = I->get_Float();
		break;
	case Type::DoubleTypeID:
		value.d = I->get_Double();
		break;
	default:
		assert(0);
		break;
	}
}
template<>
s8 Immediate::get_value() const {
	switch (get_type()) {
	case Type::IntTypeID: return (s8)value.i;
	case Type::LongTypeID: return (s8)value.l;
	default: break;
	}
	ABORT_MSG("TypeNotSupported: Immediate::get_value<s8>",
		"type " << get_type());
	return 0;
}

template<>
s4 Immediate::get_value() const {
	switch (get_type()) {
	case Type::IntTypeID: return (s4)value.i;
	case Type::LongTypeID:
		if (fits_into<s4>(value.l)) {
			return (s4)value.l;
		}
		break;
	default: break;
	}
	ABORT_MSG("TypeNotSupported: Immediate::get_value<s8>",
		"type " << get_type());
	return 0;
}

template<>
s1 Immediate::get_value() const {
	switch (get_type()) {
	case Type::IntTypeID:
		if (fits_into<s1>(value.i)) {
			return (s1)value.i;
		}
		break;
	case Type::LongTypeID:
		if (fits_into<s1>(value.l)) {
			return (s1)value.l;
		}
		break;
	default: break;
	}
	ABORT_MSG("TypeNotSupported: Immediate::get_value<s8>",
		"type " << get_type());
	return 0;
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
