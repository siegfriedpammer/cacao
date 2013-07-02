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
namespace x86_64 {

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
struct SrcOp {
	MachineOperand *op;
	explicit SrcOp(MachineOperand *op) : op(op) {}
};
/**
 * Simple wrapper for first operand of an
 * x86_64 instruction.
 */
struct Src1Op {
	MachineOperand *op;
	explicit Src1Op(MachineOperand *op) : op(op) {}
};
/**
 * Simple wrapper for second operand of an
 * x86_64 instruction.
 */
struct Src2Op {
	MachineOperand *op;
	explicit Src2Op(MachineOperand *op) : op(op) {}
};
/**
 * Simple wrapper for first operand of an
 * x86_64 instruction which is also used for the result.
 */
struct DstSrc1Op {
	MachineOperand *op;
	explicit DstSrc1Op(MachineOperand *op) : op(op) {}
};

/**
 * Simple wrapper for destination of an
 * x86_64 instruction.
 */
struct DstOp {
	MachineOperand *op;
	explicit DstOp(MachineOperand *op) : op(op) {}
};


/**
 * Superclass for general purpose register instruction
 */
class X86_64Instruction : public MachineInstruction {
public:
	X86_64Instruction(const char *name, MachineOperand* result,
		unsigned num_operands) :
			MachineInstruction(name, result, num_operands) {}
};
/**
 * Superclass for general purpose register instruction
 */
class GPInstruction : public X86_64Instruction {
public:
	enum OperandSize {
		OS_8  = 1,
		OS_16 = 2,
		OS_32 = 4,
		OS_64 = 8,
		NO_SIZE = 0
	};
	enum OperandType {
		// Reg
		Reg8,
		Reg16,
		Reg32,
		Reg64,
		// Memory
		Mem8,
		Mem16,
		Mem32,
		Mem64,
		//
		NO_RESULT
	};
	enum OpEncoding {
		// Immediate to Register
		Reg8Imm8,
		Reg16Imm16,
		Reg32Imm32,
		Reg64Imm64,
		// Immediate to Memory
		Mem8Imm8,
		Mem16Imm16,
		Mem32Imm32,
		Mem64Imm64,
		// Immediate 8 to Register
		Reg16Imm8,
		Reg32Imm8,
		Reg64Imm8,
		// Register to Register
		RegReg8,
		RegReg16,
		RegReg32,
		RegReg64,
		// Memory to Register
		RegMem8,
		RegMem16,
		RegMem32,
		RegMem64,
		// Register to Memory
		MemReg8,
		MemReg16,
		MemReg32,
		MemReg64,
		//
		NO_ENCODING
	};
private:
	OperandSize op_size;
public:
	GPInstruction(const char * name, MachineOperand* result,
		OperandSize op_size, unsigned num_operands) :
			X86_64Instruction(name, result, num_operands),
			op_size(op_size) {}

	OperandSize get_op_size() const { return op_size; }
};

/**
 * Move super instruction
 */
class MoveInst : public GPInstruction {
public:
	MoveInst( const char* name,
			MachineOperand *src,
			MachineOperand *dst,
			OperandSize op_size)
			: GPInstruction(name, dst, op_size, 1) {
		operands[0].op = src;
	}
	virtual bool is_move() const { return true; }
};

GPInstruction::OperandSize get_OperandSize_from_Type(const Type::TypeID type);

/**
 * This abstract class represents a x86_64 ALU instruction.
 *
 *
 */
class ALUInstruction : public GPInstruction {
private:
	u1 alu_id;

#if 0
void emit_impl_I(CodeMemory* CM, Immediate* imm) const;
void emit_impl_RI(CodeMemory* CM, NativeRegister* src1,
	Immediate* imm) const;
void emit_impl_MR(CodeMemory* CM, NativeRegister* src1,
	NativeRegister* src2) const;
void emit_impl_RR(CodeMemory* CM, NativeRegister* src1,
	NativeRegister* src2) const;
#endif

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
	ALUInstruction(const char * name, const DstSrc1Op &dstsrc1,
			const Src2Op &src2, OperandSize op_size, u1 alu_id)
			: GPInstruction(name, dstsrc1.op, op_size, 2) , alu_id(alu_id) {
		assert(alu_id < 8);
		operands[0].op = dstsrc1.op;
		operands[1].op = src2.op;
	}
	ALUInstruction(const char * name, const Src1Op &src1,
			const Src2Op &src2, OperandSize op_size, u1 alu_id)
			: GPInstruction(name, &NoOperand, op_size, 2) , alu_id(alu_id) {
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
class AddInst : public ALUInstruction {
public:
	AddInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64AddInst", dstsrc1, src2, op_size, 0) {
	}
};
class OrInst : public ALUInstruction {
public:
	OrInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64OrInst", dstsrc1, src2, op_size, 1) {
	}
};
class AdcInst : public ALUInstruction {
public:
	AdcInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64AdcInst", dstsrc1, src2, op_size, 2) {
	}
};
class SbbInst : public ALUInstruction {
public:
	SbbInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64SbbInst", dstsrc1, src2, op_size, 3) {
	}
};
class AndInst : public ALUInstruction {
public:
	AndInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64AndInst", dstsrc1, src2, op_size, 4) {
	}
};
class SubInst : public ALUInstruction {
public:
	SubInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64SubInst", dstsrc1, src2, op_size, 5) {
	}
};
class XorInst : public ALUInstruction {
public:
	XorInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: ALUInstruction("X86_64XorInst", dstsrc1, src2, op_size, 6) {
	}
};
class CmpInst : public ALUInstruction {
public:
	/**
	 * TODO: return type actually is status-flags
	 */
	CmpInst(const Src2Op &src2, const Src1Op &src1,
		OperandSize op_size)
			: ALUInstruction("X86_64CmpInst", src1, src2, op_size, 7) {
	}
};




