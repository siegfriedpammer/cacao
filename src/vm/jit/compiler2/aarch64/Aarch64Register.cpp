/* src/vm/jit/compiler2/aarch64/Aarch64Register.cpp

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

#include "vm/jit/compiler2/aarch64/Aarch64Register.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace aarch64 {

NativeRegister::NativeRegister(Type::TypeID type, Aarch64Register* reg)
	: MachineRegister(reg->name, type), reg(reg), fixedInterval(this) {}

const uint8_t GPRegister::base = 0;
const uint8_t FPRegister::base = 0;

GPRegister R0 ("R0" ,  0,  0*8, 8);
GPRegister R1 ("R1" ,  1,  1*8, 8);
GPRegister R2 ("R2" ,  2,  2*8, 8);
GPRegister R3 ("R3" ,  3,  3*8, 8);
GPRegister R4 ("R4" ,  4,  4*8, 8);
GPRegister R5 ("R5" ,  5,  5*8, 8);
GPRegister R6 ("R6" ,  6,  6*8, 8);
GPRegister R7 ("R7" ,  7,  7*8, 8);
GPRegister R8 ("R8" ,  8,  8*8, 8);
GPRegister R9 ("R9" ,  9,  9*8, 8);
GPRegister R10("R10", 10, 10*8, 8);
GPRegister R11("R11", 11, 11*8, 8);
GPRegister R12("R12", 12, 12*8, 8);
GPRegister R13("R13", 13, 13*8, 8);
GPRegister R14("R14", 14, 14*8, 8);
GPRegister R15("R15", 15, 15*8, 8);
GPRegister R16("R16", 16, 16*8, 8);
GPRegister R17("R17", 17, 17*8, 8);
GPRegister R18("R18", 18, 18*8, 8);
GPRegister R19("R19", 19, 19*8, 8);
GPRegister R20("R20", 20, 20*8, 8);
GPRegister R21("R21", 21, 21*8, 8);
GPRegister R22("R22", 22, 22*8, 8);
GPRegister R23("R23", 23, 23*8, 8);
GPRegister R24("R24", 24, 24*8, 8);
GPRegister R25("R25", 25, 25*8, 8);
GPRegister R26("R26", 26, 26*8, 8);
GPRegister R27("R27", 27, 27*8, 8);
GPRegister R28("R28", 28, 28*8, 8);
GPRegister R29("R29", 29, 29*8, 8);
GPRegister R30("R30", 30, 30*8, 8);
GPRegister SP ("SP" , 31, 31*8, 8);

GPRegister* IntegerArgumentRegisters[] = {
	&R0, &R1, &R2, &R3, &R4, &R5, &R6, &R7
};

GPRegister* IntegerCallerSavedRegisters[] = {
	&R0, &R1, &R2, &R3, &R4, &R5, &R6, &R7, &R8, &R10, &R11, &R12,
	&R13, &R14, &R15, &R16, &R17
};
std::size_t IntegerCallerSavedRegistersSize = 17;

FPRegister V0 ("V0" ,  0,  0*16, 16);
FPRegister V1 ("V1" ,  1,  1*16, 16);
FPRegister V2 ("V2" ,  2,  2*16, 16);
FPRegister V3 ("V3" ,  3,  3*16, 16);
FPRegister V4 ("V4" ,  4,  4*16, 16);
FPRegister V5 ("V5" ,  5,  5*16, 16);
FPRegister V6 ("V6" ,  6,  6*16, 16);
FPRegister V7 ("V7" ,  7,  7*16, 16);
FPRegister V8 ("V8" ,  8,  8*16, 16);
FPRegister V9 ("V9" ,  9,  9*16, 16);
FPRegister V10("V10", 10, 10*16, 16);
FPRegister V11("V11", 11, 11*16, 16);
FPRegister V12("V12", 12, 12*16, 16);
FPRegister V13("V13", 13, 13*16, 16);
FPRegister V14("V14", 14, 14*16, 16);
FPRegister V15("V15", 15, 15*16, 16);
FPRegister V16("V16", 16, 16*16, 16);
FPRegister V17("V17", 17, 17*16, 16);
FPRegister V18("V18", 18, 18*16, 16);
FPRegister V19("V19", 19, 19*16, 16);
FPRegister V20("V20", 20, 20*16, 16);
FPRegister V21("V21", 21, 21*16, 16);
FPRegister V22("V22", 22, 22*16, 16);
FPRegister V23("V23", 23, 23*16, 16);
FPRegister V24("V24", 24, 24*16, 16);
FPRegister V25("V25", 25, 25*16, 16);
FPRegister V26("V26", 26, 26*16, 16);
FPRegister V27("V27", 27, 27*16, 16);
FPRegister V28("V28", 28, 28*16, 16);
FPRegister V29("V29", 29, 29*16, 16);
FPRegister V30("V30", 30, 30*16, 16);
FPRegister V31("V31", 31, 31*16, 16);

FPRegister* FloatArgumentRegisters[] = {
	&V0, &V1, &V2, &V3, &V4, &V5, &V6, &V7
};


FPRegister* FloatCallerSavedRegisters[] = {
	&V0, &V1, &V2, &V3, &V4, &V5, &V6, &V7, &V16, &V17, &V18, &V19, &V20,
	&V21, &V22, &V23, &V24, &V25, &V26, &V27, &V28, &V29, &V30, &V31
};
std::size_t FloatCallerSavedRegistersSize = 24;

} // end namespace aarch64

using namespace aarch64;

template<>
OperandFile&
BackendBase<Aarch64>::get_OperandFile(OperandFile& of, MachineOperand* mo) const {
	Type::TypeID type = mo->get_type();

	switch (type) {
	case Type::CharTypeID:
	case Type::ByteTypeID:
	case Type::ShortTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	case Type::ReferenceTypeID:
		#if 1
		for (unsigned i = 0; i < IntegerCallerSavedRegistersSize; ++i) {
			of.push_back(new aarch64::NativeRegister(type, IntegerCallerSavedRegisters[i]));
		}
		assert(of.size() == IntegerCallerSavedRegistersSize);
		#else
		of.push_back(new aarch64::NativeRegister(type, &R9));
		of.push_back(new aarch64::NativeRegister(type, &R10));
		of.push_back(new aarch64::NativeRegister(type, &R11));
		of.push_back(new aarch64::NativeRegister(type, &R12));
		of.push_back(new aarch64::NativeRegister(type, &R13));
		of.push_back(new aarch64::NativeRegister(type, &R14));
		of.push_back(new aarch64::NativeRegister(type, &R15));
		
		#endif
		break;

	case Type::FloatTypeID:
	case Type::DoubleTypeID:
		for (unsigned i = 0; i < FloatCallerSavedRegistersSize; ++i) {
			of.push_back(new aarch64::NativeRegister(type, FloatCallerSavedRegisters[i]));
		}
		assert(of.size() == FloatCallerSavedRegistersSize);
		break;
		
	default:
		ABORT_MSG("aarch64 Register File Type Not supported!", 
				"Type: " << type);
	}
	return of;
}

template<>
void BackendBase<Aarch64>::get_CallerSaved(OperandFile& of) const {
	Type::TypeID type = Type::LongTypeID;
	#if 1
	for (unsigned i = 0; i < IntegerCallerSavedRegistersSize; ++i) {
		of.push_back(new aarch64::NativeRegister(type, IntegerCallerSavedRegisters[i]));
	}
	type = Type::DoubleTypeID;
	for (unsigned i = 0; i < FloatCallerSavedRegistersSize; ++i) {
		of.push_back(new aarch64::NativeRegister(type, FloatCallerSavedRegisters[i]));
	}
	#endif
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

