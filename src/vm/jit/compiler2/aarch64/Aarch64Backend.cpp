/* src/vm/jit/compiler2/aachr64/Aarch64Backend.cpp

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

#include "vm/jit/compiler2/aarch64/Aarch64.hpp"
#include "vm/jit/compiler2/aarch64/Aarch64Backend.hpp"
#include "vm/jit/compiler2/aarch64/Aarch64Instructions.hpp"
#include "vm/jit/compiler2/aarch64/Aarch64Cond.hpp"
#include "vm/jit/compiler2/aarch64/Aarch64MachineMethodDescriptor.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MethodDescriptor.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"
#include "vm/jit/compiler2/StackSlotManager.hpp"
#include "vm/jit/compiler2/MatcherDefs.hpp"
#include "vm/jit/PatcherNew.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/code.hpp"
#include "vm/class.hpp"
#include "vm/field.hpp"

#include "md-trap.hpp"

#include "toolbox/OStream.hpp"
#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/aarch64"

namespace cacao {
namespace jit {
namespace compiler2 {

// BackendBase must be specialized in namespace compiler2!
using namespace aarch64;

template<>
const char* BackendBase<Aarch64>::get_name() const {
	return "aarch64";
}

namespace {
template <class I,class Seg>
static void write_data(Seg seg, I data) {
	assert(seg.size() == sizeof(I));

	for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
		seg[i] = (u1) 0xff & *(reinterpret_cast<u1*>(&data) + i);
	}
}

inline bool is_floatingpoint(Type::TypeID type) {
	return type == Type::DoubleTypeID || type == Type::FloatTypeID;
}

}

template<>
MachineInstruction* BackendBase<Aarch64>::create_Move(MachineOperand *src,
		MachineOperand* dst) const {
	Type::TypeID type = dst->get_type();

	assert(type == src->get_type());
	assert(!(src->is_stackslot() && dst->is_stackslot()));

	if (src->is_Immediate()) {
		switch (type) {
		case Type::CharTypeID:
		case Type::ByteTypeID:
		case Type::ShortTypeID:
		case Type::IntTypeID:
			return new MovImmInst<W>(DstOp(dst), SrcOp(src));
		case Type::LongTypeID:
		case Type::ReferenceTypeID:
			return new MovImmInst<X>(DstOp(dst), SrcOp(src));
		case Type::DoubleTypeID:
		{
			// TODO: check if this is the correct way of using the DSEG
			double imm = src->to_Immediate()->get_Double();
			DataSegment &ds = get_JITData()
							  ->get_CodeMemory()->get_DataSegment();
			DataFragment data = ds.get_Ref(sizeof(double));
			DataSegment::IdxTy idx = ds.insert_tag(DSDouble(imm), data);
			write_data<double>(data, imm);
			MachineInstruction *dseg = new DsegAddrInst<D>(DstOp(dst), idx);
			return dseg;
		}
		default:
			break;
		}
	}

	if (src->is_Register() && dst->is_Register()) {
		switch (type) {
		case Type::CharTypeID:
		case Type::ByteTypeID:
		case Type::ShortTypeID:
		case Type::IntTypeID:
			return new MovInst<W>(DstOp(dst), SrcOp(src));
		case Type::LongTypeID:
		case Type::ReferenceTypeID:
			return new MovInst<X>(DstOp(dst), SrcOp(src));
		case Type::DoubleTypeID:
			return new FMovInst<D>(DstOp(dst), SrcOp(src));
		case Type::FloatTypeID:
			return new FMovInst<S>(DstOp(dst), SrcOp(src));

		default:
			break;
		}
	}

	if (src->is_Register() && (dst->is_StackSlot() || dst->is_ManagedStackSlot())) {			
		switch (type) {
		case Type::DoubleTypeID:
			return new StoreInst<D>(SrcOp(src), DstOp(dst));
		default:
			break;
		}
	}

	if (dst->is_Register() && (src->is_StackSlot() || src->is_ManagedStackSlot())) {
		switch (type) {
		case Type::DoubleTypeID:
			return new aarch64::LoadInst<D>(DstOp(dst), SrcOp(src));
		default:
			break;
		}
	}

	ABORT_MSG("aarch64: Move not implemented",
		"Inst: " << src << " -> " << dst << " type: " << type);
	return NULL;
}

template<>
MachineInstruction* BackendBase<Aarch64>::create_Jump(MachineBasicBlock *target) const {
	return new JumpInst(target);
}

namespace {

template <unsigned size, class T>
inline T align_to(T val) {
	T rem =(val % size);
	return val + ( rem == 0 ? 0 : size - rem);
}

} // end anonymous namespace


template<>
void BackendBase<Aarch64>::create_frame(CodeMemory* cm,
		StackSlotManager* ssm) const {
	// Stackframe size + FP + LR
	int frameSize = align_to<16>(ssm->get_frame_size() + 16);
	EnterInst enter(frameSize);
	enter.emit(cm);
}

void Aarch64LoweringVisitor::visit(LOADInst *I, bool copyOperands) {
	assert(I);

	const MethodDescriptor &md = I->get_Method()->get_MethodDescriptor();
	const MachineMethodDescriptor mmd(md);

	Type::TypeID type = I->get_type();
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *move = NULL;

	MachineOperand *src_op = mmd[I->get_index()];
	if (src_op->is_Register()) {
		move = get_Backend()->create_Move(src_op, dst);
	} else {
		switch (type) {
		case Type::ByteTypeID:
		case Type::ShortTypeID:
		case Type::IntTypeID:
			move = new LoadInst<W>(DstOp(dst), SrcOp(src_op));
			break;
		case Type::LongTypeID:
		case Type::ReferenceTypeID:
			move = new LoadInst<X>(DstOp(dst), SrcOp(src_op));
			break;
		case Type::DoubleTypeID:
			move = new LoadInst<D>(DstOp(dst), SrcOp(src_op));
			break;
		default:
			ABORT_MSG("aarch64 type not supported: ", I << " type: " << type);
		}
	}
	get_current()->push_back(move);
	set_op(I,move->get_result().op);
}

void Aarch64LoweringVisitor::visit(CMPInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_operand(0)->get_type();
	assert(type == I->get_operand(1)->get_type());
	MachineInstruction* cmp;
	
	MachineOperand *dst = new VirtualRegister(Type::IntTypeID);
	MachineOperand *tmp1 = new VirtualRegister(Type::IntTypeID);
	MachineOperand *tmp2 = new VirtualRegister(Type::IntTypeID);

	if (!is_floatingpoint(type)) {
		ABORT_MSG("aarch64: Lowering not supported", 
				"Inst: " << I << " type: " << type);
	}

	if (type == Type::FloatTypeID) 
		cmp = new FCmpInst<S>(SrcOp(src_op1), SrcOp(src_op2));
	else
	    cmp = new FCmpInst<D>(SrcOp(src_op1), SrcOp(src_op2));
	get_current()->push_back(cmp);

	get_current()->push_back(
		new MovImmInst<W>(DstOp(dst), SrcOp(new Immediate(0, Type::IntType()))));
	get_current()->push_back(
		new MovImmInst<W>(DstOp(tmp1), SrcOp(new Immediate(1, Type::IntType()))));
	get_current()->push_back(
		new MovImmInst<W>(DstOp(tmp2), SrcOp(new Immediate(-1, Type::IntType()))));

	if (I->get_FloatHandling() == CMPInst::L) {
		/* set to -1 if less than or unordered (NaN) */
		/* set to 1 if greater than */
		get_current()->push_back(
			new CSelInst<W>(DstOp(tmp1), SrcOp(tmp1), SrcOp(tmp2), Cond::GT));		
	} else if (I->get_FloatHandling() == CMPInst::G) {
		/* set to 1 if greater than or unordered (NaN) */
		/* set to -1 if less than */
		get_current()->push_back(
			new CSelInst<W>(DstOp(tmp1), SrcOp(tmp1), SrcOp(tmp2), Cond::HI));
	} else {
		assert(0);
	}

	/* set to 0 if equal or result of previous csel */
	get_current()->push_back(
		new CSelInst<W>(DstOp(dst), SrcOp(dst), SrcOp(tmp1), Cond::EQ));
	set_op(I, dst);
}

