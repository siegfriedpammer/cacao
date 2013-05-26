/* src/vm/jit/compiler2/X86_64Instructions.hpp - X86_64Instructions

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

#ifndef _JIT_COMPILER2_X86_64INSTRUCTIONS
#define _JIT_COMPILER2_X86_64INSTRUCTIONS

#include "vm/jit/compiler2/x86_64/X86_64.hpp"
#include "vm/jit/compiler2/x86_64/X86_64Cond.hpp"
#include "vm/jit/compiler2/MachineInstruction.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class X86_64CmpInst : public MachineInstruction {
public:
	/**
	 * TODO: return type actually is status-flags
	 */
	X86_64CmpInst()
			: MachineInstruction("X86_64CmpInst", MachineOperand::NONE,
			  2, MachineOperand::REGISTER_VALUE) {
	}

};

class X86_64CondJumpInst : public MachineInstruction {
private:
	X86_64Cond::COND cond;
public:
	X86_64CondJumpInst(X86_64Cond::COND cond)
			: MachineInstruction("X86_64CondJumpInst", MachineOperand::NONE, 1,
			  MachineOperand::ABSOLUTE_ADDR | MachineOperand::PIC_ADDR),
			  cond(cond) {
	}

};

class X86_64AddInst : public MachineInstruction {
public:
	X86_64AddInst()
			: MachineInstruction("X86_64AddInst", MachineOperand::REGISTER_VALUE, 2) {
		operands[0] = MachineOperand::REGISTER_VALUE | MachineOperand::REGISTER_MEM;
		operands[1] = MachineOperand::REGISTER_VALUE | MachineOperand::REGISTER_MEM
		            | MachineOperand::IMMEDIATE;
	}

};

class X86_64SubInst : public MachineInstruction {
public:
	X86_64SubInst()
			: MachineInstruction("X86_64SubInst", MachineOperand::REGISTER_VALUE, 2) {
		operands[0] = MachineOperand::REGISTER_VALUE | MachineOperand::REGISTER_MEM;
		operands[1] = MachineOperand::REGISTER_VALUE | MachineOperand::REGISTER_MEM
		            | MachineOperand::IMMEDIATE;
	}

};

class X86_64RetInst : public MachineInstruction {
public:
	X86_64RetInst()
			: MachineInstruction("X86_64RetInst", MachineOperand::NONE, 0) {
	}

};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_X86_64INSTRUCTIONS */


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
