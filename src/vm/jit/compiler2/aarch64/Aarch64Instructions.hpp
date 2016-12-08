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
#include "vm/jit/compiler2/aarch64/Aarch64Register.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/PatcherNew.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace aarch64 {

const u1 kStackSlotSize = 8;

// TODO: As soon as the wrappers are final, document them

struct SrcOp {
	MachineOperand *op;
	explicit SrcOp(MachineOperand *op) : op(op) {}
};

struct DstOp {
	MachineOperand *op;
	explicit DstOp(MachineOperand *op) : op(op) {}
};

struct BaseOp {
	MachineOperand *op;
	explicit BaseOp(MachineOperand *op) : op(op) {}
};

struct IdxOp {
	MachineOperand *op;
	explicit IdxOp(MachineOperand *op) : op(op) {}
};

template<> inline Immediate* cast_to<Immediate>(MachineOperand* op) {
	Immediate* imm = op->to_Immediate(); assert(imm);
	return imm;
}

template <>
inline StackSlot* cast_to<StackSlot>(MachineOperand *op) {
	switch (op->get_OperandID()) {
	case MachineOperand::ManagedStackSlotID:
		{
			ManagedStackSlot *mslot = op->to_ManagedStackSlot();
			assert(mslot);
			StackSlot *slot = mslot->to_StackSlot();
			assert(slot);
			return slot;
		}
	case MachineOperand::StackSlotID:
		{
			StackSlot *slot = op->to_StackSlot();
			assert(slot);
			return slot;
		}
	default: break;
	}
	assert(0 && "Not a stackslot");
	return NULL;
}


class AArch64Instruction : public MachineInstruction {
public:
	AArch64Instruction(const char* name, MachineOperand* result,
					   Type::TypeID type, std::size_t num_operands)
			: MachineInstruction(name, result, num_operands), 
			  result_type(type) {}

	Reg fromOp(MachineOperand* op, Type::TypeID type) const {
		Aarch64Register* reg = cast_to<Aarch64Register>(op);
		return regFromType(reg->index, type);
	}

	Reg reg_res() const {
		return fromOp(result.op, result_type);
	}

	Reg reg_op(std::size_t idx) const {
		assert(idx < operands.size());
		return fromOp(operands[idx].op, result_type);
	}

	virtual void emit(CodeMemory* cm) const {
		Emitter em;
		emit(em);
		em.emit(cm);
	}

	virtual void emit(Emitter& em) const {}
	Type::TypeID resultT() const { return result_type; }

private:
	Type::TypeID result_type;

	virtual Reg regFromType(u1 reg, Type::TypeID type) const {
		switch (type) {
		case Type::ByteTypeID:
		case Type::CharTypeID:
		case Type::ShortTypeID:
		case Type::IntTypeID: return Reg::W(reg);
		case Type::ReferenceTypeID:
		case Type::LongTypeID: return Reg::X(reg);
		case Type::FloatTypeID: return Reg::S(reg);
		case Type::DoubleTypeID: return Reg::D(reg);
		default: break;
		}
		ABORT_MSG("aarch64: regFromOpSize unsupported op_size", 
		          "Type: " << type);
		return Reg::XZR; // make compiler happy
	}
};


class MovImmInst : public AArch64Instruction {
public: 
	MovImmInst(const DstOp &dst, const SrcOp &src, Type::TypeID type)
			: AArch64Instruction("Aarch64MovImmInst", dst.op, type, 1) {
		operands[0].op = src.op;
	}

	virtual void emit(Emitter& em) const;

private:
	void emitIConst(Emitter& em) const;
	void emitLConst(Emitter& em) const;
};


class MovInst : public AArch64Instruction {
public:
	MovInst(const DstOp &dst, const SrcOp &src, Type::TypeID type)
			: AArch64Instruction("Aarch64MovInst", dst.op, type, 1) {
		operands[0].op = src.op;
	}

	virtual bool is_move() const { return true; }
	virtual void emit(Emitter& em) const;
};


class StoreInst : public AArch64Instruction {
public:
	StoreInst(const SrcOp &src, const BaseOp &base, const IdxOp &offset,
	          Type::TypeID type)
			: AArch64Instruction("Aarch64StoreInst", &NoOperand, type, 3) {
		operands[0].op = src.op;
		operands[1].op = base.op;
		operands[2].op = offset.op;
	}

	StoreInst(const SrcOp &src, const DstOp &dst, Type::TypeID type)
			: AArch64Instruction("Aarch64StoreInst", dst.op, type, 1) {
		operands[0].op = src.op;		
	}

