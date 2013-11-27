/* src/vm/jit/compiler2/X86_64Backend.hpp - X86_64Backend

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

#include "vm/jit/compiler2/x86_64/X86_64Backend.hpp"
#include "vm/jit/compiler2/x86_64/X86_64Instructions.hpp"
#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"
#include "vm/jit/compiler2/x86_64/X86_64MachineMethodDescriptor.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MethodDescriptor.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"
#include "vm/jit/compiler2/StackSlotManager.hpp"
#include "vm/jit/Patcher.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/code.hpp"
#include "vm/class.hpp"
#include "vm/field.hpp"

#include "md-trap.hpp"

#include "toolbox/OStream.hpp"
#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/x86_64"

// code.hpp fix
#undef RAX
#undef XMM0

namespace cacao {
namespace jit {
namespace compiler2 {

// BackendBase must be specialized in namespace compiler2!
using namespace x86_64;

template<>
const char* BackendBase<X86_64>::get_name() const {
	return "x86_64";
}

template<>
MachineInstruction* BackendBase<X86_64>::create_Move(MachineOperand *src,
		MachineOperand* dst) const {
	Type::TypeID type = dst->get_type();
	assert(type == src->get_type());
	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	case Type::ReferenceTypeID:
		return new MovInst(
			SrcOp(src),
			DstOp(dst),
			get_OperandSize_from_Type(type));
	case Type::DoubleTypeID:
		switch (src->get_OperandID()) {
		case MachineOperand::ImmediateID:
			return new MovImmSDInst(
				SrcOp(src),
				DstOp(dst));
		default:
			return new MovSDInst(
				SrcOp(src),
				DstOp(dst));
		}
	default: break;
	}
	ABORT_MSG("x86_64: Move not supported",
		"Inst: " << src << " -> " << dst << " type: " << type);
	return NULL;
}

template<>
MachineInstruction* BackendBase<X86_64>::create_Jump(MachineBasicBlock *target) const {
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
void BackendBase<X86_64>::create_frame(CodeMemory* CM, StackSlotManager *SSM) const {
	EnterInst enter(align_to<16>(SSM->get_frame_size()));
	enter.emit(CM);
	// fix alignment
	CodeFragment CF = CM->get_aligned_CodeFragment(0);
	emit_nop(CF,CF.size());
}

void LoweringVisitor::visit(LOADInst *I) {
	assert(I);
	//MachineInstruction *minst = loadParameter(I->get_index(), I->get_type());
	const MethodDescriptor &MD = I->get_Method()->get_MethodDescriptor();
	//FIXME inefficient
	const MachineMethodDescriptor MMD(MD);
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
			SrcOp(MMD[I->get_index()]),
			DstOp(dst),
			get_OperandSize_from_Type(type));
			break;
	case Type::DoubleTypeID:
		move = new MovSDInst(
			SrcOp(MMD[I->get_index()]),
			DstOp(dst));
			break;
	default:
		ABORT_MSG("x86_64 type not supported: ",
			  I << " type: " << type);
	}
	get_current()->push_back(move);
	set_op(I,move->get_result().op);
}

void LoweringVisitor::visit(IFInst *I) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	{
		// Integer types
		CmpInst *cmp = new CmpInst(
			Src2Op(src_op2),
			Src1Op(src_op1),
			get_OperandSize_from_Type(type));

		MachineInstruction *cjmp = NULL;
		BeginInstRef &then = I->get_then_target();
		BeginInstRef &els = I->get_else_target();

		switch (I->get_condition()) {
		case Conditional::EQ:
			cjmp = new CondJumpInst(Cond::E, get(then.get()),get(els.get()));
			break;
		case Conditional::LT:
			cjmp = new CondJumpInst(Cond::L, get(then.get()),get(els.get()));
			break;
		case Conditional::LE:
			cjmp = new CondJumpInst(Cond::LE, get(then.get()),get(els.get()));
			break;
		case Conditional::GE:
			cjmp = new CondJumpInst(Cond::GE, get(then.get()),get(els.get()));
			break;
		case Conditional::GT:
			cjmp = new CondJumpInst(Cond::G, get(then.get()),get(els.get()));
			break;
		case Conditional::NE:
			cjmp = new CondJumpInst(Cond::NE, get(then.get()),get(els.get()));
			break;
		default:
			ABORT_MSG("x86_64 Conditional not supported: ",
				  I << " cond: " << I->get_condition());
		}
		//MachineInstruction *jmp = new JumpInst(get(els.get()));
		get_current()->push_back(cmp);
		get_current()->push_back(cjmp);
		//get_current()->push_back(jmp);

		set_op(I,cjmp->get_result().op);
		return;
	}
	default: break;
	}
	ABORT_MSG("x86_64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void LoweringVisitor::visit(ADDInst *I) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(src_op1,dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new AddInst(
			Src2Op(src_op2),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	case Type::DoubleTypeID:
		alu = new AddSDInst(
			Src2Op(src_op2),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(mov);
	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
}

void LoweringVisitor::visit(ANDInst *I) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(src_op1, dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new AndInst(
			Src2Op(src_op2),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(mov);
	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
}

void LoweringVisitor::visit(SUBInst *I) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(src_op1, dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new SubInst(
			Src2Op(src_op2),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	case Type::DoubleTypeID:
		alu = new SubSDInst(
			Src2Op(src_op2),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(mov);
	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
}

void LoweringVisitor::visit(MULInst *I) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();

	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(src_op1, dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new IMulInst(
			Src2Op(src_op2),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	case Type::DoubleTypeID:
		alu = new MulSDInst(
			Src2Op(src_op2),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(mov);
	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
}

void LoweringVisitor::visit(DIVInst *I) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();

	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(src_op1, dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	#if 0
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		break;
	#endif
	case Type::DoubleTypeID:
		alu = new DivSDInst(
			Src2Op(src_op2),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(mov);
	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
}

void LoweringVisitor::visit(REMInst *I) {
	assert(I);
	//MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	//MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();

	#if 0
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(src_op1, dst);
	MachineInstruction *alu;

	switch (type) {
	#if 0
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		break;
	#endif
	case Type::DoubleTypeID:
		alu = new DivSDInst(
			Src2Op(src_op2),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(mov);
	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
	#endif
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
}

void LoweringVisitor::visit(ALOADInst *I) {
	assert(I);
	MachineOperand* src_ref = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_index = get_op(I->get_operand(1)->to_Instruction());
	assert(src_ref->get_type() == Type::ReferenceTypeID);
	assert(src_index->get_type() == Type::IntTypeID);

	Type::TypeID type = I->get_type();
	MachineOperand *vreg = new VirtualRegister(type);

	s4 offset;
	switch (type) {
	case Type::ByteTypeID:
		offset = OFFSET(java_bytearray_t, data[0]);
		break;
	case Type::ShortTypeID:
		offset = OFFSET(java_shortarray_t, data[0]);
		break;
	case Type::IntTypeID:
		offset = OFFSET(java_intarray_t, data[0]);
		break;
	case Type::LongTypeID:
		offset = OFFSET(java_longarray_t, data[0]);
		break;
	case Type::FloatTypeID:
		offset = OFFSET(java_floatarray_t, data[0]);
		break;
	case Type::DoubleTypeID:
		offset = OFFSET(java_doublearray_t, data[0]);
		break;
	case Type::ReferenceTypeID:
		offset = OFFSET(java_objectarray_t, data[0]);
		break;
	default:
		ABORT_MSG("x86_64 Lowering not supported",
			"Inst: " << I << " type: " << type);
		offset = 0;
	}
	// TODO array bounds check
	// create modrm source operand
	ModRMOperand modrm(type,IndexOp(src_index),BaseOp(src_ref),offset);

	MachineInstruction *move = new MovModRMInst(
		DstOp(vreg),
		get_OperandSize_from_Type(type),
		SrcModRM(modrm));
	get_current()->push_back(move);
	set_op(I,move->get_result().op);
}

void LoweringVisitor::visit(ASTOREInst *I) {
	assert(I);
	MachineOperand* src_ref = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_index = get_op(I->get_operand(1)->to_Instruction());
	MachineOperand* src_value = get_op(I->get_operand(2)->to_Instruction());
	assert(src_ref->get_type() == Type::ReferenceTypeID);
	assert(src_index->get_type() == Type::IntTypeID);

	Type::TypeID type = I->get_array_type();

	s4 offset;
	switch (type) {
	case Type::ByteTypeID:
		offset = OFFSET(java_bytearray_t, data[0]);
		break;
	case Type::ShortTypeID:
		offset = OFFSET(java_shortarray_t, data[0]);
		break;
	case Type::IntTypeID:
		offset = OFFSET(java_intarray_t, data[0]);
		break;
	case Type::LongTypeID:
		offset = OFFSET(java_longarray_t, data[0]);
		break;
	case Type::FloatTypeID:
		offset = OFFSET(java_floatarray_t, data[0]);
		break;
	case Type::DoubleTypeID:
		offset = OFFSET(java_doublearray_t, data[0]);
		break;
	default:
		ABORT_MSG("x86_64 Lowering not supported",
			"Inst: " << I << " type: " << type);
		offset = 0;
	}
	// TODO array bounds check
	// create modrm source operand
	ModRMOperand modrm(type,IndexOp(src_index),BaseOp(src_ref),offset);

	MachineInstruction *move = new MovModRMInst(
		SrcOp(src_value),
		get_OperandSize_from_Type(type),
		DstModRM(modrm));
	get_current()->push_back(move);
	set_op(I,move->get_result().op);
}

void LoweringVisitor::visit(ARRAYLENGTHInst *I) {
	assert(I);
	MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
	assert(I->get_type() == Type::IntTypeID);
	assert(src_op->get_type() == Type::ReferenceTypeID);
	MachineOperand *vreg = new VirtualRegister(Type::IntTypeID);
	// create modrm source operand
	ModRMOperand modrm(BaseOp(src_op),OFFSET(java_array_t,size));

	MachineInstruction *move = new MovModRMInst(
		DstOp(vreg),
		get_OperandSize_from_Type(Type::IntTypeID),
		SrcModRM(modrm));
	get_current()->push_back(move);
	set_op(I,move->get_result().op);
}

void LoweringVisitor::visit(ARRAYBOUNDSCHECKInst *I) {
	assert(I);
	MachineOperand* src_ref = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_index = get_op(I->get_operand(1)->to_Instruction());
	assert(src_ref->get_type() == Type::ReferenceTypeID);
	assert(src_index->get_type() == Type::IntTypeID);

	// load array length
	MachineOperand *len = new VirtualRegister(Type::IntTypeID);
	ModRMOperand modrm(BaseOp(src_ref),OFFSET(java_array_t,size));
	MachineInstruction *move = new MovModRMInst(
		DstOp(len),
		get_OperandSize_from_Type(Type::IntTypeID),
		SrcModRM(modrm));
	get_current()->push_back(move);
	// compare with index
	CmpInst *cmp = new CmpInst(
		Src2Op(len),
		Src1Op(src_index),
		get_OperandSize_from_Type(Type::IntTypeID));
	get_current()->push_back(cmp);
	// throw exception
	MachineInstruction *trap = new CondTrapInst(Cond::B,TRAP_ArrayIndexOutOfBoundsException, SrcOp(src_index));
	get_current()->push_back(trap);
}

void LoweringVisitor::visit(RETURNInst *I) {
	assert(I);
	Type::TypeID type = I->get_type();
	MachineOperand* src_op = (type == Type::VoidTypeID ? 0 : get_op(I->get_operand(0)->to_Instruction()));
	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	{
		MachineOperand *ret_reg = new NativeRegister(type,&RAX);
		MachineInstruction *reg = new MovInst(
			SrcOp(src_op),
			DstOp(ret_reg),
			get_OperandSize_from_Type(type));
		LeaveInst *leave = new LeaveInst();
		RetInst *ret = new RetInst(get_OperandSize_from_Type(type),SrcOp(ret_reg));
		get_current()->push_back(reg);
		get_current()->push_back(leave);
		get_current()->push_back(ret);
		set_op(I,ret->get_result().op);
		return;
	}
	case Type::DoubleTypeID:
	{
		MachineOperand *ret_reg = new NativeRegister(type,&XMM0);
		MachineInstruction *reg = new MovSDInst(
			SrcOp(src_op),
			DstOp(ret_reg));
		LeaveInst *leave = new LeaveInst();
		RetInst *ret = new RetInst(get_OperandSize_from_Type(type),SrcOp(ret_reg));
		get_current()->push_back(reg);
		get_current()->push_back(leave);
		get_current()->push_back(ret);
		set_op(I,ret->get_result().op);
		return;
	}
	case Type::VoidTypeID:
	{
		LeaveInst *leave = new LeaveInst();
		RetInst *ret = new RetInst();
		get_current()->push_back(leave);
		get_current()->push_back(ret);
		set_op(I,ret->get_result().op);
		return;
	}
	default: break;
	}
	ABORT_MSG("x86_64 Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void LoweringVisitor::visit(CASTInst *I) {
	assert(I);
	MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
	Type::TypeID from = I->get_operand(0)->to_Instruction()->get_type();
	Type::TypeID to = I->get_type();

	switch (from) {
	case Type::IntTypeID:
		switch (to) {
		case Type::LongTypeID:
		{
			MachineInstruction *mov = new MovSXInst(
				SrcOp(src_op),
				DstOp(new VirtualRegister(to)),
				GPInstruction::OS_32, GPInstruction::OS_64);
			get_current()->push_back(mov);
			set_op(I,mov->get_result().op);
			return;
		}
		default: break;
		}
		break;
	case Type::LongTypeID:
		switch (to) {
		case Type::DoubleTypeID:
		{
			MachineInstruction *mov = new CVTSI2SDInst(
				SrcOp(src_op),
				DstOp(new VirtualRegister(to)),
				GPInstruction::OS_64, GPInstruction::OS_64);
			get_current()->push_back(mov);
			set_op(I,mov->get_result().op);
			return;
		}
		default: break;
		}
		break;
	default: break;
}
ABORT_MSG("x86_64 Cast not supported!", "From " << from << " to " << to );
}

void LoweringVisitor::visit(INVOKESTATICInst *I) {
	assert(I);
	Type::TypeID type = I->get_type();
	MethodDescriptor &MD = I->get_MethodDescriptor();
	MachineMethodDescriptor MMD(MD);

	// operands for the call
	VirtualRegister *addr = new VirtualRegister(Type::ReferenceTypeID);
	MachineOperand *result = &NoOperand;

	// get return value
	switch (type) {
	case Type::IntTypeID:
	case Type::LongTypeID:
		result = new NativeRegister(type,&RAX);
		break;
	default:
		ABORT_MSG("x86_64 Lowering not supported",
		"Inst: " << I << " type: " << type);
	}

	// create call
	MachineInstruction* call = new CallInst(SrcOp(addr),DstOp(result),I->op_size());
	// move values to parameters
	for (std::size_t i = 0; i < I->op_size(); ++i ) {
		MachineInstruction* mov = get_Backend()->create_Move(
			new UnassignedReg(MD[i]),
			MMD[i]
		);
		get_current()->push_back(mov);
		// set call operand
		call->set_operand(i+1,MMD[i]);
	}
	// spill caller saved

	// load address
	DataSegment &DS = get_Backend()->get_JITData()->get_CodeMemory()->get_DataSegment();
	DataSegment::IdxTy idx = DS.get_index(DSFMIRef(I->get_fmiref()));
	if (DataSegment::is_invalid(idx)) {
		DataFragment data = DS.get_Ref(sizeof(void*));
		idx = DS.insert_tag(DSFMIRef(I->get_fmiref()),data);
	}
	if (I->is_resolved()) {
		LOG2("INVOKESTATICInst: is resolved" << nl);
		// write stubroutine
		//methodinfo*         lm;             // Local methodinfo for ICMD_INVOKE*.
		//lm = I->get_fmiref()->p.method;
		//lm->stubroutine;
	} else {
		LOG2("INVOKESTATICInst: is notresolved" << nl);
	}
	MovDSEGInst *dmov = new MovDSEGInst(DstOp(addr),idx);
	get_current()->push_back(dmov);

	// add call
	get_current()->push_back(call);
	// get result
	if (result != &NoOperand) {
		MachineInstruction *reg = new MovInst(
			SrcOp(result),
			DstOp(new VirtualRegister(type)),
			get_OperandSize_from_Type(type));
		get_current()->push_back(reg);
		set_op(I,reg->get_result().op);
	}
	#if 0
	if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
		unresolved_method*  um;
		um = iptr->sx.s23.s3.um;
		disp = dseg_add_unique_address(cd, um);

		patcher_add_patch_ref(jd, PATCHER_invokestatic_special,
							  um, disp);
	}
	else {
		methodinfo*         lm;             // Local methodinfo for ICMD_INVOKE*.
		lm = iptr->sx.s23.s3.fmiref->p.method;
		disp = dseg_add_functionptr(cd, lm->stubroutine);
	}
	#endif
}

void LoweringVisitor::visit(GETSTATICInst *I) {
	assert(I);
	DataSegment &DS = get_Backend()->get_JITData()->get_CodeMemory()->get_DataSegment();
	DataSegment::IdxTy idx = DS.get_index(DSFMIRef(I->get_fmiref()));
	if (DataSegment::is_invalid(idx)) {
		DataFragment data = DS.get_Ref(sizeof(void*));
		idx = DS.insert_tag(DSFMIRef(I->get_fmiref()),data);
	}

	if (I->is_resolved()) {
		fieldinfo* fi = I->get_fmiref()->p.field;

		if (!class_is_or_almost_initialized(fi->clazz)) {
			//PROFILE_CYCLE_STOP;
			Patcher *patcher = new InitializeClassPatcher(fi->clazz);
			PatcherPtrTy ptr(patcher);
			get_Backend()->get_JITData()->get_jitdata()->code->patchers->push_back(ptr);
			MachineInstruction *pi = new PatchInst(patcher);
			get_current()->push_back(pi);
			//PROFILE_CYCLE_START;
		}

	} else {
		assert(0 && "Not yet implemented");
		#if 0
		unresolved_field* uf = iptr->sx.s23.s3.uf;
		fieldtype = uf->fieldref->parseddesc.fd->type;
		disp      = dseg_add_unique_address(cd, 0);

		pr = patcher_add_patch_ref(jd, PATCHER_get_putstatic, uf, disp);

		fi = NULL;		/* Silence compiler warning */
		#endif
	}
	VirtualRegister *addr = new VirtualRegister(Type::ReferenceTypeID);
	MovDSEGInst *dmov = new MovDSEGInst(DstOp(addr),idx);
	MachineInstruction* mov = get_Backend()->create_Move(addr,new VirtualRegister(I->get_type()));
	get_current()->push_back(dmov);
	get_current()->push_back(mov);
	set_op(I,mov->get_result().op);
}

