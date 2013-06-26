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

/**
 *
 * These wrapper classes are used to enforce the correct
 * ordering of the parameters.
 *
 */

/**
 * Simple wrapper for the operand of an
 * single operand x86_64 instruction.
 */
struct X86_64SrcOp {
	MachineOperand *op;
	explicit X86_64SrcOp(MachineOperand *op) : op(op) {}
};
/**
 * Simple wrapper for first operand of an
 * x86_64 instruction.
 */
struct X86_64Src1Op {
	MachineOperand *op;
	explicit X86_64Src1Op(MachineOperand *op) : op(op) {}
};
/**
 * Simple wrapper for second operand of an
 * x86_64 instruction.
 */
struct X86_64Src2Op {
	MachineOperand *op;
	explicit X86_64Src2Op(MachineOperand *op) : op(op) {}
};
/**
 * Simple wrapper for first operand of an
 * x86_64 instruction which is also used for the result.
 */
struct X86_64DstSrc1Op {
	MachineOperand *op;
	explicit X86_64DstSrc1Op(MachineOperand *op) : op(op) {}
};

/**
 * Simple wrapper for destination of an
 * x86_64 instruction.
 */
struct X86_64DstOp {
	MachineOperand *op;
	explicit X86_64DstOp(MachineOperand *op) : op(op) {}
};




/**
 * This abstract class represents a x86_64 ALU instruction.
 *
 *
 */
class X86_64ALUInstruction : public MachineInstruction {
private:
	u1 alu_id;

void emit_impl_I(CodeMemory* CM, Immediate* imm) const;
void emit_impl_RI(CodeMemory* CM, X86_64Register* src1,
	Immediate* imm) const;
#if 0
void emit_impl_MR(CodeMemory* CM, X86_64Register* src1,
	X86_64Register* src2) const;
#endif
void emit_impl_RR(CodeMemory* CM, X86_64Register* src1,
	X86_64Register* src2) const;

public:
	/**
	 * @param alu_id this parameter is used to identify the
	 * instruction. The mapping is as follows:
	 * - 0 ADD
	 * - 1 OR
	 * - 2 ADC
	 * - 3 SBB
	 * - 4 AND
	 * - 5 SUB
	 * - 6 XOR
	 * - 7 CMP
	 */
	X86_64ALUInstruction(const char * name, const X86_64DstSrc1Op &dstsrc1,
			const X86_64Src2Op &src2, u1 alu_id)
			: MachineInstruction(name, dstsrc1.op, 2) , alu_id(alu_id) {
		assert(alu_id < 8);
		operands[0].op = dstsrc1.op;
		operands[1].op = src2.op;
	}
	X86_64ALUInstruction(const char * name, const X86_64Src1Op &src1,
			const X86_64Src2Op &src2, u1 alu_id)
			: MachineInstruction(name, &NoOperand, 2) , alu_id(alu_id) {
		assert(alu_id < 8);
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}
	/**
	 * This must be implemented by subclasses.
	 */
	virtual void emit(CodeMemory* CM) const;

	virtual bool accepts_immediate(unsigned i, Immediate *imm) const {
		if (i != 1) return false;
		return fits_into<s4>(imm->get_value());
	}
};




// ALU instructions
class X86_64AddInst : public X86_64ALUInstruction {
public:
	X86_64AddInst(const X86_64Src2Op &src2, const X86_64DstSrc1Op &dstsrc1)
			: X86_64ALUInstruction("X86_64AddInst", dstsrc1, src2, 0) {
	}
};
class X86_64OrInst : public X86_64ALUInstruction {
public:
	X86_64OrInst(const X86_64Src2Op &src2, const X86_64DstSrc1Op &dstsrc1)
			: X86_64ALUInstruction("X86_64OrInst", dstsrc1, src2, 1) {
	}
};
class X86_64AdcInst : public X86_64ALUInstruction {
public:
	X86_64AdcInst(const X86_64Src2Op &src2, const X86_64DstSrc1Op &dstsrc1)
			: X86_64ALUInstruction("X86_64AdcInst", dstsrc1, src2, 2) {
	}
};
class X86_64SbbInst : public X86_64ALUInstruction {
public:
	X86_64SbbInst(const X86_64Src2Op &src2, const X86_64DstSrc1Op &dstsrc1)
			: X86_64ALUInstruction("X86_64SbbInst", dstsrc1, src2, 3) {
	}
};
class X86_64AndInst : public X86_64ALUInstruction {
public:
	X86_64AndInst(const X86_64Src2Op &src2, const X86_64DstSrc1Op &dstsrc1)
			: X86_64ALUInstruction("X86_64AndInst", dstsrc1, src2, 4) {
	}
};
class X86_64SubInst : public X86_64ALUInstruction {
public:
	X86_64SubInst(const X86_64Src2Op &src2, const X86_64DstSrc1Op &dstsrc1)
			: X86_64ALUInstruction("X86_64SubInst", dstsrc1, src2, 5) {
	}
};
class X86_64XorInst : public X86_64ALUInstruction {
public:
	X86_64XorInst(const X86_64Src2Op &src2, const X86_64DstSrc1Op &dstsrc1)
			: X86_64ALUInstruction("X86_64XorInst", dstsrc1, src2, 6) {
	}
};
class X86_64CmpInst : public X86_64ALUInstruction {
public:
	/**
	 * TODO: return type actually is status-flags
	 */
	X86_64CmpInst(const X86_64Src2Op &src2, const X86_64Src1Op &src1)
			: X86_64ALUInstruction("X86_64CmpInst", src1, src2, 7) {
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

class X86_64IMulInst : public MachineInstruction {
public:
	X86_64IMulInst(const X86_64Src2Op &src2, const X86_64DstSrc1Op &dstsrc1)
			: MachineInstruction("X86_64IMulInst", dstsrc1.op, 2) {
		operands[0].op = dstsrc1.op;
		operands[1].op = src2.op;
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

class X86_64MovInst : public MachineMoveInst {
public:
	X86_64MovInst(const X86_64SrcOp &src, const X86_64DstOp &dst)
			: MachineMoveInst("X86_64MovInst", src.op, dst.op) {}
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
