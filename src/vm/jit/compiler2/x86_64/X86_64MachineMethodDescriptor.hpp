/* src/vm/jit/compiler2/X86_64MachineMethodDescriptor.hpp - X86_64MachineMethodDescriptor

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

#ifndef _JIT_COMPILER2_X86_64MACHINEMETHODDECRIPTOR
#define _JIT_COMPILER2_X86_64MACHINEMETHODDECRIPTOR

#include "vm/jit/compiler2/MethodDescriptor.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"
#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {


/**
 * MachineMethodDescriptor
 */
class MachineMethodDescriptor {
private:
	const MethodDescriptor &MD;
	MachineOperandFactory* MOF;
	alloc::vector<MachineOperand*>::type parameter;
public:
	MachineMethodDescriptor(const MethodDescriptor &MD, MachineOperandFactory* MOF) : MD(MD), MOF(MOF), parameter(MD.size()) {
		unsigned int_argument_counter = 0;
		unsigned float_argument_counter = 0;
		int stackslot_index = 0;
		for (unsigned i = 0, e = MD.size(); i < e; ++i) {
			Type::TypeID type = MD[i];
			switch (type) {
			case Type::CharTypeID:
			case Type::ShortTypeID:
			case Type::IntTypeID:
			case Type::LongTypeID:
			case Type::ReferenceTypeID:
				if (int_argument_counter < IntegerArgumentRegisterSize) {
					parameter[i]= MOF->CreateNativeRegister<NativeRegister>(type,
						IntegerArgumentRegisters[int_argument_counter]);
				} else {
					parameter[i]= MOF->CreateStackSlot(stackslot_index,type);
					stackslot_index++;
				}
				int_argument_counter++;
				break;
			case Type::FloatTypeID:
			case Type::DoubleTypeID:
				if (float_argument_counter < FloatArgumentRegisterSize) {
					parameter[i]= MOF->CreateNativeRegister<NativeRegister>(type,
						FloatArgumentRegisters[float_argument_counter]);
				} else {
					parameter[i]= MOF->CreateStackSlot(stackslot_index,type);
					stackslot_index++;
				}
				float_argument_counter++;
				break;
			default:
				ABORT_MSG("x86_64 MachineMethodDescriptor: Type not yet "
					"supported!", "Type: " << type);
			}
		}
	}
	MachineOperand* operator[](unsigned index) const {
		assert(index < parameter.size());
		return parameter[index];
	}
	friend OStream& operator<<(OStream &OS, const MachineMethodDescriptor &MMD);
};

OStream& operator<<(OStream &OS, const MachineMethodDescriptor &MMD);
inline OStream& operator<<(OStream &OS, const MachineMethodDescriptor *MMD) {
	if (!MMD) {
		return OS << "(X86_64MachineMethodDescriptor) NULL";
	}
	return OS << *MMD;
}

} // end namespace x86_64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_X86_64MACHINEMETHODDECRIPTOR */


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
