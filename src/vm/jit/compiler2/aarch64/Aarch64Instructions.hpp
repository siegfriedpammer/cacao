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


/**
 * Superclass for register instructions
 */
template<typename O, typename R>
class GPInstruction : public MachineInstruction {
public:
	GPInstruction(const char* name, MachineOperand* result, 
	              std::size_t num_operands)
	        : MachineInstruction(name, result, num_operands) {}

	O reg_op(std::size_t idx) const {
		assert(idx < operands.size());
		Aarch64Register* reg = cast_to<Aarch64Register>(operands[idx].op);
		return O(reg->index);
	}

	R reg_res() const {
		Aarch64Register* reg = cast_to<Aarch64Register>(result.op);
		return R(reg->index);
	}

	virtual void emit(CodeMemory* cm) const {
		Emitter em;
		emit(em);
		em.emit(cm);
	}

	virtual void emit(Emitter& em) const {}
};


template<typename T>
class MovImmInst : public GPInstruction<T, T> {
public: 
	MovImmInst(const DstOp &dst, const SrcOp &src)
			: GPInstruction<T, T>("Aarch64MovImmInst", dst.op, 1) {
		this->set_operand(0, src.op);
	}

	virtual void emit(Emitter& em) const;
};


template<typename T>
class MovInst : public GPInstruction<T, T> {
public:
	MovInst(const DstOp &dst, const SrcOp &src)
			: GPInstruction<T, T>("Aarch64MovInst", dst.op, 1) {
		this->set_operand(0, src.op);
	}

	virtual bool is_move() const { return true; }
	virtual void emit(Emitter& em) const;
};


template<typename T>
class MemoryInst : public MachineInstruction {
public:
	MemoryInst(const char* name, const DstOp &dst, const BaseOp &base, 
	           const IdxOp &offset)
			: MachineInstruction(name, dst.op, 2) {
		this->set_operand(0, base.op);
		this->set_operand(1, offset.op);
	}

	// Only used for StackSlot Operand at the moment
	MemoryInst(const char* name, const DstOp &dst, const SrcOp &src)
			: MachineInstruction(name, dst.op, 1) {
		this->set_operand(0, src.op);
	}

	T reg_res() const {
		Aarch64Register* reg = cast_to<Aarch64Register>(result.op);
		return T(reg->index);
	}

	X reg_base() const {
		MachineOperand *op = this->get(0).op;
		if (op->is_Register()) {
			Aarch64Register* reg = cast_to<Aarch64Register>(op);
			return X(reg->index);
		} else if (op->is_StackSlot() || op->is_ManagedStackSlot()) {
			return XFP;
		} else {
			ABORT_MSG("MemoryInst::reg_base", op << " not supported");
		}
		assert(0);
	}

	s4 offset() const {
		if (this->get(0).op->is_StackSlot() 
				|| this->get(0).op->is_ManagedStackSlot()) {
			StackSlot* stackSlot = cast_to<StackSlot>(this->get(0).op);
			return stackSlot->get_index() * kStackSlotSize;
		}

		MachineOperand *op = this->get(1).op;
		if (op->is_Immediate()) {
			Immediate* imm = cast_to<Immediate>(op);
			return imm->get_Int();
		} else {
			ABORT_MSG("Memory::offset", op << " not supported");
		}
		assert(0);
	}

	virtual void emit(CodeMemory* cm) const {
		Emitter em;
		emit(em);
		em.emit(cm);
	}

	virtual void emit(Emitter& em) const = 0;
};


template<typename T>
class StoreInst : public MachineInstruction {
public:
	StoreInst(const SrcOp &src, const BaseOp &base, const IdxOp &offset)
			: MachineInstruction("Aarch64StoreInst", &NoOperand, 3) {
		this->set_operand(0, src.op);
		this->set_operand(1, base.op);
		this->set_operand(2, offset.op);
	}

	StoreInst(const SrcOp &src, const DstOp &dst)
			: MachineInstruction("Aarch64StoreInst", dst.op, 1) {
		this->set_operand(0, src.op);		
	}

	T reg_src() const {
		Aarch64Register* reg = cast_to<Aarch64Register>(this->operands[0].op);
		return T(reg->index);
	}

	X reg_base() const {
		if (this->result.op->is_StackSlot() 
			|| this->result.op->is_ManagedStackSlot()) {
			return XFP;
		}

		MachineOperand *op = this->get(1).op;
		if (op->is_Register()) {
			Aarch64Register* reg = cast_to<Aarch64Register>(op);
			return X(reg->index);
		}  else {
			ABORT_MSG("MemoryInst::reg_base", op << " not supported");
		}
		assert(0);
	}

