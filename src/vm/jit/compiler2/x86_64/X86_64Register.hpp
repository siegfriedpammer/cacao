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

class GPRegister : public NativeRegister {
public:
	enum Type {
		R8  = 1,
		R16 = 2,
		R32 = 4,
		R64 = 8,
		NONE = 0
	};
	GPRegister(const char* name,unsigned index,bool extented_gpr) :
		NativeRegister(name,index,extented_gpr) {}
};

class SSERegister : public NativeRegister {
public:
	SSERegister(const char* name,unsigned index,bool extented_gpr) :
		NativeRegister(name,index,extented_gpr) {}
};

extern GPRegister RAX;
extern GPRegister RCX;
extern GPRegister RDX;
extern GPRegister RBX;
extern GPRegister RSP;
extern GPRegister RBP;
extern GPRegister RSI;
extern GPRegister RDI;
extern GPRegister R8;
extern GPRegister R9;
extern GPRegister R10;
extern GPRegister R11;
extern GPRegister R12;
extern GPRegister R13;
extern GPRegister R14;
extern GPRegister R15;

const unsigned IntegerArgumentRegisterSize = 6;
extern NativeRegister* IntegerArgumentRegisters[];

extern SSERegister XMM0;
extern SSERegister XMM1;
extern SSERegister XMM2;
extern SSERegister XMM3;
extern SSERegister XMM4;
extern SSERegister XMM5;
extern SSERegister XMM6;
extern SSERegister XMM7;
extern SSERegister XMM8;
extern SSERegister XMM9;
extern SSERegister XMM10;
extern SSERegister XMM11;
extern SSERegister XMM12;
extern SSERegister XMM13;
extern SSERegister XMM14;
extern SSERegister XMM15;

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