void LoweringVisitor::visit(LOOKUPSWITCHInst *I) {
	assert_msg(0 , "Fix CondJump");
	assert(I);
	MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
	Type::TypeID type = I->get_type();

	LOOKUPSWITCHInst::succ_const_iterator s = I->succ_begin();
	for(LOOKUPSWITCHInst::match_iterator i = I->match_begin(),
			e = I->match_end(); i != e; ++i) {
		CmpInst *cmp = new CmpInst(
			Src2Op(new Immediate(*i,Type::IntType())),
			Src1Op(src_op),
			get_OperandSize_from_Type(type));
		MachineInstruction *cjmp = new CondJumpInst(Cond::E, get(s->get()),get((++s)->get()));
		get_current()->push_back(cmp);
		get_current()->push_back(cjmp);
		++s;
	}

	// default
	MachineInstruction *jmp = new JumpInst(get(s->get()));
	get_current()->push_back(jmp);
	assert(++s == I->succ_end());

	set_op(I,jmp->get_result().op);
}

void LoweringVisitor::visit(TABLESWITCHInst *I) {
	assert_msg(0 , "Fix CondJump");
	assert(I);
	MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister *src = new VirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(src_op, src);
	get_current()->push_back(mov);

	s4 low = I->get_low();
	s4 high = I->get_high();

	// adjust offset
	if (low != 0) {
		SubInst *sub = new SubInst(
			Src2Op(new Immediate(low,Type::IntType())),
			DstSrc1Op(src),
			get_OperandSize_from_Type(type)
		);
		get_current()->push_back(sub);
		high -= low;
	}
	// check range
	CmpInst *cmp = new CmpInst(
		Src2Op(new Immediate(high,Type::IntType())),
		Src1Op(src),
		get_OperandSize_from_Type(type));
	MachineInstruction *cjmp = new CondJumpInst(Cond::G, get(I->succ_front().get()),get((++I->succ_begin())->get()));
	get_current()->push_back(cmp);
	get_current()->push_back(cjmp);

	// TODO load data segment and jump
	// load address
	DataSegment &DS = get_Backend()->get_JITData()->get_CodeMemory()->get_DataSegment();
	DataFragment data = DS.get_Ref(sizeof(void*) * (high - low + 1));
	DataSegment::IdxTy idx = data.get_begin();
	VirtualRegister *addr = new VirtualRegister(Type::ReferenceTypeID);
	WARNING_MSG("TODO","add offset");
	MovDSEGInst *dmov = new MovDSEGInst(DstOp(addr),idx);
	get_current()->push_back(dmov);
	IndirectJumpInst *jmp = new IndirectJumpInst(SrcOp(addr));
	// adding targets
	for(TABLESWITCHInst::succ_const_iterator i = ++I->succ_begin(),
			e = I->succ_end(); i != e; ++i) {
		jmp->add_target(get(i->get()));
	}
	get_current()->push_back(jmp);
	// assert(0 && "load data segment and jump"));
	// load table entry
	set_op(I,cjmp->get_result().op);
}

#if 0
template<>
compiler2::RegisterFile*
BackendBase<X86_64>::get_RegisterFile(Type::TypeID type) const {
	return new x86_64::RegisterFile(type);
}
#endif


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
