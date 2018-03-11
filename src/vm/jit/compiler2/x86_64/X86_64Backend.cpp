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
#include "vm/jit/compiler2/x86_64/X86_64ModRMOperand.hpp"
#include "vm/jit/compiler2/x86_64/X86_64MachineMethodDescriptor.hpp"
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

#include "mm/memory.hpp"

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
	assert_msg(type == src->get_type(), "Move needs the same type but its " << src->get_type() << " -> " << type);
	assert(!(src->is_stackslot() && dst->is_stackslot()));
	switch (type) {
	case Type::CharTypeID:
	case Type::ByteTypeID:
	case Type::ShortTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	case Type::ReferenceTypeID:
	{
		return new MovInst(
			SrcOp(src),
			DstOp(dst),
			get_OperandSize_from_Type(type));
	}
	case Type::DoubleTypeID:
	{
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
		break;
	}
	case Type::FloatTypeID:
	{
		switch (src->get_OperandID()) {
		case MachineOperand::ImmediateID:
			return new MovImmSSInst(
				SrcOp(src),
				DstOp(dst));
		default:
			return new MovSSInst(
				SrcOp(src),
				DstOp(dst));
		}
		break;
	}
	default:
		break;
	}
	ABORT_MSG("x86_64: Move not supported",
		"Inst: " << src << " -> " << dst << " type: " << type);
	return NULL;
}

template<>
MachineInstruction* BackendBase<X86_64>::create_Jump(MachineBasicBlock *target) const {
	return new JumpInst(target);
}

template<>
MachineInstruction* BackendBase<X86_64>::create_Swap(MachineOperand *op1, MachineOperand *op2) const {
	assert(op1->is_Register() && op2->is_Register() && "Swap only implemented for register operands!");
	assert(op1->get_type() == op2->get_type() && "Operand types of a swap have to match!");
	return new SwapInst(Src1Op(op1), Src2Op(op2), get_OperandSize_from_Type(op1->get_type()));
}

namespace {

template <unsigned size, class T>
inline T align_to(T val) {
	T rem =(val % size);
	return val + ( rem == 0 ? 0 : size - rem);
}

template <class I,class Seg>
static void write_data(Seg seg, I data) {
	assert(seg.size() == sizeof(I));

	for (int i = 0, e = sizeof(I) ; i < e ; ++i) {
		seg[i] = (u1) 0xff & *(reinterpret_cast<u1*>(&data) + i);
	}

}

} // end anonymous namespace

template<>
void BackendBase<X86_64>::create_prolog(MachineBasicBlock* entry, const CalleeSavedRegisters& callee_saved) const {
	// Save callee saved registers
	std::vector<MachineInstruction*> moves;
	for (const auto& pair : callee_saved) {
		auto move = create_Move(pair.first, pair.second);
		moves.push_back(move);
	}
	entry->insert_after(entry->begin(), moves.begin(), moves.end());

	// Create stackframe
	auto SSM = get_JITData()->get_StackSlotManager();
	auto enter = new EnterInst(align_to<16>(SSM->get_frame_size()));

	entry->insert_after(entry->begin(), enter);
}

template<>
void BackendBase<X86_64>::create_epilog(MachineBasicBlock* exit, const CalleeSavedRegisters& callee_saved) const {
	// Restore callee saved registers, since a exit block has 2 instructions (leaveq, ret), we need to
	// backoff for a bit
	std::vector<MachineInstruction*> moves;
	for (const auto& pair : callee_saved) {
		auto move = create_Move(pair.second, pair.first);
		moves.push_back(move);
	}

	auto iter = --exit->end();
	exit->insert_before(iter, moves.begin(), moves.end());
}

template<>
void BackendBase<X86_64>::create_frame(CodeMemory* CM, StackSlotManager *SSM, const OperandSet& callee_saved) const {
	EnterInst enter(align_to<16>(SSM->get_frame_size()));
	enter.emit(CM);
	// fix alignment
	CodeFragment CF = CM->get_aligned_CodeFragment(0);
	emit_nop(CF,CF.size());
}

void X86_64LoweringVisitor::visit(LOADInst *I, bool copyOperands) {
	assert(I);
	//MachineInstruction *minst = loadParameter(I->get_index(), I->get_type());
	const MethodDescriptor &MD = I->get_Method()->get_MethodDescriptor();
	//FIXME inefficient
	const MachineMethodDescriptor MMD(MD, get_Backend()->get_JITData()->get_MachineOperandFactory());
	Type::TypeID type = I->get_type();

	MachineOperand *parameter = MMD[I->get_index()];
	MachineOperand *src = parameter;
	if (parameter->is_Register())
		src = CreateVirtualRegister(type);
	
	MachineOperand *dst = CreateVirtualRegister(type);

	// On the first parameter we create the param instruction that defines ALL parameters
	if (parameter->is_Register()) {
		if (param_instruction == nullptr) {
			param_instruction = new ParamInst(src, parameter);
			get_current()->push_back(param_instruction);
		} else {
			assert(param_instruction);
			param_instruction->add_result(src, parameter);
		}
	}

	// Prematurley split the live-range, their are some cases where repairing
	// is not able to do it for us
	get_current()->push_back(get_Backend()->create_Move(src, dst));
	set_op(I, dst);
}

