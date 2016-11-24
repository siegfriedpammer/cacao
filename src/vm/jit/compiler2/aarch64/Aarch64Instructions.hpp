/* src/vm/jit/compiler2/aarch64/Aarch64Instructions.hpp 

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

#ifndef _JIT_COMPILER2_AARCH64_INSTRUCTIONS
#define _JIT_COMPILER2_AARCH64_INSTRUCTIONS

#include "vm/jit/compiler2/aarch64/Aarch64Cond.hpp"
#include "vm/jit/compiler2/aarch64/Aarch64Emitter.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace aarch64 {

// TODO: As soon as the wrappers are final, document them

struct SrcOp {
	MachineOperand *op;
	explicit SrcOp(MachineOperand *op) : op(op) {}
};

struct DstOp {
	MachineOperand *op;
	explicit DstOp(MachineOperand *op) : op(op) {}
};


class MovInst : public MachineInstruction {
public:
	MovInst(const DstOp &dst, const SrcOp &src)
		: MachineInstruction("Aarch64MovInst", dst.op, 1) {
		operands[0].op = src.op;
	}

	virtual void emit(CodeMemory* cm) const;
};


class AddInst : public MachineInstruction {
public:
	AddInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2)
			: MachineInstruction("Arch64AddInst", dst.op, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(CodeMemory* cm) const;
};


class SubInst : public MachineInstruction {
public:
	SubInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2)
			: MachineInstruction("Arch64SubInst", dst.op, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(CodeMemory* cm) const;
};


class MulInst : public MachineInstruction {
public:
	MulInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2)
			: MachineInstruction("Arch64MulInst", dst.op, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(CodeMemory* cm) const;	
};


class DivInst : public MachineInstruction {
public:
	DivInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2)
			: MachineInstruction("Arch64DivInst", dst.op, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(CodeMemory* cm) const;	
};


class JumpInst : public MachineJumpInst {
public:
	JumpInst(MachineBasicBlock* target) : MachineJumpInst("Aarch64JumpInst") {
		successors.push_back(target);
	}

	virtual bool is_jump() const { return true; }
	void set_target(MachineBasicBlock* target) { successors.front() = target; }

	virtual void emit(CodeMemory* cm) const;
	virtual void link(CodeFragment& cf) const;
};


class CmpInst : public MachineInstruction {
public:
	CmpInst(const SrcOp &src1, const SrcOp &src2)
		: MachineInstruction("Aarch64CmpInst", &NoOperand, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(CodeMemory* cm) const;
};


class CondJumpInst : public MachineInstruction {
public:
	CondJumpInst(Cond::COND cond, MachineBasicBlock* then_target,
				 MachineBasicBlock* else_target)
		: MachineInstruction("Aarch64CondJumpInst", &NoOperand, 0),
		  cond(cond), jump(else_target) {
		successors.push_back(then_target);
		successors.push_back(else_target);
	}

	virtual bool is_jump() const { return true; }
	MachineBasicBlock* get_then() const { return successor_front(); }
	MachineBasicBlock* get_else() const { return successor_back(); }

	virtual void emit(CodeMemory* cm) const;
	virtual void link(CodeFragment& cf) const;

private:
	Cond::COND cond;
	mutable JumpInst jump;
};


class EnterInst : public MachineInstruction {
private:
	u2 framesize;
public:
	EnterInst(u2 framesize)
		: MachineInstruction("Aarch64EnterInst", &NoOperand, 0),
		framesize(framesize) {}

	virtual void emit(CodeMemory* cm) const;
};


class LeaveInst : public MachineInstruction {
private:
	u2 framesize;
public:
	LeaveInst(u2 framesize) 
		: MachineInstruction("Aarch64LeaveInst", &NoOperand, 0),
		framesize(framesize) {}

	virtual void emit(CodeMemory* cm) const;
};


class RetInst : public MachineInstruction {
public:
	RetInst() : MachineInstruction("Aarch64RetInst", &NoOperand, 0) {}

	virtual bool is_end() const { return true; }
	virtual void emit(CodeMemory* cm) const;
};

} // end namespace aarch64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_AARCH64_INSTRUCTIONS */


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
