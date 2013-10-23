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
const uint8_t SSERegister::base = 0;

GPRegister RAX("RAX",0x0,false,0x0*8,8);
GPRegister RCX("RCX",0x1,false,0x1*8,8);
GPRegister RDX("RDX",0x2,false,0x2*8,8);
GPRegister RBX("RBX",0x3,false,0x3*8,8);
GPRegister RSP("RSP",0x4,false,0x4*8,8);
GPRegister RBP("RBP",0x5,false,0x5*8,8);
GPRegister RSI("RSI",0x6,false,0x6*8,8);
GPRegister RDI("RDI",0x7,false,0x7*8,8);
GPRegister R8 ("R8", 0x0,true ,0x8*8,8);
GPRegister R9 ("R9", 0x1,true ,0x9*8,8);
GPRegister R10("R10",0x2,true ,0xa*8,8);
GPRegister R11("R11",0x3,true ,0xb*8,8);
GPRegister R12("R12",0x4,true ,0xc*8,8);
GPRegister R13("R13",0x5,true ,0xd*8,8);
GPRegister R14("R14",0x6,true ,0xe*8,8);
GPRegister R15("R15",0x7,true ,0xf*8,8);

GPRegister* IntegerArgumentRegisters[] = {
&RDI, &RSI, &RDX, &RCX, &R8, &R9
};

SSERegister XMM0 ("XMM0" ,0x0,false,0x0*16,16);
SSERegister XMM1 ("XMM1" ,0x1,false,0x1*16,16);
SSERegister XMM2 ("XMM2" ,0x2,false,0x2*16,16);
SSERegister XMM3 ("XMM3" ,0x3,false,0x3*16,16);
SSERegister XMM4 ("XMM4" ,0x4,false,0x4*16,16);
SSERegister XMM5 ("XMM5" ,0x5,false,0x5*16,16);
SSERegister XMM6 ("XMM6" ,0x6,false,0x6*16,16);
SSERegister XMM7 ("XMM7" ,0x7,false,0x7*16,16);
SSERegister XMM8 ("XMM8" ,0x0,true ,0x8*16,16);
SSERegister XMM9 ("XMM9" ,0x1,true ,0x9*16,16);
SSERegister XMM10("XMM10",0x2,true ,0xa*16,16);
SSERegister XMM11("XMM11",0x3,true ,0xb*16,16);
SSERegister XMM12("XMM12",0x4,true ,0xc*16,16);
SSERegister XMM13("XMM13",0x5,true ,0xd*16,16);
SSERegister XMM14("XMM14",0x6,true ,0xe*16,16);
SSERegister XMM15("XMM15",0x7,true ,0xf*16,16);

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
		OF.push_back(new x86_64::NativeRegister(type,&RDI));
		OF.push_back(new x86_64::NativeRegister(type,&RSI));
		OF.push_back(new x86_64::NativeRegister(type,&RDX));
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