	s4 offset() const {
		if (this->result.op->is_StackSlot() 
				|| this->result.op->is_ManagedStackSlot()) {
			StackSlot* stackSlot = cast_to<StackSlot>(this->result.op);
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
		return this->operands[0].op->is_Register() && 
		       (this->result.op->is_StackSlot() 
			   	|| this->result.op->is_ManagedStackSlot());
	}

	virtual void emit(CodeMemory* cm) const {
		Emitter em;
		emit(em);
		em.emit(cm);
	}

	virtual void emit(Emitter& em) const;
};


template<typename T>
class LoadInst : public MemoryInst<T> {
public:
	LoadInst(const DstOp &dst, const BaseOp &base, const IdxOp &offset)
			: MemoryInst<T>("Aarch64LoadInst", dst, base, offset) {}
	
	LoadInst(const DstOp &dst, const SrcOp &src)
			: MemoryInst<T>("Aarch64LoadInst", dst, src) {}

	virtual bool is_move() const { 
		return this->result.op->is_Register() && 
		       (this->operands[0].op->is_StackSlot() 
			   	|| this->operands[0].op->is_ManagedStackSlot());
	}
	virtual void emit(Emitter& em) const;
};


template<typename T>
class DsegAddrInst : public GPInstruction<T, T> {
private:
	DataSegment::IdxTy data_index;
public:
	DsegAddrInst(const DstOp &dst, DataSegment::IdxTy &data_index)
			: GPInstruction<T, T>("Aarch64DsegAddrInst", dst.op, 0), 
		      data_index(data_index) {}

	virtual void emit(CodeMemory* cm) const;
	virtual void link(CodeFragment &CF) const;
};


template<typename T>
class AddInst : public GPInstruction<T, T> {
public:
	AddInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2,
			Shift::SHIFT shift = Shift::LSL, u1 amount = 0)
			: GPInstruction<T, T>("Aarch64AddInst", dst.op, 2), shift(shift),
			  amount(amount) {
		this->set_operand(0, src1.op);
		this->set_operand(1, src2.op);
	}

	virtual void emit(Emitter& em) const;

private:
	Shift::SHIFT shift;
	u1 amount;
};


template<typename T>
class SubInst : public GPInstruction<T, T> {
public:
	SubInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2)
			: GPInstruction<T, T>("Aarch64SubInst", dst.op, 2) {
		this->operands[0].op = src1.op;
		this->operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;
};


template<typename T>
class MulInst : public GPInstruction<T, T> {
public:
	MulInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2)
			: GPInstruction<T, T>("Aarch64MulInst", dst.op, 2) {
		this->operands[0].op = src1.op;
		this->operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;	
};


template<typename T>
class MulSubInst : public GPInstruction<T, T> {
public:
	MulSubInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2,
	           const SrcOp &src3)
			: GPInstruction<T, T>("Aarch64MulSubInst", dst.op, 3) {
		this->set_operand(0, src1.op);
		this->set_operand(1, src2.op);
		this->set_operand(2, src3.op);
	}

	virtual void emit(Emitter& em) const;
};


template<typename T>
class DivInst : public GPInstruction<T, T> {
public:
	DivInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2)
			: GPInstruction<T, T>("Aarch64DivInst", dst.op, 2) {
		this->operands[0].op = src1.op;
		this->operands[1].op = src2.op;
	}

	virtual void emit(Emitter& em) const;	
};


template<typename T>
class NegInst : public GPInstruction<T, T> {
public:
	NegInst(const DstOp &dst, const SrcOp &src)
			: GPInstruction<T, T>("Aarch64NegInst", dst.op, 1) {
		this->set_operand(0, src.op);
	}

	virtual void emit(Emitter& em) const;
};

template<typename T>
class AndInst : public GPInstruction<T, T> {
public:
	AndInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2,
			Shift::SHIFT shift = Shift::LSL, u1 amount = 0)
			: GPInstruction<T, T>("Aarch64AndInst", dst.op, 2), shift(shift),
			  amount(amount) {
		this->set_operand(0, src1.op);
		this->set_operand(1, src2.op);
	}

	virtual void emit(Emitter& em) const;

private:
	Shift::SHIFT shift;
	u1 amount;
};


template<typename T>
class FCmpInst : public GPInstruction<T, T> {
public:
	FCmpInst(const SrcOp &src1, const SrcOp &src2)
			: GPInstruction<T, T>("Aarch64FCmpInst", &NoOperand, 2) {
		this->set_operand(0, src1.op);
		this->set_operand(1, src2.op);
	}

	virtual void emit(Emitter& em) const;
};


template<typename T>
class FMovInst : public GPInstruction<T, T> {
public:
	FMovInst(const DstOp &dst, const SrcOp &src)
			: GPInstruction<T, T>("Aarch64FMovInst", dst.op, 1) {
		this->set_operand(0, src.op);
	}