void Aarch64LoweringVisitor::visit(IFInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	MachineInstruction* cmp;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	{
		if (type != Type::LongTypeID)
			cmp = new CmpInst<W>(SrcOp(src_op1), SrcOp(src_op2));
		else
			cmp = new CmpInst<X>(SrcOp(src_op1), SrcOp(src_op2));
		
		MachineInstruction* cjmp = NULL;
		BeginInstRef& then = I->get_then_target();
		BeginInstRef& els = I->get_else_target();

		switch (I->get_condition()) {
		case Conditional::EQ:
			cjmp = new CondJumpInst(Cond::EQ, get(then.get()),get(els.get()));
			break;
		case Conditional::LT:
			cjmp = new CondJumpInst(Cond::LT, get(then.get()),get(els.get()));
			break;
		case Conditional::LE:
			cjmp = new CondJumpInst(Cond::LE, get(then.get()),get(els.get()));
			break;
		case Conditional::GE:
			cjmp = new CondJumpInst(Cond::GE, get(then.get()),get(els.get()));
			break;
		case Conditional::GT:
			cjmp = new CondJumpInst(Cond::GT, get(then.get()),get(els.get()));
			break;
		case Conditional::NE:
			cjmp = new CondJumpInst(Cond::NE, get(then.get()),get(els.get()));
			break;
		default:
			ABORT_MSG("aarch64 Conditional not supported: ", I << "cond: "
					<< I->get_condition());
		}
		get_current()->push_back(cmp);
		get_current()->push_back(cjmp);

		set_op(I, cjmp->get_result().op);
		return;
	}
	default:
		break;
	}
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(NEGInst *I, bool copyOperands) {
	assert(I);
	Type::TypeID type = I->get_type();
	MachineOperand* src = get_op(I->get_operand(0)->to_Instruction());
	VirtualRegister* dst = new VirtualRegister(type);

	MachineInstruction* neg;

	switch (type) {
	case Type::IntTypeID:
		neg = new NegInst<W>(DstOp(dst), SrcOp(src));
		break;
	case Type::LongTypeID:
		neg = new NegInst<X>(DstOp(dst), SrcOp(src));
		break;
	case Type::DoubleTypeID:
		neg = new FNegInst<D>(DstOp(dst), SrcOp(src));
		break;
	case Type::FloatTypeID:
		neg = new FNegInst<S>(DstOp(dst), SrcOp(src));
		break;

	default:
		ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(neg);
	set_op(I, neg->get_result().op);
}

void Aarch64LoweringVisitor::visit(ADDInst *I, bool copyOperands) {
	assert(I);
	assert(copyOperands);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	assert(src_op1->is_Register() && src_op2->is_Register());
	
	Type::TypeID type = I->get_type();
	VirtualRegister* dst = new VirtualRegister(type);
	MachineInstruction* inst;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
		inst = new AddInst<W>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	case Type::LongTypeID:
		inst = new AddInst<X>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	case Type::DoubleTypeID:
		inst = new FAddInst<D>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	case Type::FloatTypeID:
		inst = new FAddInst<S>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	default:
		ABORT_MSG("aarch64: Lowering not supported",
				"Inst: " << I << " type: " << type);
	}

	get_current()->push_back(inst);
	set_op(I, inst->get_result().op);
}