class EnterInst : public X86_64Instruction {
private:
	u2 framesize;
public:
	EnterInst(u2 framesize)
			: X86_64Instruction("X86_64EnterInst", &NoOperand, 0),
			framesize(framesize) {}
	virtual void emit(CodeMemory* CM) const;
};

class LeaveInst : public X86_64Instruction {
public:
	LeaveInst()
			: X86_64Instruction("X86_64LeaveInst", &NoOperand, 0) {}
	virtual void emit(CodeMemory* CM) const;
};

class CondJumpInst : public X86_64Instruction {
private:
	Cond::COND cond;
	BeginInstRef &target;
public:
	CondJumpInst(Cond::COND cond, BeginInstRef &target)
			: X86_64Instruction("X86_64CondJumpInst", &NoOperand, 0),
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

class IMulInst : public GPInstruction {
public:
	IMulInst(const Src2Op &src2, const DstSrc1Op &dstsrc1,
		OperandSize op_size)
			: GPInstruction("X86_64IMulInst", dstsrc1.op, op_size, 2) {
		operands[0].op = dstsrc1.op;
		operands[1].op = src2.op;
	}
	virtual void emit(CodeMemory* CM) const;
};

class RetInst : public GPInstruction {
public:
	RetInst(OperandSize op_size)
			: GPInstruction("X86_64RetInst", &NoOperand, op_size, 0) {
	}
	virtual void emit(CodeMemory* CM) const;
};

class MovInst : public MoveInst {
public:
	MovInst(const SrcOp &src, const DstOp &dst,
		GPInstruction::OperandSize op_size)
			: MoveInst("X86_64MovInst", src.op, dst.op, op_size) {}
	virtual bool accepts_immediate(unsigned i, Immediate *imm) const {
		return true;
	}
	virtual void emit(CodeMemory* CM) const;
};

/**
 * Move with Sign-Extension
 */
class MovSXInst : public MoveInst {
GPInstruction::OperandSize from;
GPInstruction::OperandSize to;
public:
	MovSXInst(const SrcOp &src, const DstOp &dst,
			GPInstruction::OperandSize from, GPInstruction::OperandSize to)
				: MoveInst("X86_64MovSXInst", src.op, dst.op, to),
				from(from), to(to) {}
	virtual void emit(CodeMemory* CM) const;
};

class JumpInst : public MachineJumpInst {
private:
	BeginInstRef &target;
public:
	JumpInst(BeginInstRef &target) : MachineJumpInst("X86_64JumpInst"),
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
// Double & Float operations

class AddSDInst : public GPInstruction {
public:
	AddSDInst(const Src2Op &src2, const DstSrc1Op &dstsrc1)
			: GPInstruction("X86_64AddSDInst", dstsrc1.op, OS_64, 2) {
		operands[0].op = dstsrc1.op;
		operands[1].op = src2.op;
	}
	virtual void emit(CodeMemory* CM) const;
};

class AddSSInst : public GPInstruction {
public:
	AddSSInst(const Src2Op &src2, const DstSrc1Op &dstsrc1)
			: GPInstruction("X86_64AddSSInst", dstsrc1.op, OS_32, 2) {
		operands[0].op = dstsrc1.op;
		operands[1].op = src2.op;
	}
	virtual void emit(CodeMemory* CM) const;
};

class MovSDInst : public MoveInst {
public:
	MovSDInst(const SrcOp &src, const DstOp &dst)
			: MoveInst("X86_64MovSDInst", src.op, dst.op, OS_64) {}
	virtual void emit(CodeMemory* CM) const;
};

class MovSSInst : public MoveInst {
public:
	MovSSInst(const SrcOp &src, const DstOp &dst)
			: MoveInst("X86_64MovSSInst", src.op, dst.op, OS_32) {}
	virtual void emit(CodeMemory* CM) const;
};

} // end namespace x86_64
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