void X86_64LoweringVisitor::visit(CMPInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_operand(0)->get_type();
	assert(type == I->get_operand(1)->get_type());
	switch (type) {
	case Type::FloatTypeID:
	case Type::DoubleTypeID:
	{

		MachineBasicBlock *MBB = get_current();
		GPInstruction::OperandSize op_size = get_OperandSize_from_Type(type);
		MachineOperand *dst = CreateVirtualRegister(Type::IntTypeID);
		MachineOperand *zero = CreateVirtualRegister(Type::IntTypeID);
		MachineOperand *less = CreateVirtualRegister(Type::IntTypeID);
		MachineOperand *greater = CreateVirtualRegister(Type::IntTypeID);
		// unordered 0
		MBB->push_back(new MovInst(
			SrcOp(new Immediate(0,Type::IntType())),
			DstOp(zero),
			op_size
		));
		// less then (1)
		MBB->push_back(new MovInst(
			SrcOp(new Immediate(1,Type::IntType())),
			DstOp(less),
			op_size
		));
		// greater then (-1)
		MBB->push_back(new MovInst(
			SrcOp(new Immediate(-1,Type::IntType())),
			DstOp(greater),
			op_size
		));
		// compare
		switch (type) {
		case Type::FloatTypeID:
			MBB->push_back(new UCOMISSInst(Src2Op(src_op1), Src1Op(src_op2)));
			break;
		case Type::DoubleTypeID:
			MBB->push_back(new UCOMISDInst(Src2Op(src_op1), Src1Op(src_op2)));
			break;
		default: assert(0);
			break;
		}
		// cmov less
		VirtualRegister *cless = CreateVirtualRegister(Type::IntTypeID);
		MBB->push_back(new CMovInst(
			Cond::B,
			Src1Op(zero),
			Src2Op(less),
			DstOp(cless),
			op_size
		));
		// cmov greater
		VirtualRegister *cgreater = CreateVirtualRegister(Type::IntTypeID);
		MBB->push_back(new CMovInst(
			Cond::A,
			Src1Op(cless),
			Src2Op(greater),
			DstOp(cgreater),
			op_size
		));
		switch (I->get_FloatHandling()) {
		case CMPInst::L:
			// treat unordered as GT
			MBB->push_back(new CMovInst(
				Cond::P,
				Src1Op(cgreater),
				Src2Op(greater),
				DstOp(dst),
				op_size
			));
			break;

		case CMPInst::G:
			// treat unordered as LT
			MBB->push_back(new CMovInst(
				Cond::P,
				Src1Op(cgreater),
				Src2Op(less),
				DstOp(dst),
				op_size
			));
			break;
		default: assert(0);
			break;
		}
		set_op(I,dst);
		return;
	}
	default: break;
	}
	ABORT_MSG("x86_64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void X86_64LoweringVisitor::visit(IFInst *I, bool copyOperands) {
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
		get_current()->set_last_insertion_point(--get_current()->mi_last());
		//get_current()->push_back(jmp);

		set_op(I,cjmp->get_result().op);
		return;
	}
	default: break;
	}
	ABORT_MSG("x86_64: Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void X86_64LoweringVisitor::visit(NEGInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src = get_op(I->get_operand(0)->to_Instruction());
	Type::TypeID type = I->get_type();
	MachineBasicBlock *MBB = get_current();

	VirtualRegister *src_copy = CreateVirtualRegister(src->get_type());
	VirtualRegister *dst = CreateVirtualRegister(type);

	switch (type) {
	case Type::FloatTypeID:
	{
		VirtualRegister *tmp = CreateVirtualRegister(type);
		MBB->push_back(new MovImmSSInst(
			SrcOp(new Immediate(0x80000000,Type::IntType())),
			DstOp(tmp)
		));
		MBB->push_back(new XORPSInst(
			Src1Op(src),
			Src2Op(tmp),
			DstOp(dst)
		));
		break;
	}
	case Type::DoubleTypeID:
	{
		VirtualRegister *tmp = CreateVirtualRegister(type);
		MBB->push_back(new MovImmSDInst(
			SrcOp(new Immediate(0x8000000000000000L,Type::LongType())),
			DstOp(tmp)
		));
		MBB->push_back(new XORPDInst(
			Src1Op(src),
			Src2Op(tmp),
			DstOp(dst)
		));
		break;
	}
	case Type::IntTypeID:
	case Type::LongTypeID:
		MBB->push_back(get_Backend()->create_Move(src,src_copy));
		MBB->push_back(new NegInst(SrcOp(src_copy),DstOp(dst),get_operand_size_from_Type(type)));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	set_op(I,dst);
}

void X86_64LoweringVisitor::visit(ADDInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister *dst = CreateVirtualRegister(type);

	// setupSrcDst(src_op1, src_op2, dst, type, copyOperands, I->is_commutable());

	// Split the live-range of the first argument by making a copy
	MachineOperand* src_op1_copy = CreateVirtualRegister(src_op1->get_type());
	auto move = get_Backend()->create_Move(src_op1, src_op1_copy);
	get_current()->push_back(move);

	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new AddInst(
			Src1Op(src_op1_copy),
			Src2Op(src_op2),
			DstOp(dst),
			get_OperandSize_from_Type(type));
		break;
	case Type::DoubleTypeID:
		alu = new AddSDInst(
			Src1Op(src_op1_copy),
			Src2Op(src_op2),
			DstOp(dst));
		break;
	case Type::FloatTypeID:
		alu = new AddSSInst(
			Src1Op(src_op1_copy),
			Src2Op(src_op2),
			DstOp(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
}

void X86_64LoweringVisitor::visit(ANDInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister *dst = CreateVirtualRegister(type);

	// setupSrcDst(src_op1, src_op2, dst, type, copyOperands, I->is_commutable());

	// Split the live-range of the first argument by making a copy
	MachineOperand* src_op1_copy = CreateVirtualRegister(src_op1->get_type());
	auto move = get_Backend()->create_Move(src_op1, src_op1_copy);
	get_current()->push_back(move);

	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new AndInst(
			Src1Op(src_op1_copy),
			Src2Op(src_op2),
			DstOp(dst),
			get_OperandSize_from_Type(type));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
}

void X86_64LoweringVisitor::visit(ORInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister *dst = NULL;

	setupSrcDst(src_op1, src_op2, dst, type, copyOperands, I->is_commutable());

	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new OrInst(
			Src2Op(src_op2),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
}

void X86_64LoweringVisitor::visit(XORInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister *dst = NULL;

	setupSrcDst(src_op1, src_op2, dst, type, copyOperands, I->is_commutable());

	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new XorInst(
			Src2Op(src_op2),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
}

void X86_64LoweringVisitor::visit(SUBInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister *dst = CreateVirtualRegister(type);

	// setupSrcDst(src_op1, src_op2, dst, type, copyOperands, I->is_commutable());
	
	// Split the live-range of the first argument by making a copy
	MachineOperand* src_op1_copy = CreateVirtualRegister(src_op1->get_type());
	auto move = get_Backend()->create_Move(src_op1, src_op1_copy);
	get_current()->push_back(move);

	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new SubInst(
			DstOp(dst),
			Src1Op(src_op1_copy),
			Src2Op(src_op2),
			get_OperandSize_from_Type(type));
		break;
	case Type::DoubleTypeID:
		alu = new SubSDInst(
			Src1Op(src_op1_copy),
			Src2Op(src_op2),
			DstOp(dst));
		break;
	case Type::FloatTypeID:
		alu = new SubSSInst(
			Src1Op(src_op1_copy),
			Src2Op(src_op2),
			DstOp(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
}

void X86_64LoweringVisitor::visit(MULInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister *dst = CreateVirtualRegister(type);

	// setupSrcDst(src_op1, src_op2, dst, type, copyOperands, I->is_commutable());

	// Split the live-range of the first argument by making a copy
	MachineOperand* src_op1_copy = CreateVirtualRegister(src_op1->get_type());
	auto move = get_Backend()->create_Move(src_op1, src_op1_copy);
	get_current()->push_back(move);

	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new IMulInst(
			DstOp(dst),
			Src1Op(src_op1_copy),
			Src2Op(src_op2),
			get_OperandSize_from_Type(type));
		break;
	case Type::DoubleTypeID:
		alu = new MulSDInst(
			Src1Op(src_op1_copy),
			Src2Op(src_op2),
			DstOp(dst));
		break;
	case Type::FloatTypeID:
		alu = new MulSSInst(
			Src1Op(src_op1_copy),
			Src2Op(src_op2),
			DstOp(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
}

void X86_64LoweringVisitor::visit(DIVInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op1 = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	GPInstruction::OperandSize opsize = get_OperandSize_from_Type(type);

	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::IntTypeID:
	case Type::LongTypeID:
	{
		// 1. move the dividend to RAX
		// 2. extend the dividend to RDX:RAX
		// 3. perform the division

		// Split live-ranges of src_op1 for easier register assignment
		MachineOperand *src_op1_copy = CreateVirtualRegister(src_op1->get_type());
		get_current()->push_back(get_Backend()->create_Move(src_op1, src_op1_copy));

		MachineOperand *dividendUpper = CreateVirtualRegister(type);
		MachineOperand *result = CreateVirtualRegister(type);
		MachineInstruction *convertToQuadword = new CDQInst(SrcOp(src_op1_copy), DstOp(dividendUpper), DstOp(result), opsize);
		get_current()->push_back(convertToQuadword);

		MachineOperand *quotient = CreateVirtualRegister(type);
		MachineOperand *remainder = CreateVirtualRegister(type);
		alu = new IDivInst(Src1Op(src_op2), Src2Op(dividendUpper), Src2Op(result), DstOp(quotient), DstOp(remainder), opsize);
		break;
	}
	case Type::DoubleTypeID:
	{
		VirtualRegister *dst = CreateVirtualRegister(type);
		VirtualRegister *src_op1_copy = CreateVirtualRegister(src_op1->get_type());

		MachineInstruction *mov = get_Backend()->create_Move(src_op1, src_op1_copy);
		get_current()->push_back(mov);
		alu = new DivSDInst(
			Src1Op(src_op1_copy),
			Src2Op(src_op2),
			DstOp(dst));
		break;
	}
	case Type::FloatTypeID:
	{
		VirtualRegister *dst = CreateVirtualRegister(type);
		VirtualRegister *src_op1_copy = CreateVirtualRegister(src_op1->get_type());

		MachineInstruction *mov = get_Backend()->create_Move(src_op1, src_op1_copy);
		get_current()->push_back(mov);
		alu = new DivSSInst(
			Src1Op(src_op1_copy),
			Src2Op(src_op2),
			DstOp(dst));
		break;
	}
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}

	get_current()->push_back(alu);
	set_op(I,alu->get_result().op);
}

void X86_64LoweringVisitor::visit(REMInst *I, bool copyOperands) {
	assert(I);

	MachineOperand* dividendLower;
	MachineOperand* src_op2 = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = I->get_type();
	GPInstruction::OperandSize opsize = get_OperandSize_from_Type(type);
	MachineOperand *dividend = get_op(I->get_operand(0)->to_Instruction());;

	MachineInstruction *resultInst = nullptr;
	MachineOperand *resultOperand = nullptr;

	StackSlotManager *ssm;
	ManagedStackSlot *src;
	ManagedStackSlot *dst;
	ManagedStackSlot *resultSlot;

	switch (type) {
	case Type::IntTypeID:
	case Type::LongTypeID:
	{
		// 1. move the dividend to RAX
		// 2. extend the dividend to RDX:RAX
		// 3. perform the division

		// Split live-ranges of src_op1 for easier register assignment
		MachineOperand *src_op1_copy = CreateVirtualRegister(dividend->get_type());
		get_current()->push_back(get_Backend()->create_Move(dividend, src_op1_copy));

		MachineOperand *dividendUpper = CreateVirtualRegister(type);
		MachineOperand *result = CreateVirtualRegister(type);
		MachineInstruction *convertToQuadword = new CDQInst(SrcOp(src_op1_copy), DstOp(dividendUpper), DstOp(result), opsize);
		get_current()->push_back(convertToQuadword);

		MachineOperand *quotient = CreateVirtualRegister(type);
		MachineOperand *remainder = CreateVirtualRegister(type);
		resultInst = new IDivInst(Src1Op(src_op2), Src2Op(dividendUpper), Src2Op(result), DstOp(quotient), DstOp(remainder), opsize);
		get_current()->push_back(resultInst);
		resultOperand = remainder;
		break;
	}
	case Type::FloatTypeID:
	case Type::DoubleTypeID:
		ssm = get_Backend()->get_JITData()->get_StackSlotManager();
		src = ssm->create_slot(type);
		dst = ssm->create_slot(type);
		resultSlot = ssm->create_slot(type);

		// operands of the FP stack can only be loaded from memory
		get_current()->push_back(get_Backend()->create_Move(dividend, src));
		get_current()->push_back(get_Backend()->create_Move(src_op2, dst));

		// initialize the FP stack
		get_current()->push_back(new FLDInst(SrcMemOp(dst), opsize));
		get_current()->push_back(new FLDInst(SrcMemOp(src), opsize));

		get_current()->push_back(new FPRemInst(opsize));
		resultInst = new FSTPInst(DstMemOp(resultSlot), opsize);
		get_current()->push_back(resultInst);

		// clean the FP stack
		get_current()->push_back(new FFREEInst(&ST0));
		get_current()->push_back(new FINCSTPInst());
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}

	resultOperand = resultOperand ? resultOperand : resultInst->get_result().op;
	set_op(I, resultOperand);
}

void X86_64LoweringVisitor::visit(AREFInst *I, bool copyOperands) {
	// Only emit Instructions if Pattern Matching is used. If not ASTOREInst/ALOADInst will handle everything
#if 0
//#ifdef PATTERN_MATCHING // won't currently work because if base or index are modified, AREF should be modified instead/aswell.
	assert(I);
	MachineOperand* src_ref = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_index = get_op(I->get_operand(1)->to_Instruction());
	assert(src_ref->get_type() == Type::ReferenceTypeID);
	assert(src_index->get_type() == Type::IntTypeID);

	Type::TypeID type = I->get_type();

	VirtualRegister *dst = CreateVirtualRegister(Type::ReferenceTypeID);

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

	// create modrm source operand
	MachineOperand *modrm = new X86_64ModRMOperand(BaseOp(src_ref),IndexOp(src_index),type,offset);
	MachineInstruction* lea = new LEAInst(DstOp(dst), get_OperandSize_from_Type(Type::ReferenceTypeID), SrcOp(modrm));
	get_current()->push_back(lea);
	set_op(I,lea->get_result().op);
#endif
}

void X86_64LoweringVisitor::visit(ALOADInst *I, bool copyOperands) {
	assert(I);
	Type::TypeID type = I->get_type();
	Instruction* ref_inst = I->get_operand(0)->to_Instruction();
	MachineOperand *vreg = CreateVirtualRegister(type);
	MachineOperand *modrm = NULL;

	// if Pattern Matching is used, src op is Register with Effective Address , otherwise src op is AREFInst 
#if 0
//#ifdef PATTERN_MATCHING // won't currently work because if base or index are modified, AREF should be modified instead/aswell.
	MachineOperand* src_ref = get_op(ref_inst);
	assert(src_ref->get_type() == Type::ReferenceTypeID);

	modrm = new X86_64ModRMOperand(BaseOp(src_ref));
#else
	MachineOperand* src_base = get_op(ref_inst->get_operand(0)->to_Instruction());
	assert(src_base->get_type() == Type::ReferenceTypeID);
	MachineOperand* src_index = get_op(ref_inst->get_operand(1)->to_Instruction());
	assert(src_index->get_type() == Type::IntTypeID 
			   || src_index->get_type() == Type::ShortTypeID
			   || src_index->get_type() == Type::CharTypeID
			   || src_index->get_type() == Type::ByteTypeID);

	s4 offset;
	switch (type) {
	case Type::ByteTypeID:
		offset = OFFSET(java_bytearray_t, data[0]);
		break;
	case Type::ShortTypeID:
		offset = OFFSET(java_shortarray_t, data[0]);
		break;
	case Type::CharTypeID:
		offset = OFFSET(java_chararray_t, data[0]);
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

	modrm = new X86_64ModRMOperand(type,BaseOp(src_base),IndexOp(src_index),type,offset);
#endif
	MachineInstruction *move = get_Backend()->create_Move(modrm, vreg);
	get_current()->push_back(move);
	set_op(I,move->get_result().op);
}

void X86_64LoweringVisitor::visit(ASTOREInst *I, bool copyOperands) {
	assert(I);
	Instruction* ref_inst = I->get_operand(0)->to_Instruction();
	MachineOperand* src_value = get_op(I->get_operand(1)->to_Instruction());
	Type::TypeID type = src_value->get_type();
	MachineOperand *modrm = NULL;

	// if Pattern Matching is used, src op is Register with Effective Address , otherwise src op is AREFInst 
#if 0
//#ifdef PATTERN_MATCHING // won't currently work because if base or index are modified, AREF should be modified instead/aswell.
	MachineOperand* src_ref = get_op(ref_inst);
	assert(src_ref->get_type() == Type::ReferenceTypeID);

	modrm = new X86_64ModRMOperand(BaseOp(src_ref));
#else
	MachineOperand* src_base = get_op(ref_inst->get_operand(0)->to_Instruction());
	assert(src_base->get_type() == Type::ReferenceTypeID);
	MachineOperand* src_index = get_op(ref_inst->get_operand(1)->to_Instruction());
	assert(src_index->get_type() == Type::IntTypeID 
			   || src_index->get_type() == Type::ShortTypeID
			   || src_index->get_type() == Type::CharTypeID
			   || src_index->get_type() == Type::ByteTypeID);

	s4 offset;
	switch (type) {
	case Type::ByteTypeID:
		offset = OFFSET(java_bytearray_t, data[0]);
		break;
	case Type::ShortTypeID:
		offset = OFFSET(java_shortarray_t, data[0]);
		break;
	case Type::CharTypeID:
		offset = OFFSET(java_chararray_t, data[0]);
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

	modrm = new X86_64ModRMOperand(type,BaseOp(src_base),IndexOp(src_index),type,offset);
#endif
	MachineInstruction *move = get_Backend()->create_Move(src_value, modrm);
	get_current()->push_back(move);
	set_op(I,move->get_result().op);
}

void X86_64LoweringVisitor::visit(ARRAYLENGTHInst *I, bool copyOperands) {
	assert(I);

	// Implicit null-checks are handled via deoptimization.
	place_deoptimization_marker(I);

	MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
	assert(I->get_type() == Type::IntTypeID);
	assert(src_op->get_type() == Type::ReferenceTypeID);
	MachineOperand *vreg = CreateVirtualRegister(Type::IntTypeID);
	// create modrm source operand
	MachineOperand *modrm = new X86_64ModRMOperand(Type::IntTypeID,BaseOp(src_op),OFFSET(java_array_t,size));
	MachineInstruction *move = new MovInst(SrcOp(modrm)
					      ,DstOp(vreg)
					      ,get_OperandSize_from_Type(Type::IntTypeID)
					      );
	get_current()->push_back(move);
	set_op(I,move->get_result().op);
}

void X86_64LoweringVisitor::visit(ARRAYBOUNDSCHECKInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_ref = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand* src_index = get_op(I->get_operand(1)->to_Instruction());
	assert(src_ref->get_type() == Type::ReferenceTypeID);
	assert(src_index->get_type() == Type::IntTypeID 
			   || src_index->get_type() == Type::ShortTypeID
			   || src_index->get_type() == Type::CharTypeID
			   || src_index->get_type() == Type::ByteTypeID);

	// Implicit null-checks are handled via deoptimization.
	place_deoptimization_marker(I);

	// load array length
	MachineOperand *len = CreateVirtualRegister(Type::IntTypeID);
	MachineOperand *modrm = new X86_64ModRMOperand(Type::IntTypeID,BaseOp(src_ref),OFFSET(java_array_t,size));
	MachineInstruction *move = new MovInst(SrcOp(modrm)
					      ,DstOp(len)
					      ,get_OperandSize_from_Type(Type::IntTypeID)
					      );
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

void X86_64LoweringVisitor::visit(RETURNInst *I, bool copyOperands) {
	assert(I);
	Type::TypeID type = I->get_type();
	MachineOperand* src_op = (type == Type::VoidTypeID ? 0 : get_op(I->get_operand(0)->to_Instruction()));
	switch (type) {
	case Type::CharTypeID:
	case Type::ByteTypeID:
	case Type::ShortTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	case Type::ReferenceTypeID:
	{
		// TODO: Just for testing, we do not spilt the life time manually here but
		//       let the register allocator do it, if its needed. We only add the
		//       requirement that VirtualReg MUST be in the return register
		
		// MachineOperand *ret_reg = new NativeRegister(type,&RAX);
		// MachineInstruction *reg = new MovInst(
		// 	SrcOp(src_op),
		//	DstOp(ret_reg),
		//	get_OperandSize_from_Type(type));
		LeaveInst *leave = new LeaveInst();
		RetInst *ret = new RetInst(get_OperandSize_from_Type(type),SrcOp(src_op));
		// get_current()->push_back(reg);
		// get_current()->push_back(leave);
		get_current()->push_back(ret);
		get_current()->set_last_insertion_point(get_current()->mi_last());
		set_op(I,ret->get_result().op);
		return;
	}
	case Type::FloatTypeID:
	{
		// MachineOperand *ret_reg = new NativeRegister(type,&XMM0);
		// MachineInstruction *reg = new MovSSInst(
		// 	SrcOp(src_op),
		// 	DstOp(ret_reg));
		LeaveInst *leave = new LeaveInst();
		RetInst *ret = new RetInst(get_OperandSize_from_Type(type),SrcOp(src_op));
		// get_current()->push_back(reg);
		// get_current()->push_back(leave);
		get_current()->push_back(ret);
		get_current()->set_last_insertion_point(get_current()->mi_last());
		set_op(I,ret->get_result().op);
		return;
	}
	case Type::DoubleTypeID:
	{
		// MachineOperand *ret_reg = new NativeRegister(type,&XMM0);
		// MachineInstruction *reg = new MovSDInst(
		//	SrcOp(src_op),
		//	DstOp(ret_reg));
		LeaveInst *leave = new LeaveInst();
		RetInst *ret = new RetInst(get_OperandSize_from_Type(type),SrcOp(src_op));
		// get_current()->push_back(reg);
		// get_current()->push_back(leave);
		get_current()->push_back(ret);
		get_current()->set_last_insertion_point(get_current()->mi_last());
		set_op(I,ret->get_result().op);
		return;
	}
	case Type::VoidTypeID:
	{
		LeaveInst *leave = new LeaveInst();
		RetInst *ret = new RetInst();
		// get_current()->push_back(leave);
		get_current()->push_back(ret);
		get_current()->set_last_insertion_point(get_current()->mi_last());
		set_op(I,ret->get_result().op);
		return;
	}
	default: break;
	}
	ABORT_MSG("x86_64 Lowering not supported",
		"Inst: " << I << " type: " << type);
}

void X86_64LoweringVisitor::visit(CASTInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
	Type::TypeID from = I->get_operand(0)->to_Instruction()->get_type();
	Type::TypeID to = I->get_type();

	switch (from) {
	case Type::IntTypeID:
	{
		switch (to) {
		case Type::ByteTypeID:
		{
			MachineInstruction *mov = new MovSXInst(SrcOp(src_op), DstOp(CreateVirtualRegister(to)),
					GPInstruction::OS_8, GPInstruction::OS_32);
			get_current()->push_back(mov);
			set_op(I, mov->get_result().op);
			return;
		}
		case Type::CharTypeID:
		case Type::ShortTypeID:
		{
			MachineInstruction *mov = new MovSXInst(SrcOp(src_op), DstOp(CreateVirtualRegister(to)),
					GPInstruction::OS_16, GPInstruction::OS_32);
			get_current()->push_back(mov);
			set_op(I, mov->get_result().op);
			return;
		}
		case Type::LongTypeID:
		{
			MachineInstruction *mov = new MovSXInst(
				SrcOp(src_op),
				DstOp(CreateVirtualRegister(to)),
				GPInstruction::OS_32, GPInstruction::OS_64);
			get_current()->push_back(mov);
			set_op(I,mov->get_result().op);
			return;
		}
 		case Type::DoubleTypeID:
		{
			MachineOperand *clear_result = CreateVirtualRegister(Type::DoubleTypeID);
			MachineOperand *result = CreateVirtualRegister(Type::DoubleTypeID);
			MachineInstruction *clear = new MovImmSDInst(SrcOp(new Immediate(0, Type::DoubleType())), DstOp(clear_result));
			MachineInstruction *conversion = new CVTSI2SDInst(
				SrcOp(src_op),
				DstOp(result),
				GPInstruction::OS_32, GPInstruction::OS_64);
			// Connect the clear result to the conversion result via register requirement
			// this is so SSA is preserved
			auto requirement = std::make_unique<MachineRegisterRequirement>(clear_result);
			conversion->get_result().set_MachineRegisterRequirement(requirement);

			get_current()->push_back(clear);
			get_current()->push_back(conversion);
			set_op(I,conversion->get_result().op);
			return;
		}
		case Type::FloatTypeID:
		{
			MachineInstruction *mov = new CVTSI2SSInst(
				SrcOp(src_op),
				DstOp(CreateVirtualRegister(to)),
				GPInstruction::OS_32, GPInstruction::OS_32);
			get_current()->push_back(mov);
			set_op(I,mov->get_result().op);
			return;
		}
		default:
			break;
		}
		break;
	}
	case Type::LongTypeID:
	{
		switch (to) {
		case Type::IntTypeID:
		{
			// force a 32bit move to cut the upper byte
			MachineInstruction *mov = new MovInst(SrcOp(src_op), DstOp(CreateVirtualRegister(to)), GPInstruction::OS_32);
			get_current()->push_back(mov);
			set_op(I, mov->get_result().op);
			return;
		}
		case Type::DoubleTypeID:
		{
			MachineInstruction *mov = new CVTSI2SDInst(
				SrcOp(src_op),
				DstOp(CreateVirtualRegister(to)),
				GPInstruction::OS_64, GPInstruction::OS_64);
			get_current()->push_back(mov);
			set_op(I,mov->get_result().op);
			return;
		}
		case Type::FloatTypeID:
		{
			MachineInstruction *mov = new CVTSI2SSInst(
				SrcOp(src_op),
				DstOp(CreateVirtualRegister(to)),
				GPInstruction::OS_64, GPInstruction::OS_32);
			get_current()->push_back(mov);
			set_op(I,mov->get_result().op);
			return;
		}
		default:
			break;
		}

		break;
	}

	case Type::DoubleTypeID:
	{
		switch (to) {

		case Type::IntTypeID:
		{
			// TODO: currently this is replaced by the stackanalysis pass with ICMD_BUILTIN and therefore implemented
			// in a builtin function
		}
		case Type::LongTypeID:
		{
			// TODO: currently this is replaced by the stackanalysis pass with ICMD_BUILTIN and therefore implemented
			// in a builtin function
		}
		case Type::FloatTypeID:
		{
			MachineInstruction *mov = new CVTSD2SSInst(
				SrcOp(src_op),
				DstOp(CreateVirtualRegister(to)),
				GPInstruction::OS_64, GPInstruction::OS_32);
			get_current()->push_back(mov);
			set_op(I,mov->get_result().op);
			return;
		}
		default:
			break;
		}
		break;
	}
	case Type::FloatTypeID:
	{
		switch(to) {

		case Type::IntTypeID:
		{
			// TODO: currently this is replaced by the stackanalysis pass with ICMD_BUILTIN and therefore implemented
			// in a builtin function
		}
		case Type::LongTypeID:
		{
			// TODO: currently this is replaced by the stackanalysis pass with ICMD_BUILTIN and therefore implemented
			// in a builtin function
		}
		case Type::DoubleTypeID:
		{
			MachineInstruction *mov = new CVTSS2SDInst(
				SrcOp(src_op),
				DstOp(CreateVirtualRegister(to)),
				GPInstruction::OS_64);
			get_current()->push_back(mov);
			set_op(I,mov->get_result().op);
			return;
		}
		default:
			break;
		}
	break;
	}
	default:
		break;
	}

	ABORT_MSG("x86_64 Cast not supported!", "From " << from << " to " << to );
}

void X86_64LoweringVisitor::visit(INVOKEInst *I, bool copyOperands) {
	assert(I);
	Type::TypeID type = I->get_type();
	bool call_has_result = type != Type::VoidTypeID;
	MethodDescriptor &MD = I->get_MethodDescriptor();
	MachineMethodDescriptor MMD(MD, get_Backend()->get_JITData()->get_MachineOperandFactory());
	StackSlotManager *SSM = get_Backend()->get_JITData()->get_StackSlotManager();

	// Implicit null-checks are handled via deoptimization
	// TODO: Check with Matthias if we can move this really here
	//       Its moved here because live-ranges of parameters are split
	//       afterwards, but the deoptimize causes the arguments to live longer
	//       then necessary
	if (I->to_INVOKEVIRTUALInst()) {
		place_deoptimization_marker(I->to_INVOKEVIRTUALInst());
	} else if (I->to_INVOKEINTERFACEInst()) {
		place_deoptimization_marker(I->to_INVOKEINTERFACEInst());
	}

	// operands for the call
	VirtualRegister *addr = CreateVirtualRegister(Type::ReferenceTypeID);
	MachineOperand *result = CreateVirtualRegister(type);

	// create call
	MachineInstruction* call = new CallInst(SrcOp(addr), DstOp(result), I->op_size(), MMD, get_Backend());

	// Split live ranges of parameters for easier register assignment
	int arg_counter = 0;
	for (std::size_t i = 0; i < I->op_size(); ++i ) {
		MachineOperand *arg_dst = MMD[i];
		if (arg_dst->is_StackSlot()) {
			arg_dst = SSM->create_argument_slot(arg_dst->get_type(), arg_counter++);
		} else {
			// Even split the live range of arguments, since the same vreg may occur
			// in multiple argument slots and it needs a different vreg assigned
			arg_dst = CreateVirtualRegister(arg_dst->get_type());
		}

		MachineInstruction* mov = get_Backend()->create_Move(
			get_op(I->get_operand(i)->to_Instruction()),
			arg_dst
		);
		// Guide the reg alloc by restricting the spilt live-ranges to the required
		// argument registers
		if (!arg_dst->is_ManagedStackSlot()) {
			auto requirement = std::make_unique<MachineRegisterRequirement>(cast_to<X86_64Register>(MMD[i]));
			// mov->get_result().set_MachineRegisterRequirement(requirement);
		}
		get_current()->push_back(mov);
		
		// set call operand
		call->set_operand(i+1,arg_dst);
	}

	if (I->to_INVOKESTATICInst() || I->to_INVOKESPECIALInst()) {
		methodinfo* callee = I->get_fmiref()->p.method;
		Immediate *method_address = new Immediate(reinterpret_cast<s8>(callee->code->entrypoint),
				Type::ReferenceType());
		MachineInstruction *mov = get_Backend()->create_Move(method_address, addr);
		get_current()->push_back(mov);
	} else if (I->to_INVOKEVIRTUALInst()) {

		// Implicit null-checks are handled via deoptimization.
		// place_deoptimization_marker(I->to_INVOKEVIRTUALInst());

		methodinfo* callee = I->get_fmiref()->p.method;
		int32_t s1 = OFFSET(vftbl_t, table[0]) + sizeof(methodptr) * callee->vftblindex;
		VirtualRegister *vftbl_address = CreateVirtualRegister(Type::ReferenceTypeID);
		MachineOperand *receiver = get_op(I->get_operand(0)->to_Instruction());
		MachineOperand *vftbl_offset = new X86_64ModRMOperand(Type::ReferenceTypeID, BaseOp(receiver), OFFSET(java_object_t, vftbl));
		MachineInstruction *load_vftbl_address = new MovInst(SrcOp(vftbl_offset), DstOp(vftbl_address),
				GPInstruction::OS_64);
		get_current()->push_back(load_vftbl_address);

		MachineOperand *method_offset = new X86_64ModRMOperand(Type::ReferenceTypeID, BaseOp(vftbl_address), s1);
		MachineInstruction *load_method_address = new MovInst(SrcOp(method_offset), DstOp(addr),
				GPInstruction::OS_64);
		get_current()->push_back(load_method_address);
	} else if (I->to_INVOKEINTERFACEInst()) {

		// Implicit null-checks are handled via deoptimization.
		// place_deoptimization_marker(I->to_INVOKEINTERFACEInst());

		methodinfo* callee = I->get_fmiref()->p.method;
		int32_t s1 = OFFSET(vftbl_t, interfacetable[0]) - sizeof(methodptr) * callee->clazz->index;
		VirtualRegister *vftbl_address = CreateVirtualRegister(Type::ReferenceTypeID);
		MachineOperand *receiver = get_op(I->get_operand(0)->to_Instruction());
		MachineOperand *vftbl_offset = new X86_64ModRMOperand(Type::ReferenceTypeID, BaseOp(receiver), OFFSET(java_object_t, vftbl));
		MachineInstruction *load_vftbl_address = new MovInst(SrcOp(vftbl_offset), DstOp(vftbl_address),
				GPInstruction::OS_64);
		get_current()->push_back(load_vftbl_address);

		VirtualRegister *interface_address = CreateVirtualRegister(Type::ReferenceTypeID);
		MachineOperand *interface_offset = new X86_64ModRMOperand(Type::ReferenceTypeID, BaseOp(vftbl_address), s1);
		MachineInstruction *load_interface_address = new MovInst(SrcOp(interface_offset),
				DstOp(interface_address), GPInstruction::OS_64);
		get_current()->push_back(load_interface_address);

		int32_t s2 = sizeof(methodptr) * (callee - callee->clazz->methods);
		MachineOperand *method_offset = new X86_64ModRMOperand(Type::ReferenceTypeID, BaseOp(interface_address), s2);
		MachineInstruction *load_method_address = new MovInst(SrcOp(method_offset), DstOp(addr),
				GPInstruction::OS_64);
		get_current()->push_back(load_method_address);
	} else if (I->to_BUILTINInst()) {
		Immediate *method_address = new Immediate(reinterpret_cast<s8>(I->to_BUILTINInst()->get_address()),
				Type::ReferenceType());
		MachineInstruction *mov = get_Backend()->create_Move(method_address, addr);
		get_current()->push_back(mov);
	}

	// add call
	get_current()->push_back(call);

	// get result
	if (call_has_result) {
		MachineOperand *dst = CreateVirtualRegister(type);
		MachineInstruction *reg = get_Backend()->create_Move(result, dst);
		get_current()->push_back(reg);
		set_op(I,reg->get_result().op);
	}
}

void X86_64LoweringVisitor::visit(INVOKESTATICInst *I, bool copyOperands) {
	visit(static_cast<INVOKEInst*>(I), copyOperands);
}

void X86_64LoweringVisitor::visit(INVOKESPECIALInst *I, bool copyOperands) {
	visit(static_cast<INVOKEInst*>(I), copyOperands);
}

void X86_64LoweringVisitor::visit(INVOKEVIRTUALInst *I, bool copyOperands) {
	visit(static_cast<INVOKEInst*>(I), copyOperands);
}

void X86_64LoweringVisitor::visit(INVOKEINTERFACEInst *I, bool copyOperands) {
	visit(static_cast<INVOKEInst*>(I), copyOperands);
}

void X86_64LoweringVisitor::visit(BUILTINInst *I, bool copyOperands) {
	visit(static_cast<INVOKEInst*>(I), copyOperands);
}

void X86_64LoweringVisitor::visit(GETFIELDInst *I, bool copyOperands) {
	assert(I);

	// Implicit null-checks are handled via deoptimization.
	place_deoptimization_marker(I);

	MachineOperand* objectref = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand *field_address = new X86_64ModRMOperand(I->get_type(),
			BaseOp(objectref), I->get_field()->offset);
	MachineOperand *vreg = CreateVirtualRegister(I->get_type());
	MachineInstruction *read_field = get_Backend()->create_Move(field_address, vreg);
	get_current()->push_back(read_field);
	set_op(I, read_field->get_result().op);
}

void X86_64LoweringVisitor::visit(PUTFIELDInst *I, bool copyOperands) {
	assert(I);

	// Implicit null-checks are handled via deoptimization.
	place_deoptimization_marker(I);

	MachineOperand *objectref = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand *value = get_op(I->get_operand(1)->to_Instruction());
	MachineOperand *field_address = new X86_64ModRMOperand(value->get_type(),
			BaseOp(objectref), I->get_field()->offset);
	MachineInstruction *write_field = get_Backend()->create_Move(value, field_address);
	get_current()->push_back(write_field);
	set_op(I, write_field->get_result().op);
}

void X86_64LoweringVisitor::visit(GETSTATICInst *I, bool copyOperands) {
	assert(I);

	Immediate *field_address_imm = new Immediate(reinterpret_cast<s8>(I->get_field()->value),
			Type::ReferenceType());

	// TODO Remove this as soon as loads from immediate addresses are supported.
	VirtualRegister *field_address = CreateVirtualRegister(Type::ReferenceTypeID);
	MachineInstruction *load_field_address = get_Backend()->create_Move(field_address_imm,
			field_address);
	get_current()->push_back(load_field_address);

	MachineOperand *modrm = new X86_64ModRMOperand(I->get_type(), BaseOp(field_address));
	MachineOperand *vreg = CreateVirtualRegister(I->get_type());
	MachineInstruction *read_field = get_Backend()->create_Move(modrm, vreg);
	get_current()->push_back(read_field);
	set_op(I, read_field->get_result().op);
}

void X86_64LoweringVisitor::visit(PUTSTATICInst *I, bool copyOperands) {
	assert(I);

	Immediate *field_address_imm = new Immediate(reinterpret_cast<s8>(I->get_field()->value),
			Type::ReferenceType());

	// TODO Remove this as soon as stores to immediate addresses are supported.
	VirtualRegister *field_address = CreateVirtualRegister(Type::ReferenceTypeID);
	MachineInstruction *load_field_address = get_Backend()->create_Move(field_address_imm,
			field_address);
	get_current()->push_back(load_field_address);

	MachineOperand *value = get_op(I->get_operand(0)->to_Instruction());
	MachineOperand *modrm = new X86_64ModRMOperand(value->get_type(), BaseOp(field_address));
	MachineInstruction *write_field = get_Backend()->create_Move(value, modrm);
	get_current()->push_back(write_field);
	set_op(I, write_field->get_result().op);
}

void X86_64LoweringVisitor::visit(LOOKUPSWITCHInst *I, bool copyOperands) {
	assert(I);
	MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
	Type::TypeID type = I->get_type();

	LOOKUPSWITCHInst::succ_const_iterator s = I->succ_begin();
	for(LOOKUPSWITCHInst::match_iterator i = I->match_begin(),
			e = I->match_end(); i != e; ++i) {
		// create compare
		CmpInst *cmp = new CmpInst(
			Src2Op(new Immediate(*i,Type::IntType())),
			Src1Op(src_op),
			get_OperandSize_from_Type(type));
		get_current()->push_back(cmp);
		// create new block
		MachineBasicBlock *then_block = get(s->get());
		MachineBasicBlock *else_block = new_block();
		assert(else_block);
		else_block->insert_pred(get_current());
		else_block->push_front(new MachineLabelInst());
		// create cond jump
		MachineInstruction *cjmp = new CondJumpInst(Cond::E, then_block, else_block);
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

void X86_64LoweringVisitor::visit(TABLESWITCHInst *I, bool copyOperands) {
	assert_msg(0 , "Fix CondJump");
	assert(I);
	MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
	Type::TypeID type = I->get_type();
	VirtualRegister *src = CreateVirtualRegister(type);
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
	VirtualRegister *addr = CreateVirtualRegister(Type::ReferenceTypeID);
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

void X86_64LoweringVisitor::visit(CHECKNULLInst *I, bool copyOperands) {
	
}

void X86_64LoweringVisitor::visit(AssumptionInst *I, bool copyOperands) {
	assert(I);

	SourceStateInst *source_state = I->get_source_state();
	assert(source_state);
	MachineDeoptInst *MI = new MachineDeoptInst(
			source_state->get_source_location(), source_state->op_size());
	lower_source_state_dependencies(MI, source_state);
	get_current()->push_back(MI);

	// compare with `1`
	MachineOperand* cond_op = get_op(I->get_operand(0)->to_Instruction());
	Immediate *imm = new Immediate(1,Type::IntType());
	CmpInst *cmp = new CmpInst(
		Src2Op(imm),
		Src1Op(cond_op),
		get_OperandSize_from_Type(Type::IntTypeID));
	get_current()->push_back(cmp);

	// deoptimize
	auto def_instr = new MachineDefInst(Type::ReferenceTypeID, 
		get_Backend()->get_JITData()->get_MachineOperandFactory());
	get_current()->push_back(def_instr);

	MachineOperand *methodptr = def_instr->get_result().op;
	MachineInstruction *trap = new CondTrapInst(Cond::NE, TRAP_DEOPTIMIZE, SrcOp(methodptr));
	get_current()->push_back(trap);
}

void X86_64LoweringVisitor::visit(DeoptimizeInst *I, bool copyOperands) {
	assert(I);

	SourceStateInst *source_state = I->get_source_state();
	assert(source_state);
	MachineDeoptInst *MI = new MachineDeoptInst(
			source_state->get_source_location(), source_state->op_size());
	lower_source_state_dependencies(MI, source_state);
	get_current()->push_back(MI);

	// deoptimize
	auto def_instr = new MachineDefInst(Type::ReferenceTypeID, 
		get_Backend()->get_JITData()->get_MachineOperandFactory());
	get_current()->push_back(def_instr);
	
	MachineOperand *methodptr = def_instr->get_result().op;
	MachineInstruction *deoptimize_trap = new TrapInst(TRAP_DEOPTIMIZE, SrcOp(methodptr));
	get_current()->push_back(deoptimize_trap);
}

void X86_64LoweringVisitor::lowerComplex(Instruction* I, int ruleId){

	switch(ruleId){
		case AddImmImm:
		{
			assert(I);
			Type::TypeID type = I->get_type();
			CONSTInst* const_left = I->get_operand(0)->to_Instruction()->to_CONSTInst();
			CONSTInst* const_right = I->get_operand(1)->to_Instruction()->to_CONSTInst();

			Immediate *imm = NULL;
			switch (type) {
				case Type::IntTypeID: 
				{
					s4 val = const_left->get_Int() + const_right->get_Int();
					imm = new Immediate(val, Type::IntType());
					break;
				}
				case Type::LongTypeID: 
				{
					s8 val = const_left->get_Long() + const_right->get_Long();
					imm = new Immediate(val, Type::LongType());
					break;
				}
				case Type::FloatTypeID: 
				{
					float val = const_left->get_Float() + const_right->get_Float();
					imm = new Immediate(val, Type::FloatType());
					break;
				}
				case Type::DoubleTypeID: 
				{
					double val = const_left->get_Double() + const_right->get_Double();
					imm = new Immediate(val, Type::DoubleType());
					break;
				}
				default:
					assert(0);
					break;
			}
			
			VirtualRegister *reg = CreateVirtualRegister(I->get_type());
			MachineInstruction *move = get_Backend()->create_Move(imm,reg);
			get_current()->push_back(move);
			set_op(I,move->get_result().op);
			break;
		}
		// all immediates should be second operand, see ssa construction pass
		case AddRegImm:
		{	// todo: copyOperands?!?
			// todo: extend pattern to not rely on data type, instead check if const fits into imm encoding
			assert(I);
			Type::TypeID type = I->get_type();

			MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
			Immediate* const_op = new Immediate(I->get_operand(1)->to_Instruction()->to_CONSTInst());

			VirtualRegister *dst = CreateVirtualRegister(type);
			MachineInstruction *mov = get_Backend()->create_Move(src_op,dst);
			
			MachineInstruction *alu = NULL;

			switch (type) {
				case Type::ByteTypeID:
				case Type::IntTypeID:
				case Type::LongTypeID:
					alu = new AddInst(
						Src2Op(const_op),
						DstSrc1Op(dst),
						get_OperandSize_from_Type(type));
					break;
				default:
					ABORT_MSG("x86_64: AddImm Lowering not supported",
						"Inst: " << I << " type: " << type);
			}
			get_current()->push_back(mov);
			get_current()->push_back(alu);
			set_op(I,alu->get_result().op);

			break;

		}
		case SubRegImm:
		{
			assert(I);
			Type::TypeID type = I->get_type();

			MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
			Immediate* const_op = new Immediate(I->get_operand(1)->to_Instruction()->to_CONSTInst());

			VirtualRegister *dst = CreateVirtualRegister(type);
			MachineInstruction *mov = get_Backend()->create_Move(src_op,dst);
			MachineInstruction *alu = NULL;

			switch (type) {
				case Type::ByteTypeID:
				case Type::IntTypeID:
				case Type::LongTypeID:
					alu = new SubInst(
						Src2Op(const_op),
						DstSrc1Op(dst),
						get_OperandSize_from_Type(type));
					break;
				default:
					ABORT_MSG("x86_64: SubRegImm Lowering not supported",
						"Inst: " << I << " type: " << type);
			}
			get_current()->push_back(mov);
			get_current()->push_back(alu);
			set_op(I,alu->get_result().op);
			break;
		}
		case MulRegImm:
		{
			// todo: copyOperands?!?
			// todo: 3operand version!
			assert(I);
			Type::TypeID type = I->get_type();

			MachineOperand* src_op = get_op(I->get_operand(0)->to_Instruction());
			Immediate* const_op = new Immediate(I->get_operand(1)->to_Instruction()->to_CONSTInst());

			VirtualRegister *dst = CreateVirtualRegister(type);
			
			MachineInstruction *alu = NULL;

			switch (type) {
				case Type::ByteTypeID:
				case Type::IntTypeID:
				case Type::LongTypeID:
					alu = new IMulImmInst(
						Src1Op(src_op),
						Src2Op(const_op),
						DstOp(dst),
						get_OperandSize_from_Type(type));
					break;
				default:
					ABORT_MSG("x86_64: MulImm Lowering not supported",
						"Inst: " << I << " type: " << type);
			}
			get_current()->push_back(alu);
			set_op(I,alu->get_result().op);

			break;
		}
		// LEA
		case BaseIndexDisplacement:
		{	
			/*
			ADDInstID
				ADDInstID
					stm, 		// base
					stm  		// index
				CONSTInstID		// disp
			*/
			assert(I);
			Type::TypeID type = I->get_type();

			Instruction* nested_add = I->get_operand(0)->to_Instruction();
			MachineOperand* base = get_op(nested_add->get_operand(0)->to_Instruction());
			MachineOperand* index = get_op(nested_add->get_operand(1)->to_Instruction());

			CONSTInst* displacement = I->get_operand(1)->to_Instruction()->to_CONSTInst();
			VirtualRegister *dst = CreateVirtualRegister(type);

			MachineOperand *modrm = new X86_64ModRMOperand(Type::VoidTypeID,BaseOp(base),IndexOp(index),displacement->get_value());
			MachineInstruction* lea = new LEAInst(DstOp(dst), get_OperandSize_from_Type(type), SrcOp(modrm));

			get_current()->push_back(lea);
			set_op(I,lea->get_result().op);
			break;
		}
		case BaseIndexDisplacement2:
		{	
			/*
			ADDInstID
					stm 
					ADDInstID
						stm
						CONSTInstID
			*/
			assert(I);
			Type::TypeID type = I->get_type();
			MachineOperand* base = get_op(I->get_operand(0)->to_Instruction());

			Instruction* nested_add = I->get_operand(1)->to_Instruction();
			MachineOperand* index = get_op(nested_add->get_operand(0)->to_Instruction());

			CONSTInst* displacement = nested_add->get_operand(1)->to_Instruction()->to_CONSTInst();
			VirtualRegister *dst = CreateVirtualRegister(type);

			MachineOperand *modrm = new X86_64ModRMOperand(Type::VoidTypeID,BaseOp(base),IndexOp(index),displacement->get_value());
			MachineInstruction* lea = new LEAInst(DstOp(dst), get_OperandSize_from_Type(type), SrcOp(modrm));

			get_current()->push_back(lea);
			set_op(I,lea->get_result().op);
			break;
		}
		case BaseIndexMultiplier:
		{	
			/*
			ADDInstID
				stm, 				// base
				MULInstID
					stm, 			// index
					CONSTInstID		// multiplier
			*/
			assert(I);
			Type::TypeID type = I->get_type();

			MachineOperand* base = get_op(I->get_operand(0)->to_Instruction());

			Instruction* nested_mul = I->get_operand(1)->to_Instruction();
			MachineOperand* index = get_op(nested_mul->get_operand(0)->to_Instruction());
			CONSTInst* multiplier = nested_mul->get_operand(1)->to_Instruction()->to_CONSTInst();

			VirtualRegister *dst = CreateVirtualRegister(type);
			MachineOperand *modrm = new X86_64ModRMOperand(Type::VoidTypeID
								      ,BaseOp(base)
								      ,IndexOp(index)
								      ,X86_64ModRMOperand::get_scale(multiplier->get_Int())
								      );
			MachineInstruction* lea = new LEAInst(DstOp(dst), get_OperandSize_from_Type(type), SrcOp(modrm));

			get_current()->push_back(lea);
			set_op(I,lea->get_result().op);
			break;
		}
		case BaseIndexMultiplier2:
		{	
			/*
			ADDInstID
				MULInstID
					stm, 			// index
					CONSTInstID		// multiplier
				stm 				// base
			*/
			assert(I);
			Type::TypeID type = I->get_type();

			MachineOperand* base = get_op(I->get_operand(1)->to_Instruction());

			Instruction* nested_mul = I->get_operand(0)->to_Instruction();
			MachineOperand* index = get_op(nested_mul->get_operand(0)->to_Instruction());
			CONSTInst* multiplier = nested_mul->get_operand(1)->to_Instruction()->to_CONSTInst();

			VirtualRegister *dst = CreateVirtualRegister(type);
			MachineOperand *modrm = new X86_64ModRMOperand(Type::VoidTypeID
								      ,BaseOp(base)
								      ,IndexOp(index)
								      ,X86_64ModRMOperand::get_scale(multiplier->get_Int())
								      );
			MachineInstruction* lea = new LEAInst(DstOp(dst), get_OperandSize_from_Type(type), SrcOp(modrm));

			get_current()->push_back(lea);
			set_op(I,lea->get_result().op);
			break;
		}
		case BaseIndexMultiplierDisplacement:
		{	
			/*
			ADDInstID
				ADDInstID
					stm, 				// base
					MULInstID
						stm, 			// index
						CONSTInstID 	// multiplier
				CONSTInstID 			// displacement
			*/
			assert(I);
			Type::TypeID type = I->get_type();

			Instruction* bim_root = I->get_operand(0)->to_Instruction();
			MachineOperand* base = get_op(bim_root->get_operand(0)->to_Instruction());

			Instruction* nested_mul = bim_root->get_operand(1)->to_Instruction();
			MachineOperand* index = get_op(nested_mul->get_operand(0)->to_Instruction());
			CONSTInst* multiplier = nested_mul->get_operand(1)->to_Instruction()->to_CONSTInst();

			CONSTInst* displacement = I->get_operand(1)->to_Instruction()->to_CONSTInst();

			VirtualRegister *dst = CreateVirtualRegister(type);
			MachineOperand *modrm = new X86_64ModRMOperand(Type::VoidTypeID
								      ,BaseOp(base)
								      ,IndexOp(index)
								      ,X86_64ModRMOperand::get_scale(multiplier->get_Int())
								      ,displacement->get_value()
								      );
			MachineInstruction* lea = new LEAInst(DstOp(dst), get_OperandSize_from_Type(type), SrcOp(modrm));

			get_current()->push_back(lea);
			set_op(I,lea->get_result().op);
			break;
		}
		case BaseIndexMultiplierDisplacement2:
		{	
			/*
			ADDInstID
				stm, 					// base
				ADDInstID
					MULInstID
						stm, 			// index
						CONSTInstID		// multiplier
					CONSTInstID 		// displacement
			*/
			assert(I);
			Type::TypeID type = I->get_type();

			MachineOperand* base = get_op(I->get_operand(0)->to_Instruction());

			Instruction* mul_add = I->get_operand(1)->to_Instruction();
			MachineOperand* index = get_op(mul_add->get_operand(0)->to_Instruction()->get_operand(0)->to_Instruction());
			CONSTInst* multiplier = mul_add->get_operand(0)->to_Instruction()->get_operand(1)->to_Instruction()->to_CONSTInst();

			CONSTInst* displacement = mul_add->get_operand(1)->to_Instruction()->to_CONSTInst();

			VirtualRegister *dst = CreateVirtualRegister(type);
			MachineOperand *modrm = new X86_64ModRMOperand(Type::VoidTypeID
								      ,BaseOp(base)
								      ,IndexOp(index)
								      ,X86_64ModRMOperand::get_scale(multiplier->get_Int())
								      ,displacement->get_value()
								      );
			MachineInstruction* lea = new LEAInst(DstOp(dst), get_OperandSize_from_Type(type), SrcOp(modrm));

			get_current()->push_back(lea);
			set_op(I,lea->get_result().op);
			break;
		}
		case ALoad:
		{
			/*
			ALOADInstID
				AREFInstID
					stm,
					stm
			*/

			assert(I);
			Instruction *ref = I->get_operand(0)->to_Instruction();
			assert(ref);
			MachineOperand* src_ref = get_op(ref->get_operand(0)->to_Instruction());
			MachineOperand* src_index = get_op(ref->get_operand(1)->to_Instruction());
			assert(src_ref->get_type() == Type::ReferenceTypeID);
			assert(src_index->get_type() == Type::IntTypeID);

			Type::TypeID type = ref->get_type();

			VirtualRegister *dst = CreateVirtualRegister(type);

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

			// create modrm source operand
			MachineOperand *modrm = new X86_64ModRMOperand(type,BaseOp(src_ref),IndexOp(src_index),type,offset);
			MachineInstruction *move = get_Backend()->create_Move(modrm, dst);
			get_current()->push_back(move);
			set_op(I,move->get_result().op);
			break;
		}
		case AStore:
		{
			/*
			ASTOREInstID
				AREFInstID,
					stm,
					stm
				stm
			*/

			assert(I);
			Instruction *ref = I->get_operand(0)->to_Instruction();
			assert(ref);
			MachineOperand* dst_ref = get_op(ref->get_operand(0)->to_Instruction());
			MachineOperand* dst_index = get_op(ref->get_operand(1)->to_Instruction());
			assert(dst_ref->get_type() == Type::ReferenceTypeID);
			assert(dst_index->get_type() == Type::IntTypeID);
			MachineOperand *src = get_op(I->get_operand(1)->to_Instruction());

			Type::TypeID type = ref->get_type();

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
				// TODO: implement me
			default:
				ABORT_MSG("x86_64 Lowering not supported",
					"Inst: " << I << " type: " << type);
				offset = 0;
			}

			// create modrm source operand
			MachineOperand *modrm = new X86_64ModRMOperand(type,BaseOp(dst_ref),IndexOp(dst_index),type,offset);
			MachineInstruction *move = get_Backend()->create_Move(src, modrm);
			get_current()->push_back(move);
			set_op(I,move->get_result().op);
			break;
		}
		case AStoreImm:
		{
			/*
			ASTOREInstID
				AREFInstID,
					stm,
					stm
				CONSTInstID	
			*/	

			assert(I);
			Instruction *ref = I->get_operand(0)->to_Instruction();
			assert(ref);
			MachineOperand* dst_ref = get_op(ref->get_operand(0)->to_Instruction());
			MachineOperand* dst_index = get_op(ref->get_operand(1)->to_Instruction());
			assert(dst_ref->get_type() == Type::ReferenceTypeID);
			assert(dst_index->get_type() == Type::IntTypeID);
			Immediate* imm = new Immediate(I->get_operand(1)->to_Instruction()->to_CONSTInst());

			Type::TypeID type = ref->get_type();

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
				// TODO: implement me
			default:
				ABORT_MSG("x86_64 Lowering not supported",
					"Inst: " << I << " type: " << type);
				offset = 0;
			}

			// create modrm source operand
			MachineOperand *modrm = new X86_64ModRMOperand(type,BaseOp(dst_ref),IndexOp(dst_index),type,offset);
			MachineInstruction *move = get_Backend()->create_Move(imm, modrm);
			get_current()->push_back(move);
			set_op(I,move->get_result().op);
			break;
		}
		default:
			ABORT_MSG("Rule not supported", "Rule " << ruleId << " is not supported by method lowerComplex!");

	}
}

void X86_64LoweringVisitor::setupSrcDst(MachineOperand*& src_op1, MachineOperand*& src_op2, VirtualRegister*& dst, 
	Type::TypeID type, bool copyOperands, bool isCommutable) {

	if (!copyOperands){
		if (src_op1->is_Register()){
			dst = src_op1->to_Register()->to_VirtualRegister();
			return;
		} else if (src_op2->is_Register() && isCommutable){
			dst = src_op2->to_Register()->to_VirtualRegister();
			src_op2 = src_op1;
			return;
		} 
	}
	dst = CreateVirtualRegister(type);
	MachineInstruction *mov = get_Backend()->create_Move(src_op1,dst);
	get_current()->push_back(mov);
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