void Aarch64LoweringVisitor::visit(ANDInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister* dst = new VirtualRegister(type);
	MachineInstruction* inst;
	
	switch (type) {
	case Type::IntTypeID:
		inst = new AndInst<W>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;
	case Type::LongTypeID:
		inst = new AndInst<X>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	default:
		ABORT_MSG("aarch64: Lowering not supported",
				"Inst: " << I << " type: " << type);
	}
	
	get_current()->push_back(inst);
	set_op(I, inst->get_result().op);
}

void Aarch64LoweringVisitor::visit(SUBInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister* dst = new VirtualRegister(type);
	MachineInstruction* inst;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
		inst = new SubInst<W>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;
	
	case Type::LongTypeID:
		inst = new SubInst<X>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;
	
	case Type::DoubleTypeID:
		inst = new FSubInst<D>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	case Type::FloatTypeID:
		inst = new FSubInst<S>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	default:
		ABORT_MSG("aarch64: Lowering not supported",
				"Inst: " << I << " type: " << type);
	}
	
	get_current()->push_back(inst);
	set_op(I, inst->get_result().op);
}

void Aarch64LoweringVisitor::visit(MULInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister* dst = new VirtualRegister(type);
	MachineInstruction* inst;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
		inst = new MulInst<W>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	case Type::LongTypeID:
		inst = new MulInst<X>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	case Type::DoubleTypeID:
		inst = new FMulInst<D>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	case Type::FloatTypeID:
		inst = new FMulInst<S>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;


	default:
		ABORT_MSG("aarch64: Lowering not supported",
				"Inst: " << I << " type: " << type);
	}

	get_current()->push_back(inst);
	set_op(I, inst->get_result().op);
}

