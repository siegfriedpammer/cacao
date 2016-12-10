/* src/vm/jit/compiler2/X86_64Register.hpp - X86_64Register

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

#include "vm/jit/compiler2/x86_64/X86_64ModRMOperand.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {

NativeAddress *NativeAddress::to_NativeAddress() {
	return this;
}

X86_64ModRMOperand *X86_64ModRMOperand::to_X86_64ModRMOperand() {
	return this;
}

void X86_64ModRMOperand::prepareEmit() {
	base86_64 = cast_to<X86_64Register>(embedded_operands[base].real->op);
	if (op_size() > index)
		index86_64 = cast_to<X86_64Register>(embedded_operands[index].real->op);
}

u1 X86_64ModRMOperand::getRex(const X86_64Register &reg, bool opsiz64) {
	assert(base86_64);
	const unsigned rex_w = 3;
	const unsigned rex_r = 2;
	const unsigned rex_x = 1;
	const unsigned rex_b = 0;

	u1 rex = 0x40;

	// 64-bit operand size
	if (opsiz64) {
		rex |= (1 << rex_w);
	}
	if (reg.extented) {
		rex |= (1 << rex_r);
	}
	if (base86_64 && base86_64->extented) {
		rex |= (1 << rex_b);
	}
	if (index86_64 && index86_64->extented) {
		rex |= (1 << rex_x);
	}
	return rex;

}

bool X86_64ModRMOperand::useSIB() {
	assert(base86_64);
	return index || base86_64 == &RSP || base86_64 == &R12;
}

u1 X86_64ModRMOperand::getModRM(const X86_64Register &reg) {
	assert(base86_64);
	u1 rex = getRex(reg,false);

	u1 modrm_mod = 6;
	u1 modrm_reg = 3;
	u1 modrm_rm = 0;

	u1 mod = 0;
	u1 rm = 0;
	u1 modrm = 0;

	if (useDisp8()) {
		// disp8
		mod = 0x01; // 0b01
	} else if (useDisp32()) {
		// disp32
		mod = 0x02; // 0b10
	}
	else {
		// no disp
		mod = 0x00; // 0b00
	}

	if (useSIB()) {
		rm = 0x04; // 0b100		
	}
	else {
		rm = base86_64->get_index();	
	}

	modrm = mod << modrm_mod;
	modrm |= reg.get_index() << modrm_reg;
	modrm |= rm << modrm_rm;

	return modrm;
}

u1 X86_64ModRMOperand::getSIB(const X86_64Register &reg) {
	assert(base86_64);
	u1 rex = getRex(reg,false);

	u1 sib_scale = 6;
	u1 sib_index = 3;
	u1 sib_base = 0;

	u1 sib = 0;	

	sib = scale << sib_scale;
	if (index86_64) {
		sib |= index86_64->get_index() << sib_index;
	}
	else {
		sib |= RSP.get_index() << sib_index;
	}

	sib |= base86_64->get_index() << sib_base;

	return sib;
}

bool X86_64ModRMOperand::useDisp8() {
	return disp != 0 && fits_into<s1>(disp);
}

bool X86_64ModRMOperand::useDisp32() {
	return disp != 0 && !fits_into<s1>(disp);
}

s1 X86_64ModRMOperand::getDisp8() {
	return disp;
}

s1 X86_64ModRMOperand::getDisp32_1() {
	return 0xff & (disp >> 0);
}

s1 X86_64ModRMOperand::getDisp32_2() {
	return 0xff & (disp >> 8);
}

s1 X86_64ModRMOperand::getDisp32_3() {
	return 0xff & (disp >> 16);
}

s1 X86_64ModRMOperand::getDisp32_4() {
	return 0xff & (disp >> 24);
}

X86_64ModRMOperand::ScaleFactor X86_64ModRMOperand::get_scale(Type::TypeID type) {
	switch (type) {
	case Type::ByteTypeID:
		return Scale1;
	case Type::ShortTypeID:
		return Scale2;
	case Type::IntTypeID:
	case Type::FloatTypeID:
		return Scale4;
	case Type::LongTypeID:
	case Type::DoubleTypeID:
	case Type::ReferenceTypeID:
		return Scale8;
	default:
		break;
	}
	ABORT_MSG("type not supported", "x86_64 ModRMOperand::get_scale() type: " << type);
	return Scale1;
}

X86_64ModRMOperand::ScaleFactor X86_64ModRMOperand::get_scale(int32_t scale) {
	switch (scale) {
	case 1:
		return Scale1;
	case 2:
		return Scale2;
	case 4:
		return Scale4;
	case 8:
		return Scale8;
	default:
		break;
	}
	ABORT_MSG("type not supported", "x86_64 ModRMOperand::get_scale() type: " << scale);
	return Scale1;
}

} // end namespace x86_64
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
