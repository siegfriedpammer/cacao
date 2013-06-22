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
#include "vm/types.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class X86_64CmpInst : public MachineInstruction {
public:
	/**
	 * TODO: return type actually is status-flags
	 */
	X86_64CmpInst(Register *src2, MachineOperand *src1)
			: MachineInstruction("X86_64CmpInst", &NoOperand, 2) {
		operands[0].op = src1;
		operands[1].op = src2;
	}
	virtual void emit(CodeMemory* CM) const;
	virtual bool accepts_immediate(unsigned i, Immediate *imm) const {
		if (i != 1) return false;
		return fits_into<s4>(imm->get_value());
	}
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
	BeginInstRef &target;
public:
	X86_64CondJumpInst(X86_64Cond::COND cond, BeginInstRef &target)
			: MachineInstruction("X86_64CondJumpInst", &NoOperand, 0),
			  cond(cond), target(target) {
	}
	BeginInst* get_BeginInst() const {
		return target.get();
	}
	virtual void emit(CodeMemory* CM) const;
	virtual OStream& print(OStream &OS) const {
		return OS << "[" << setz(4) << get_id() << "] "
			<< get_name() << "-> " << get_BeginInst();
	}
};

class X86_64AddInst : public MachineInstruction {
public:
	X86_64AddInst(Register *src2, Register *dstsrc1)
			: MachineInstruction("X86_64AddInst", dstsrc1, 2) {
		operands[0].op = dstsrc1;
		operands[1].op = src2;
	}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64IMulInst : public MachineInstruction {
public:
	X86_64IMulInst(Register *src2, Register *dstsrc1)
			: MachineInstruction("X86_64IMulInst", dstsrc1, 2) {
		operands[0].op = dstsrc1;
		operands[1].op = src2;
	}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64SubInst : public MachineInstruction {
public:
	X86_64SubInst(Register *src2, Register *dstsrc1)
			: MachineInstruction("X86_64SubInst", dstsrc1, 2) {
		operands[0].op = dstsrc1;
		operands[1].op = src2;
	}
	virtual void emit(CodeMemory* CM) const;
	virtual bool accepts_immediate(unsigned i, Immediate *imm) const {
		if (i != 1) return false;
		return fits_into<s4>(imm->get_value());
	}
};

class X86_64RetInst : public MachineInstruction {
public:
	X86_64RetInst()
			: MachineInstruction("X86_64RetInst", &NoOperand, 0) {
	}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64MovInst : public MachineMoveInst {
public:
	X86_64MovInst(MachineOperand *src, MachineOperand *dst)
			: MachineMoveInst("X86_64MovInst", src, dst) {}
	virtual void emit(CodeMemory* CM) const;
};

class X86_64JumpInst : public MachineJumpInst {
private:
	BeginInstRef &target;
public:
	X86_64JumpInst(BeginInstRef &target) : MachineJumpInst("X86_64JumpInst"),
		target(target) {}
	virtual void emit(CodeMemory* CM) const;
	virtual void emit(CodeFragment &CF) const;
	virtual OStream& print(OStream &OS) const {
		return OS << "[" << setz(4) << get_id() << "] "
			<< get_name() << "-> " << get_BeginInst();
	}
	BeginInst* get_BeginInst() const {
		return target.get();
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
