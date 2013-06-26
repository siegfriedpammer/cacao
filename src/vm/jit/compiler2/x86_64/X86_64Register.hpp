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

#ifndef _JIT_COMPILER2_X86_64REGISTER
#define _JIT_COMPILER2_X86_64REGISTER

#include "vm/jit/compiler2/x86_64/X86_64.hpp"
#include "vm/jit/compiler2/MachineRegister.hpp"
#include "vm/jit/compiler2/RegisterFile.hpp"

#include <vector>

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {

/**
 * x86_64 Register
 */

class NativeRegister : public MachineRegister {
public:
	const unsigned index;
	const bool extented_gpr;

	NativeRegister(const char* name,unsigned index,bool extented_gpr)
		: MachineRegister(name), index(index), extented_gpr(extented_gpr) {}
	virtual NativeRegister* to_NativeRegister() {
		return this;
	}
	unsigned get_index() const {
		return index;
	}
};

extern NativeRegister RAX;
extern NativeRegister RCX;
extern NativeRegister RDX;
extern NativeRegister RBX;
extern NativeRegister RSP;
extern NativeRegister RBP;
extern NativeRegister RSI;
extern NativeRegister RDI;
extern NativeRegister R8;
extern NativeRegister R9;
extern NativeRegister R10;
extern NativeRegister R11;
extern NativeRegister R12;
extern NativeRegister R13;
extern NativeRegister R14;
extern NativeRegister R15;

const unsigned IntegerArgumentRegisterSize = 6;
extern NativeRegister* IntegerArgumentRegisters[];

extern NativeRegister XMM0;
extern NativeRegister XMM1;
extern NativeRegister XMM2;
extern NativeRegister XMM3;
extern NativeRegister XMM4;
extern NativeRegister XMM5;
extern NativeRegister XMM6;
extern NativeRegister XMM7;
extern NativeRegister XMM8;
extern NativeRegister XMM9;
extern NativeRegister XMM10;
extern NativeRegister XMM11;
extern NativeRegister XMM12;
extern NativeRegister XMM13;
extern NativeRegister XMM14;
extern NativeRegister XMM15;

const unsigned FloatArgumentRegisterSize = 8;
extern NativeRegister* FloatArgumentRegisters[];

class RegisterFile : public compiler2::RegisterFile {
private:
	RegisterFile() {
		#if 0
		for(unsigned i = 0; i < IntegerArgumentRegisterSize ; ++i) {
			regs.push_back(IntegerArgumentRegisters[i]);
		}
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

	}
public:
	static RegisterFile* factory() {
		static RegisterFile rf;
		return &rf;
	}
};

} // end namespace x86_64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_X86_64REGISTER */


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
