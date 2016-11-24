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
// #include "vm/jit/compiler2/x86_64/X86_64Register.hpp"
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

template<>
MachineInstruction* BackendBase<Aarch64>::create_Move(MachineOperand *src,
		MachineOperand* dst) const {
	Type::TypeID type = dst->get_type();

	assert(type == src->get_type());
	assert(!(src->is_stackslot() && dst->is_stackslot()));

	switch (type) {
	case Type::CharTypeID:
	case Type::ByteTypeID:
	case Type::ShortTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	case Type::ReferenceTypeID:
	{
		return new MovInst(DstOp(dst), SrcOp(src));
	}
	default:
		break;
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
	EnterInst enter(align_to<16>(ssm->get_frame_size() + 16));
	enter.emit(cm);
}

void Aarch64LoweringVisitor::visit(LOADInst *I, bool copyOperands) {
	assert(I);

	const MethodDescriptor &md = I->get_Method()->get_MethodDescriptor();
	const MachineMethodDescriptor mmd(md);

	Type::TypeID type = I->get_type();
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *move = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::ShortTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	case Type::ReferenceTypeID:
		move = new MovInst(
			DstOp(dst),
			SrcOp(mmd[I->get_index()]));
			break;

	default:
		ABORT_MSG("aarch64 type not supported: ", I << " type: " << type);
	}
	get_current()->push_back(move);
	set_op(I,move->get_result().op);
}

void Aarch64LoweringVisitor::visit(CMPInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported", 
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(IFInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	{
		CmpInst* cmp = new CmpInst(SrcOp(src_op1), SrcOp(src_op2));
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
		return;
	}
	default:
		break;
	}
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(NEGInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(ADDInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister* dst = new VirtualRegister(type);
	MachineInstruction* inst = new AddInst(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		get_current()->push_back(inst);
		set_op(I, inst->get_result().op);
		return;

	default:
		ABORT_MSG("aarch64: Lowering not supported",
				"Inst: " << I << " type: " << type);
	}
}

void Aarch64LoweringVisitor::visit(ANDInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(SUBInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister* dst = new VirtualRegister(type);
	MachineInstruction* inst = new SubInst(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		get_current()->push_back(inst);
		set_op(I, inst->get_result().op);
		return;

	default:
		ABORT_MSG("aarch64: Lowering not supported",
				"Inst: " << I << " type: " << type);
	}
}

void Aarch64LoweringVisitor::visit(MULInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister* dst = new VirtualRegister(type);
	MachineInstruction* inst = new MulInst(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		get_current()->push_back(inst);
		set_op(I, inst->get_result().op);
		return;

	default:
		ABORT_MSG("aarch64: Lowering not supported",
				"Inst: " << I << " type: " << type);
	}
}

void Aarch64LoweringVisitor::visit(DIVInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister* dst = new VirtualRegister(type);
	MachineInstruction* inst = new DivInst(DstOp(dst), SrcOp(src_op1), SrcOp(src_op2));

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		get_current()->push_back(inst);
		set_op(I, inst->get_result().op);
		return;

	default:
		ABORT_MSG("aarch64: Lowering not supported",
				"Inst: " << I << " type: " << type);
	}
}

void Aarch64LoweringVisitor::visit(REMInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(ALOADInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(ASTOREInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(ARRAYLENGTHInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(ARRAYBOUNDSCHECKInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(RETURNInst *I, bool copyOperands) {
	assert(I);
	Type::TypeID type = I->get_type();
	MachineOperand* src_op = type == Type::VoidTypeID ?
			0 : get_op(I->get_operand(0)->to_Instruction());

	// TODO: I need to framesize for LEAVE, how do I get it correctly?
	StackSlotManager* ssm =
		get_Backend()->get_JITData()->get_StackSlotManager();

	switch (type) {
	case Type::CharTypeID:
	case Type::ByteTypeID:
	case Type::ShortTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	{
		MachineOperand *ret_reg = new NativeRegister(type, &R0);
		MachineInstruction *mov = new MovInst(DstOp(ret_reg), SrcOp(src_op));
		LeaveInst *leave = new LeaveInst(align_to<16>(ssm->get_frame_size() + 16));
		RetInst *ret = new RetInst();
		get_current()->push_back(mov);
		get_current()->push_back(leave);
		get_current()->push_back(ret);
		set_op(I,ret->get_result().op);
		return;
	}

	default:
		break;
	}
	ABORT_MSG("aarch64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(CASTInst *I, bool copyOperands) {
	Type::TypeID from = I->get_operand(0)->to_Instruction()->get_type();
	Type::TypeID to = I->get_type();
	ABORT_MSG("aarch64: Cast not supported!", "From " << from << " to " << to );
}

void Aarch64LoweringVisitor::visit(INVOKESTATICInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}


namespace {

inline bool is_floatingpoint(Type::TypeID type) {
	return type == Type::DoubleTypeID || type == Type::FloatTypeID;
}
template <class I,class Seg>
static void write_data(Seg seg, I data) {
	assert(seg.size() == sizeof(I));

	for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
		seg[i] = (u1) 0xff & *(reinterpret_cast<u1*>(&data) + i);
	}

}
} // end anonymous namespace


void Aarch64LoweringVisitor::visit(GETSTATICInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void Aarch64LoweringVisitor::visit(LOOKUPSWITCHInst *I, bool copyOperands) {
	Type::TypeID type = I->get_type();
	ABORT_MSG("aarch64: Lowering not supported",
		"Inst: " << I << " type: " << type);
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