	Reg reg_src() const {
		return reg_op(0);
	}

	Reg reg_base() const {
		if (result.op->is_StackSlot() || result.op->is_ManagedStackSlot()) {
			return Reg::XFP;
		}

		MachineOperand *op = get(1).op;
		if (op->is_Register()) {
			return reg_op(1);
		}  else {
			ABORT_MSG("MemoryInst::reg_base", op << " not supported");
		}
		assert(0);
	}

	s4 offset() const {
		if (result.op->is_StackSlot() || result.op->is_ManagedStackSlot()) {
			StackSlot* stackSlot = cast_to<StackSlot>(result.op);
			return stackSlot->get_index() * kStackSlotSize;
		}

		MachineOperand *op = this->get(2).op;
		if (op->is_Immediate()) {
			Immediate* imm = cast_to<Immediate>(op);
			return imm->get_Int();
		} else {
			ABORT_MSG("Memory::offset", op << " not supported");
		}
		assert(0);
	}
	
	virtual bool is_move() const { 
		return operands[0].op->is_Register() && 
		       (result.op->is_StackSlot() 
			   	|| result.op->is_ManagedStackSlot());
	}

	virtual void emit(Emitter& em) const;

private:
	virtual Reg regFromType(u1 reg, Type::TypeID type) const {
		switch (type) {
		case Type::ByteTypeID: return Reg::B(reg);
		case Type::ShortTypeID: return Reg::H(reg);
		case Type::IntTypeID: return Reg::W(reg);
		case Type::ReferenceTypeID:
		case Type::LongTypeID: return Reg::X(reg);
		case Type::FloatTypeID: return Reg::S(reg);
		case Type::DoubleTypeID: return Reg::D(reg);
		default: break;
		}
		ABORT_MSG("aarch64: regFromOpSize unsupported type", 
		          "Type: " << type);
		return Reg::XZR; // make compiler happy
	}
};


class LoadInst : public AArch64Instruction {
public:
	LoadInst(const DstOp &dst, const BaseOp &base, const IdxOp &offset,
	         Type::TypeID type)
			: AArch64Instruction("Aarch64LoadInst", dst.op, type, 2) {
		operands[0].op = base.op;
		operands[1].op = offset.op;
	}
	
	LoadInst(const DstOp &dst, const SrcOp &src, Type::TypeID type)
			: AArch64Instruction("Aarch64LoadInst", dst.op, type, 1) {
		operands[0].op = src.op;
	}

	Reg reg_base() const {
		MachineOperand *op = get(0).op;
		if (op->is_Register()) {
			return reg_op(0);
		} else if (op->is_StackSlot() || op->is_ManagedStackSlot()) {
			return Reg::XFP;
		} else {
			ABORT_MSG("MemoryInst::reg_base", op << " not supported");
		}
		assert(0);
	}

	s4 offset() const {
		if (get(0).op->is_StackSlot() || get(0).op->is_ManagedStackSlot()) {
			StackSlot* stackSlot = cast_to<StackSlot>(get(0).op);
			return stackSlot->get_index() * kStackSlotSize;
		}

		MachineOperand *op = get(1).op;
		if (op->is_Immediate()) {
			Immediate* imm = cast_to<Immediate>(op);
			return imm->get_Int();
		} else {
			ABORT_MSG("Memory::offset", op << " not supported");
		}
		assert(0);
	}

	virtual bool is_move() const { 
		return result.op->is_Register() && 
		       (operands[0].op->is_StackSlot() 
			   	|| operands[0].op->is_ManagedStackSlot());
	}
	virtual void emit(Emitter& em) const;

private:
	virtual Reg regFromType(u1 reg, Type::TypeID type) const {
		switch (type) {
		case Type::ByteTypeID: return Reg::B(reg);
		case Type::ShortTypeID: return Reg::H(reg);
		case Type::IntTypeID: return Reg::W(reg);
		case Type::ReferenceTypeID:
		case Type::LongTypeID: return Reg::X(reg);
		case Type::FloatTypeID: return Reg::S(reg);
		case Type::DoubleTypeID: return Reg::D(reg);
		default: break;
		}
		ABORT_MSG("aarch64: regFromOpSize unsupported type", 
		          "Type: " << type);
		return Reg::XZR; // make compiler happy
	}
};


