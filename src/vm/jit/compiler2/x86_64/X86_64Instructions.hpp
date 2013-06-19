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
#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"
#include "vm/jit/compiler2/MachineInstruction.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class X86_64CmpInst : public MachineInstruction {
public:
	/**
	 * TODO: return type actually is status-flags
	 */
	X86_64CmpInst(Register *src1, MachineOperand *src2)
			: MachineInstruction("X86_64CmpInst", &NoOperand, 2) {
		operands[0].op = src1;
		operands[1].op = src2;
	}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64EnterInst : public MachineInstruction {
private:
	u2 framesize;
public:
	X86_64EnterInst(u2 framesize)
			: MachineInstruction("X86_64EnterInst", &NoOperand, 0),
			framesize(framesize) {}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64LeaveInst : public MachineInstruction {
public:
	X86_64LeaveInst()
			: MachineInstruction("X86_64LeaveInst", &NoOperand, 0) {}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64CondJumpInst : public MachineInstruction {
private:
	X86_64Cond::COND cond;
	BeginInst* target;
public:
	X86_64CondJumpInst(X86_64Cond::COND cond, BeginInst* target)
			: MachineInstruction("X86_64CondJumpInst", &NoOperand, 0),
			  cond(cond), target(target) {
	}
	BeginInst* get_BeginInst() const { return target; }
	virtual void emit(CodeMemory* CM) const;
};

class X86_64AddInst : public MachineInstruction {
public:
	X86_64AddInst(Register *dst, Register *src1, Register *src2)
			: MachineInstruction("X86_64AddInst", dst, 2) {
		operands[0].op = src1;
		operands[1].op = src2;
	}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64IMulInst : public MachineInstruction {
public:
	X86_64IMulInst(Register *dst, Register *src1, Register *src2)
			: MachineInstruction("X86_64IMulInst", dst, 2) {
		operands[0].op = src1;
		operands[1].op = src2;
	}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64SubInst : public MachineInstruction {
public:
	X86_64SubInst(Register *dst, Register *src1, Register *src2)
			: MachineInstruction("X86_64SubInst", dst, 2) {
		operands[0].op = src1;
		operands[1].op = src2;
	}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64RetInst : public MachineInstruction {
public:
	X86_64RetInst()
			: MachineInstruction("X86_64RetInst", &NoOperand, 0) {
	}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64MovInst : public MachineInstruction {
public:
	X86_64MovInst(MachineRegister *dst, MachineRegister *src)
			: MachineInstruction("X86_64MovInst", dst, 1) {
		operands[0].op = src;
	}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64MovStack2RegInst : public MachineInstruction {
public:
	X86_64MovStack2RegInst(MachineRegister *dst, StackSlot *slot)
			: MachineInstruction("X86_64MovStack2RegInst", dst, 1) {
		operands[0].op = slot;
	}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64MovImm2RegInst : public MachineInstruction {
public:
	X86_64MovImm2RegInst(MachineRegister *dst, Immediate *imm)
			: MachineInstruction("X86_64MovImm2RegInst", dst, 1) {
		operands[0].op = imm;
	}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64MovReg2StackInst : public MachineInstruction {
public:
	X86_64MovReg2StackInst(StackSlot *slot, MachineRegister *src)
			: MachineInstruction("X86_64MovReg2StackInst", slot, 1) {
		operands[0].op = src;
	}
	virtual void emit(CodeMemory* CM) const;
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
