/* src/vm/jit/compiler2/X86_64Register.cpp - X86_64Register

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

#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {

NativeRegister::NativeRegister(Type::TypeID type,
	X86_64Register* reg) : MachineRegister(reg->name, type),
	reg(reg) {
}

const uint8_t GPRegister::base = 0;

MachineOperand::IdentifyOffsetTy GPRegister::id_offset() const {
	if (extented)
		return index + 0x8;
	return index;
}

const uint8_t SSERegister::base = 0;

MachineOperand::IdentifyOffsetTy SSERegister::id_offset() const {
	if (extented)
		return index + 0x8;
	return index;
}

GPRegister RAX("RAX",0x0,false);
GPRegister RCX("RCX",0x1,false);
GPRegister RDX("RDX",0x2,false);
GPRegister RBX("RBX",0x3,false);
GPRegister RSP("RSP",0x4,false);
GPRegister RBP("RBP",0x5,false);
GPRegister RSI("RSI",0x6,false);
GPRegister RDI("RDI",0x7,false);
GPRegister R8 ("R8", 0x0,true );
GPRegister R9 ("R9", 0x1,true );
GPRegister R10("R10",0x2,true );
GPRegister R11("R11",0x3,true );
GPRegister R12("R12",0x4,true );
GPRegister R13("R13",0x5,true );
GPRegister R14("R14",0x6,true );
GPRegister R15("R15",0x7,true );

GPRegister* IntegerArgumentRegisters[] = {
&RDI, &RSI, &RDX, &RCX, &R8, &R9
};

SSERegister XMM0 ("XMM0" ,0x0,false);
SSERegister XMM1 ("XMM1" ,0x1,false);
SSERegister XMM2 ("XMM2" ,0x2,false);
SSERegister XMM3 ("XMM3" ,0x3,false);
SSERegister XMM4 ("XMM4" ,0x4,false);
SSERegister XMM5 ("XMM5" ,0x5,false);
SSERegister XMM6 ("XMM6" ,0x6,false);
SSERegister XMM7 ("XMM7" ,0x7,false);
SSERegister XMM8 ("XMM8" ,0x0,true );
SSERegister XMM9 ("XMM9" ,0x1,true );
SSERegister XMM10("XMM10",0x2,true );
SSERegister XMM11("XMM11",0x3,true );
SSERegister XMM12("XMM12",0x4,true );
SSERegister XMM13("XMM13",0x5,true );
SSERegister XMM14("XMM14",0x6,true );
SSERegister XMM15("XMM15",0x7,true );

SSERegister* FloatArgumentRegisters[] = {
&XMM0, &XMM1, &XMM2, &XMM3, &XMM4, &XMM5, &XMM6, &XMM7
};

} // end namespace x86_64
using namespace x86_64;

template<>
OperandFile&
BackendBase<X86_64>::get_OperandFile(OperandFile& OF,MachineOperand *MO) const {
	Type::TypeID type = MO->get_type();

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	case Type::ReferenceTypeID:
		#if 1
		for(unsigned i = 0; i < IntegerArgumentRegisterSize ; ++i) {
			OF.push_back(new x86_64::NativeRegister(type,IntegerArgumentRegisters[i]));
		}
		assert(OF.size() == IntegerArgumentRegisterSize);
		#else
		regs.push_back(&RDI);
		regs.push_back(&RSI);
		regs.push_back(&RDX);
		#if 0
		regs.push_back(&RCX);
		regs.push_back(&R8);
		regs.push_back(&R9);
		#endif
		#endif
		return OF;
	case Type::DoubleTypeID:
		#if 1
		for(unsigned i = 0; i < FloatArgumentRegisterSize ; ++i) {
			OF.push_back(new x86_64::NativeRegister(type,FloatArgumentRegisters[i]));
		}
		assert(OF.size() == FloatArgumentRegisterSize);
		#else
		regs.push_back(&XMM0);
		regs.push_back(&XMM1);
		regs.push_back(&XMM2);
		#endif
		return OF;
	default: break;
	}
	ABORT_MSG("X86_64 Register File Type Not supported!",
		"Type: " << type);
	return OF;
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