class DsegAddrInst : public AArch64Instruction {
private:
	DataSegment::IdxTy data_index;
public:
	DsegAddrInst(const DstOp &dst, DataSegment::IdxTy &data_index, 
	             Type::TypeID type)
			: AArch64Instruction("Aarch64DsegAddrInst", dst.op, type, 0), 
		      data_index(data_index) {}

	virtual void emit(CodeMemory* cm) const;
	virtual void link(CodeFragment &CF) const;
};


class AddInst : public AArch64Instruction {
public:
	AddInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2,
			Type::TypeID type, Shift::SHIFT shift = Shift::LSL, u1 amount = 0)
			: AArch64Instruction("Aarch64AddInst", dst.op, type, 2), 
			  shift(shift), amount(amount) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;

private:
	Shift::SHIFT shift;
	u1 amount;
};


class SubInst : public AArch64Instruction {
public:
	SubInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2, 
	        Type::TypeID type)
			: AArch64Instruction("Aarch64SubInst", dst.op, type, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;
};


class MulInst : public AArch64Instruction {
public:
	MulInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2,
	        Type::TypeID type)
			: AArch64Instruction("Aarch64MulInst", dst.op, type, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;	
};


class MulSubInst : public AArch64Instruction {
public:
	MulSubInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2,
	           const SrcOp &src3, Type::TypeID type)
			: AArch64Instruction("Aarch64MulSubInst", dst.op, type, 3) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
		operands[2].op = src3.op;
	}

	virtual void emit(Emitter& em) const;
};


class DivInst : public AArch64Instruction {
public:
	DivInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2,
	        Type::TypeID type)
			: AArch64Instruction("Aarch64DivInst", dst.op, type, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;	
};

class NegInst : public AArch64Instruction {
public:
	NegInst(const DstOp &dst, const SrcOp &src, Type::TypeID type)
			: AArch64Instruction("Aarch64NegInst", dst.op, type, 1) {
		operands[0].op = src.op;
	}

	virtual void emit(Emitter& em) const;
};


class AndInst : public AArch64Instruction {
public:
	AndInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2,
	        Type::TypeID type, Shift::SHIFT shift = Shift::LSL, u1 amount = 0)
			: AArch64Instruction("Aarch64AndInst", dst.op, type, 2), 
			  shift(shift), amount(amount) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;

private:
	Shift::SHIFT shift;
	u1 amount;
};


class FCmpInst : public AArch64Instruction {
public:
	FCmpInst(const SrcOp &src1, const SrcOp &src2, Type::TypeID type)
			: AArch64Instruction("Aarch64FCmpInst", &NoOperand, type, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;
};


class FMovInst : public AArch64Instruction {
public:
	FMovInst(const DstOp &dst, const SrcOp &src, Type::TypeID type)
			: AArch64Instruction("Aarch64FMovInst", dst.op, type, 1) {
		operands[0].op = src.op;
	}

	virtual bool is_move() const { return true; }
	virtual void emit(Emitter& em) const;
};


class FAddInst : public AArch64Instruction {
public:
	FAddInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2,
	         Type::TypeID type)
			: AArch64Instruction("Aarch64FAddInst", dst.op, type, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;	
};


class FDivInst : public AArch64Instruction {
public:
	FDivInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2,
	         Type::TypeID type)
			: AArch64Instruction("Aarch64FDivInst", dst.op, type, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;
};


class FMulInst : public AArch64Instruction {
public:
	FMulInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2,
	         Type::TypeID type)
			: AArch64Instruction("Aarch64FMulInst", dst.op, type, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;	
};


class FSubInst : public AArch64Instruction {
public:
	FSubInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2,
	         Type::TypeID type)
			: AArch64Instruction("Aarch64FSubInst", dst.op, type, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;
};


class FNegInst : public AArch64Instruction {
public:
	FNegInst(const DstOp &dst, const SrcOp &src, Type::TypeID type)
			: AArch64Instruction("Aarch64FNegInst", dst.op, type, 1) {
		operands[0].op = src.op;
	}