void Aarch64LoweringVisitor::visit(DIVInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister* dst = new VirtualRegister(type);
	MachineInstruction* inst;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
		inst = new DivInst<W>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	case Type::LongTypeID:
		inst = new DivInst<X>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	case Type::DoubleTypeID:
		inst = new FDivInst<D>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;

	case Type::FloatTypeID:
		inst = new FDivInst<S>(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));
		break;
		
	default:
		ABORT_MSG("aarch64: Lowering not supported",
				"Inst: " << I << " type: " << type);
	}

	get_current()->push_back(inst);
	set_op(I, inst->get_result().op);
}

void Aarch64LoweringVisitor::visit(REMInst *I, bool copyOperands) {
	assert(I);

	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	MachineOperand *dividend = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand *tmp = new VirtualRegister(I->get_type());
	MachineOperand *dst = new VirtualRegister(I->get_type());
	MachineInstruction *div;
	MachineInstruction *msub;

	switch (type) {
	case Type::IntTypeID:
		div = new DivInst<W>(DstOp(tmp), SrcOp(dividend), SrcOp(src_op2));
		msub = new MulSubInst<W>(DstOp(dst), SrcOp(tmp), SrcOp(src_op2), 
			                     SrcOp(dividend));
		get_current()->push_back(div);
		get_current()->push_back(msub);
		set_op(I, msub->get_result().op);
		break;
	
	default:
		ABORT_MSG("aarch64: Lowering not supported",
				"Inst: " << I << " type: " << type);
	}	
}

void Aarch64LoweringVisitor::visit(ALOADInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_ref = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_index = get_op(I->get_operand(1)->to_Instruction());
	assert(src_ref->get_type() == Type::ReferenceTypeID);
	assert(src_index->get_type() == Type::IntTypeID);

	Type::TypeID type = I->get_type();
	MachineOperand *vreg = new VirtualRegister(type);
	//MachineOperand *base = new VirtualRegister(Type::LongTypeID);
	MachineOperand *base = new NativeRegister(Type::LongTypeID, &R9);
	Immediate *imm;

	s4 offset = 0;
	switch (type) {
	case Type::ByteTypeID:
		offset = OFFSET(java_bytearray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());

		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index)));
		get_current()->push_back(
			new LoadInst<B>(DstOp(vreg), BaseOp(base), IdxOp(imm)));
		break;
	case Type::ShortTypeID:
		offset = OFFSET(java_shortarray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());

		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index),
			               Shift::LSL, 1));
		get_current()->push_back(
			new LoadInst<H>(DstOp(vreg), BaseOp(base), IdxOp(imm)));
		break;
	case Type::IntTypeID:
		offset = OFFSET(java_intarray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());

		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index),
			               Shift::LSL, 2));
		get_current()->push_back(
			new LoadInst<W>(DstOp(vreg), BaseOp(base), IdxOp(imm)));
		break;
	case Type::LongTypeID:
		offset = OFFSET(java_longarray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());

		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index),
			               Shift::LSL, 3));
		get_current()->push_back(
			new LoadInst<X>(DstOp(vreg), BaseOp(base), IdxOp(imm)));
		break;
	case Type::ReferenceTypeID:
		offset = OFFSET(java_objectarray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());

		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index),
			               Shift::LSL, 3));
		get_current()->push_back(
			new LoadInst<X>(DstOp(vreg), BaseOp(base), IdxOp(imm)));
		break;
	case Type::FloatTypeID:
		offset = OFFSET(java_floatarray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());

		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index),
			               Shift::LSL, 2));
		get_current()->push_back(
			new LoadInst<S>(DstOp(vreg), BaseOp(base), IdxOp(imm)));
		break;
	case Type::DoubleTypeID:
		offset = OFFSET(java_doublearray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());

		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index),
			               Shift::LSL, 3));
		get_current()->push_back(
			new LoadInst<D>(DstOp(vreg), BaseOp(base), IdxOp(imm)));
		break;
	default:
		ABORT_MSG("aarch64 Lowering not supported",
			"Inst: " << I << " type: " << type);
	}

	set_op(I, vreg);
}