	virtual bool is_move() const { return true; }
	virtual void emit(Emitter& em) const;
};


template<typename T>
class FAddInst : public GPInstruction<T, T> {
public:
	FAddInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2)
			: GPInstruction<T, T>("Aarch64FAddInst", dst.op, 2) {
		this->set_operand(0, src1.op);
		this->set_operand(1, src2.op);
	}

	virtual void emit(Emitter& em) const;	
};


template<typename T>
class FDivInst : public GPInstruction<T, T> {
public:
	FDivInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2)
			: GPInstruction<T, T>("Aarch64FDivInst", dst.op, 2) {
		this->set_operand(0, src1.op);
		this->set_operand(1, src2.op);
	}

	virtual void emit(Emitter& em) const;	
};


template<typename T>
class FMulInst : public GPInstruction<T, T> {
public:
	FMulInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2)
			: GPInstruction<T, T>("Aarch64FMulInst", dst.op, 2) {
		this->set_operand(0, src1.op);
		this->set_operand(1, src2.op);
	}

	virtual void emit(Emitter& em) const;	
};


template<typename T>
class FSubInst : public GPInstruction<T, T> {
public:
	FSubInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2)
			: GPInstruction<T, T>("Aarch64FSubInst", dst.op, 2) {
		this->set_operand(0, src1.op);
		this->set_operand(1, src2.op);
	}

	virtual void emit(Emitter& em) const;	
};


template<typename T>
class FNegInst : public GPInstruction<T, T> {
public:
	FNegInst(const DstOp &dst, const SrcOp &src)
			: GPInstruction<T, T>("Aarch64FNegInst", dst.op, 1) {
		this->set_operand(0, src.op);
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


template<typename T>
class CmpInst : public GPInstruction<T, T> {
public:
	CmpInst(const SrcOp &src1, const SrcOp &src2)
		: GPInstruction<T, T>("Aarch64CmpInst", &NoOperand, 2) {
		this->set_operand(0, src1.op);
		this->set_operand(1, src2.op);
	}

	virtual void emit(Emitter& em) const;
};


template<typename T>
class CSelInst : public GPInstruction<T, T> {
private:
	Cond::COND cond;
public:
	CSelInst(const DstOp &dst, const SrcOp &src1, const SrcOp &src2, 
	         Cond::COND cond)
			: GPInstruction<T, T>("Aarch64CSelInst", dst.op, 2), cond(cond) {
		this->set_operand(0, src1.op);
		this->set_operand(1, src2.op);
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


template<typename O, typename R>
class IntToFpInst : public GPInstruction<O, R> {
public:
	IntToFpInst(const DstOp &dst, const SrcOp &src) 
			: GPInstruction<O, R>("Aarch64IntToFpInst", dst.op, 1) {
		this->set_operand(0, src.op);
	}

	virtual void emit(Emitter& em) const;
};

template<typename O, typename R>
class FcvtInst : public GPInstruction<O, R> {
public:
	FcvtInst(const DstOp &dst, const SrcOp &src) 
			: GPInstruction<O, R>("Aarch64FcvtInst", dst.op, 1) {
		this->set_operand(0, src.op);
	}

	virtual void emit(Emitter& em) const;
};

class IntToLongInst : public GPInstruction<W, X> {
public:
	IntToLongInst(const DstOp &dst, const SrcOp &src)
			: GPInstruction<W, X>("Aarch64IntToLongInst", dst.op, 1) {
		operands[0].op = src.op;
	}

	virtual void emit(Emitter& em) const;
};

// Int and long to byte
template<typename T>
class IntToByteInst : public GPInstruction<T, T> {
public:
	IntToByteInst(const DstOp &dst, const SrcOp &src)
			: GPInstruction<T, T>("Aarch64IntToByteInst", dst.op, 1) {
		this->set_operand(0, src.op);
	}

	virtual void emit(Emitter& em) const;
};

class IntToCharInst : public GPInstruction<W, W> {
public:
	IntToCharInst(const DstOp &dst, const SrcOp &src)
			: GPInstruction<W, W>("Aarch64IntToCharInst", dst.op, 1) {
		this->set_operand(0, src.op);
	}

	virtual void emit(Emitter& em) const;
};

template<typename T>
class IntToShortInst : public GPInstruction<T, T> {
public:
	IntToShortInst(const DstOp &dst, const SrcOp &src)
			: GPInstruction<T, T>("Aarch64IntToShortInst", dst.op, 1) {
		this->set_operand(0, src.op);
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

// Separted all the template definitions to keep file size "reasonable"
#define _INSIDE_INSTRUCTIONS_HPP
#include "vm/jit/compiler2/aarch64/Aarch64InstructionsT.hpp"
#undef _INSIDE_INSTRUCTIONS_HPP

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