	virtual void emit(Emitter& em) const;
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


class CmpInst : public AArch64Instruction {
public:
	CmpInst(const SrcOp &src1, const SrcOp &src2, Type::TypeID type)
		: AArch64Instruction("Aarch64CmpInst", &NoOperand, type, 2) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;
};


class CSelInst : public AArch64Instruction {
private:
	Cond::COND cond;
public:
	CSelInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2, 
	         Type::TypeID type, Cond::COND cond)
			: AArch64Instruction("Aarch64CSelInst", dst.op, type, 2), 
			  cond(cond) {
		operands[0].op = src1.op;
		operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;
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
public:
	LeaveInst() 
		: MachineInstruction("Aarch64LeaveInst", &NoOperand, 0) {}

	virtual void emit(CodeMemory* cm) const;
};


class RetInst : public MachineInstruction {
public:
	RetInst() : MachineInstruction("Aarch64RetInst", &NoOperand, 0) {}
	/**
	 * Non-void return. The source operand is only used to guide
	 * the register allocator. The user must ensure that the value
	 * really is in the correct register (e.g. by inserting a move)
	 */
	 RetInst(const SrcOp &src) 
	 		: MachineInstruction("Aarch64RetInst", &NoOperand, 1) {
		operands[0].op = src.op;
	}

	virtual bool is_end() const { return true; }
	virtual void emit(CodeMemory* cm) const;
};


class IntToFpInst : public AArch64Instruction {
public:
	IntToFpInst(const DstOp &dst, const SrcOp &src, Type::TypeID toType,
	            Type::TypeID fromType) 
			: AArch64Instruction("Aarch64IntToFpInst", dst.op, toType, 1),
			  from_type(fromType) {
		operands[0].op = src.op;
	}

	virtual void emit(Emitter& em) const;

private:
	Type::TypeID from_type;

	Reg reg_from() const {
		return fromOp(operands[0].op, from_type);
	}
};

class FcvtInst : public AArch64Instruction {
public:
	FcvtInst(const DstOp &dst, const SrcOp &src, Type::TypeID toType,
	         Type::TypeID fromType)  
			: AArch64Instruction("Aarch64FcvtInst", dst.op, toType, 1),
			  from_type(fromType) {
		operands[0].op = src.op;
	}

	virtual void emit(Emitter& em) const;

private:
	Type::TypeID from_type;

	Reg reg_from() const {
		return fromOp(operands[0].op, from_type);
	}
};

class IntToLongInst : public AArch64Instruction {
public:
	IntToLongInst(const DstOp &dst, const SrcOp &src)
			: AArch64Instruction("Aarch64IntToLongInst", dst.op, 
			                     Type::IntTypeID, 1) {
		operands[0].op = src.op;
	}

	virtual void emit(Emitter& em) const;
};


class LongToIntInst : public AArch64Instruction {
public:
	LongToIntInst(const DstOp &dst, const SrcOp &src)
			: AArch64Instruction("Aarch64LongToIntInst", dst.op, 
			                     Type::IntTypeID, 1) {
		operands[0].op = src.op;
	}

	virtual void emit(Emitter& em) const;
};


class IntegerToByteInst : public AArch64Instruction {
public:
	IntegerToByteInst(const DstOp &dst, const SrcOp &src, Type::TypeID type)
			: AArch64Instruction("Aarch64IntToByteInst", dst.op, type, 1) {
		operands[0].op = src.op;
	}

	virtual void emit(Emitter& em) const;
};


class IntToCharInst : public AArch64Instruction {
public:
	IntToCharInst(const DstOp &dst, const SrcOp &src)
			: AArch64Instruction("Aarch64IntToCharInst", dst.op, 
			                     Type::IntTypeID, 1) {
		operands[0].op = src.op;
	}

	virtual void emit(Emitter& em) const;
};


class IntegerToShortInst : public AArch64Instruction {
public:
	IntegerToShortInst(const DstOp &dst, const SrcOp &src, Type::TypeID type)
			: AArch64Instruction("Aarch64IntToShortInst", dst.op, type, 1) {
		operands[0].op = src.op;
	}

	virtual void emit(Emitter& em) const;
};


class PatchInst : public MachineInstruction {
private:
	Patcher *patcher;
public:
	PatchInst(Patcher *patcher)
			: MachineInstruction("Aarch64PatchInst", &NoOperand, 0),
			  patcher(patcher) {}

	virtual void emit(CodeMemory* cm) const;
	virtual void link(CodeFragment &cf) const;
};


class CallInst : public AArch64Instruction {
public:
	CallInst(const SrcOp &src, const DstOp &dst, std::size_t argc)
			: AArch64Instruction("Aarch64CallInst", dst.op, Type::LongTypeID,
			                     1 + argc) {
		operands[0].op = src.op;							 
	}

	virtual void emit(Emitter& em) const;
	virtual bool is_call() const { return true; }
};

} // end namespace aarch64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


#endif  /* _JIT_COMPILER2_AARCH64_INSTRUCTIONS */


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