void Aarch64LoweringVisitor::visit(ASTOREInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_ref = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_index = get_op(I->get_operand(1)->to_Instruction());
	MachineOperand* src_value = get_op(I->get_operand(2)->to_Instruction());
	assert(src_ref->get_type() == Type::ReferenceTypeID);
	assert(src_index->get_type() == Type::IntTypeID);

	//MachineOperand *base = new VirtualRegister(Type::LongTypeID);
	MachineOperand *base = new NativeRegister(Type::LongTypeID, &R9);
	Immediate *imm;
	Type::TypeID type = I->get_array_type();

	s4 offset = 0;
	switch (type) {
	case Type::ByteTypeID:
		offset = OFFSET(java_bytearray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());
		
		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index)));
		get_current()->push_back(
			new StoreInst<B>(SrcOp(src_value), BaseOp(base), IdxOp(imm)));
		break;
	case Type::ShortTypeID:
		offset = OFFSET(java_shortarray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());
		
		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index),
						   Shift::LSL, 1));
		get_current()->push_back(
			new StoreInst<H>(SrcOp(src_value), BaseOp(base), IdxOp(imm)));
		break;
	case Type::IntTypeID:
		offset = OFFSET(java_intarray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());

		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index),
			               Shift::LSL, 2));
		get_current()->push_back(
			new StoreInst<W>(SrcOp(src_value), BaseOp(base), IdxOp(imm)));
		break;
	case Type::LongTypeID:
		offset = OFFSET(java_longarray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());

		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index),
			               Shift::LSL, 3));
		get_current()->push_back(
			new StoreInst<X>(SrcOp(src_value), BaseOp(base), IdxOp(imm)));
		break;
	case Type::ReferenceTypeID:
		offset = OFFSET(java_objectarray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());

		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index),
			               Shift::LSL, 3));
		get_current()->push_back(
			new StoreInst<X>(SrcOp(src_value), BaseOp(base), IdxOp(imm)));
		break;
	case Type::FloatTypeID:
		offset = OFFSET(java_floatarray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());

		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index),
			               Shift::LSL, 2));
		get_current()->push_back(
			new StoreInst<S>(SrcOp(src_value), BaseOp(base), IdxOp(imm)));
		break;
	case Type::DoubleTypeID:
		offset = OFFSET(java_doublearray_t, data[0]);
		imm = new Immediate(offset, Type::IntType());

		get_current()->push_back(
			new AddInst<X>(DstOp(base), SrcOp(src_ref), SrcOp(src_index),
			               Shift::LSL, 3));
		get_current()->push_back(
			new StoreInst<D>(SrcOp(src_value), BaseOp(base), IdxOp(imm)));
		break;
	default:
		ABORT_MSG("aarch64 Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
}

void Aarch64LoweringVisitor::visit(ARRAYLENGTHInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
	assert(I->get_type() == Type::IntTypeID);
	assert(src_op->get_type() == Type::ReferenceTypeID);
	MachineOperand *vreg = new VirtualRegister(Type::IntTypeID);
	Immediate *imm = new Immediate(OFFSET(java_array_t, size), Type::IntType());

	MachineInstruction *load = 
		new LoadInst<W>(DstOp(vreg), BaseOp(src_op), IdxOp(imm));
	get_current()->push_back(load);
	set_op(I, load->get_result().op);
}

void Aarch64LoweringVisitor::visit(ARRAYBOUNDSCHECKInst *I, bool copyOperands) {
	//Type::TypeID type = I->get_type();
	//ABORT_MSG("aarch64: Lowering not supported",
	//		"Inst: " << I << " type: " << type);
	// TODO: implement
}

void Aarch64LoweringVisitor::visit(RETURNInst *I, bool copyOperands) {
	assert(I);
	Type::TypeID type = I->get_type();
	MachineOperand* src_op = type == Type::VoidTypeID ?
			0 : get_op(I->get_operand(0)->to_Instruction());

	
	MachineInstruction *mov = NULL;
	LeaveInst *leave = new LeaveInst();

	RetInst *ret = NULL;

	switch (type) {
	case Type::CharTypeID:
	case Type::ByteTypeID:
	case Type::ShortTypeID:
	case Type::IntTypeID:
	{
		MachineOperand *ret_reg = new NativeRegister(type, &R0);
		mov = new MovInst<W>(DstOp(ret_reg), SrcOp(src_op));
		break;
	}
	case Type::LongTypeID:
	{
		MachineOperand *ret_reg = new NativeRegister(type, &R0);
		mov = new MovInst<X>(DstOp(ret_reg), SrcOp(src_op));
		break;
	}
	case Type::DoubleTypeID:
	{
		MachineOperand *ret_reg = new NativeRegister(type, &V0);
		mov = new FMovInst<D>(DstOp(ret_reg), SrcOp(src_op));
		break;
	}
	case Type::FloatTypeID:
	{
		MachineOperand *ret_reg = new NativeRegister(type, &V0);
		mov = new FMovInst<S>(DstOp(ret_reg), SrcOp(src_op));
		break;
	}
	case Type::VoidTypeID: 
		break;

	default:
		ABORT_MSG("aarch64: Lowering not supported",
		"Inst: " << I << " type: " << type);
	}

	
	if (type != Type::VoidTypeID) {
		get_current()->push_back(mov);
		set_op(I, mov->get_result().op);
		ret = new RetInst(SrcOp(mov->get_result().op));
	} else {
		ret = new RetInst();
	}
	get_current()->push_back(leave);
	get_current()->push_back(ret);
}

void Aarch64LoweringVisitor::visit(CASTInst *I, bool copyOperands) {
	MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
	Type::TypeID from = I->get_operand(0)->to_Instruction()->get_type();
	Type::TypeID to = I->get_type();

	switch (from) {
	case Type::LongTypeID:
	{
		switch (to) {
		case Type::DoubleTypeID:
		{
			MachineInstruction *conv = new IntToFpInst<X, D>(
				DstOp(new VirtualRegister(to)), SrcOp(src_op));
			get_current()->push_back(conv);
			set_op(I, conv->get_result().op);
			return;
		}
		default:
			break;
		}
		break;
	}
	case Type::IntTypeID:
	{
		switch (to) {
		case Type::ByteTypeID:
		{
			MachineInstruction *conv = 
				new IntToByteInst<W>(DstOp(new VirtualRegister(to)), SrcOp(src_op));
			get_current()->push_back(conv);
			set_op(I, conv->get_result().op);
			return;
		}
		case Type::ShortTypeID:
		{
			MachineInstruction *conv = 
				new IntToShortInst<W>(DstOp(new VirtualRegister(to)), SrcOp(src_op));
			get_current()->push_back(conv);
			set_op(I, conv->get_result().op);
			return;
		}
		case Type::CharTypeID:
		{
			MachineInstruction *conv = 
				new IntToCharInst(DstOp(new VirtualRegister(to)), SrcOp(src_op));
			get_current()->push_back(conv);
			set_op(I, conv->get_result().op);
			return;
		}
		case Type::LongTypeID:
		{
			MachineInstruction *conv = 
				new IntToLongInst(DstOp(new VirtualRegister(to)), SrcOp(src_op));
			get_current()->push_back(conv);
			set_op(I, conv->get_result().op);
			return;
		}
		case Type::FloatTypeID:
		{
			MachineInstruction *conv = 
				new IntToFpInst<W, S>(DstOp(new VirtualRegister(to)), SrcOp(src_op));
			get_current()->push_back(conv);
			set_op(I, conv->get_result().op);
			return;
		}
		case Type::DoubleTypeID:
		{
			MachineInstruction *conv = 
				new IntToFpInst<W, D>(DstOp(new VirtualRegister(to)), SrcOp(src_op));
			get_current()->push_back(conv);
			set_op(I, conv->get_result().op);
			return;			
		}
		default:
			break;
		break;
		}
	}
	case Type::DoubleTypeID:
	{
		switch (to) {
		case Type::FloatTypeID:
		{
			MachineInstruction *conv = 
				new FcvtInst<D, S>(DstOp(new VirtualRegister(to)), SrcOp(src_op));
			get_current()->push_back(conv);
			set_op(I, conv->get_result().op);
			return;
		}
		default:
			break;
		break;
		}
	}
	case Type::FloatTypeID:
	{
		switch (to) {
		case Type::DoubleTypeID:
		{
			MachineInstruction *conv = 
				new FcvtInst<S, D>(DstOp(new VirtualRegister(to)), SrcOp(src_op));
			get_current()->push_back(conv);
			set_op(I, conv->get_result().op);
			return;
		}
		default:
			break;
		break;
		}
	}
	default:
		break;
	}
	ABORT_MSG("aarch64: Cast not supported!", "From " << from << " to " << to );
}

void Aarch64LoweringVisitor::visit(INVOKESTATICInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}


void Aarch64LoweringVisitor::visit(GETSTATICInst *I, bool copyOperands) {
	assert(I);
	DataSegment &ds = get_Backend()->get_JITData()->get_CodeMemory()
	                  ->get_DataSegment();
	constant_FMIref* fmiref = I->get_fmiref();
	DataSegment::IdxTy idx = ds.get_index(DSFMIRef(fmiref));
	size_t size = sizeof(void*);
	if (DataSegment::is_invalid(idx)) {
		DataFragment datafrag = ds.get_Ref(size);
		idx = ds.insert_tag(DSFMIRef(fmiref), datafrag);
	}

	if (I->is_resolved()) {
		fieldinfo* fi = I->get_fmiref()->p.field;

		if (!class_is_or_almost_initialized(fi->clazz)) {
			//PROFILE_CYCLE_STOP;
			Patcher *patcher = new InitializeClassPatcher(fi->clazz);
			PatcherPtrTy ptr(patcher);
			get_Backend()->get_JITData()->get_jitdata()->code->patchers
				->push_back(ptr);
			MachineInstruction *pi = new PatchInst(patcher);
			get_current()->push_back(pi);
			//PROFILE_CYCLE_START;
		}
		DataFragment datafrag = ds.get_Ref(idx, size);
		// write data
		write_data<void*>(datafrag, fmiref->p.field->value);
	} else {
		assert(0 && "Not yet implemented");
	}
	VirtualRegister *addr = new VirtualRegister(Type::ReferenceTypeID);
	MachineInstruction *dseg = new DsegAddrInst<X>(DstOp(addr), idx);
	MachineOperand *vreg = new VirtualRegister(I->get_type());
	MachineInstruction *load = NULL;
	Immediate *imm = new Immediate(0, Type::IntType());

	switch (I->get_type()) {
	case Type::IntTypeID:
		load = new LoadInst<W>(DstOp(vreg), BaseOp(addr), IdxOp(imm));
		break;
	case Type::LongTypeID:
		load = new LoadInst<X>(DstOp(vreg), BaseOp(addr), IdxOp(imm));
		break;		
	default:
		ABORT_MSG("aarch64: Type not implemented.", 
			"Inst: " << I << " type: " << I->get_type());
	}

	get_current()->push_back(dseg);
	get_current()->push_back(load);
	set_op(I, load->get_result().op);
}

void Aarch64LoweringVisitor::visit(LOOKUPSWITCHInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
	Type::TypeID type = I->get_type();

	LOOKUPSWITCHInst::succ_const_iterator s = I->succ_begin();
	for(LOOKUPSWITCHInst::match_iterator i = I->match_begin(),
			e = I->match_end(); i != e; ++i) {
		// create compare
		Immediate *imm = new Immediate(*i, Type::IntType());
		MachineOperand *cmpOp = imm;
		if (*i < 0 || *i > 4095) {
			cmpOp = new VirtualRegister(type);
			MachineInstruction *mov = get_Backend()->create_Move(imm, cmpOp);
			get_current()->push_back(mov);
		}

		MachineInstruction *cmp = NULL;
		switch (type) {
		case Type::IntTypeID:
			cmp = new CmpInst<W>(SrcOp(src_op), SrcOp(cmpOp));
			break;
		default:
			UNIMPLEMENTED_MSG("aarch64: LOOKUPSWITCHInst type: " << type);
		} 
		get_current()->push_back(cmp);
		// create new block
		MachineBasicBlock *then_block = get(s->get());
		MachineBasicBlock *else_block = new_block();
		assert(else_block);
		else_block->insert_pred(get_current());
		else_block->push_front(new MachineLabelInst());
		// create cond jump
		MachineInstruction *cjmp = new CondJumpInst(Cond::EQ, then_block, else_block);
		get_current()->push_back(cjmp);
		// set current
		set_current(else_block);
		++s;
	}

	// default
	MachineInstruction *jmp = new JumpInst(get(s->get()));
	get_current()->push_back(jmp);
	assert(++s == I->succ_end());

	set_op(I,jmp->get_result().op);
}

void Aarch64LoweringVisitor::visit(TABLESWITCHInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::lowerComplex(Instruction* I, int ruleId){
	ABORT_MSG("Rule not supported", "Rule " 
			<< ruleId << " is not supported by method lowerComplex!");
}

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


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
