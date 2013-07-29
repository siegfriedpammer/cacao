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
#include "vm/jit/compiler2/LoweredInstDAG.hpp"
#include "vm/jit/compiler2/MethodDescriptor.hpp"
#include "vm/jit/compiler2/CodeMemory.hpp"
#include "vm/jit/compiler2/StackSlotManager.hpp"

#include "toolbox/OStream.hpp"
#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/x86_64"

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
	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
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
MachineJumpInst* BackendBase<X86_64>::create_Jump(BeginInstRef &target) const {
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
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerLOADInst(LOADInst *I) const {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
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
	dag->add(move);
	dag->set_result(move);
	return dag;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerIFInst(IFInst *I) const {
	assert(I);
	Type::TypeID type = I->get_type();
	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	{
		// Integer types
		LoweredInstDAG *dag = new LoweredInstDAG(I);
		CmpInst *cmp = new CmpInst(
			Src2Op(new UnassignedReg(type)),
			Src1Op(new UnassignedReg(type)),
			get_OperandSize_from_Type(type));

		CondJumpInst *cjmp = NULL;
		BeginInstRef &then = I->get_then_target();
		BeginInstRef &els = I->get_else_target();

		switch (I->get_condition()) {
		case Conditional::EQ:
			cjmp = new CondJumpInst(Cond::E, then);
			break;
		case Conditional::LT:
			cjmp = new CondJumpInst(Cond::L, then);
			break;
		case Conditional::LE:
			cjmp = new CondJumpInst(Cond::LE, then);
			break;
		case Conditional::GE:
			cjmp = new CondJumpInst(Cond::GE, then);
			break;
		default:
			ABORT_MSG("x86_64 Conditioal not supported: ",
				  I << " cond: " << I->get_condition());
		}
		JumpInst *jmp = new JumpInst(els);
		dag->add(cmp);
		dag->add(cjmp);
		dag->add(jmp);

		dag->set_input(0,cmp,0);
		dag->set_input(1,cmp,1);
		dag->set_result(jmp);
		return dag;
	}
	default: break;
	}
	ABORT_MSG("x86_64: Lowering not supported",
		"Inst: " << I << " type: " << type);
	return NULL;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerADDInst(ADDInst *I) const {
	assert(I);
	Type::TypeID type = I->get_type();
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = create_Move(new UnassignedReg(type),dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new AddInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	case Type::DoubleTypeID:
		alu = new AddSDInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	dag->add(mov);
	dag->add(alu);
	dag->set_input(1,alu,1);
	dag->set_input(0,mov,0);
	dag->set_result(alu);
	return dag;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerSUBInst(SUBInst *I) const {
	assert(I);
	Type::TypeID type = I->get_type();
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = create_Move(new UnassignedReg(type),dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new SubInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	case Type::DoubleTypeID:
		alu = new SubSDInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	dag->add(mov);
	dag->add(alu);
	dag->set_input(1,alu,1);
	dag->set_input(0,mov,0);
	dag->set_result(alu);
	return dag;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerMULInst(MULInst *I) const {
	assert(I);
	Type::TypeID type = I->get_type();

	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = create_Move(new UnassignedReg(type),dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		alu = new IMulInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst),
			get_OperandSize_from_Type(type));
		break;
	case Type::DoubleTypeID:
		alu = new MulSDInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	dag->add(mov);
	dag->add(alu);
	dag->set_input(1,alu,1);
	dag->set_input(0,mov,0);
	dag->set_result(alu);
	return dag;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerDIVInst(DIVInst *I) const {
	assert(I);
	Type::TypeID type = I->get_type();

	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister(type);
	MachineInstruction *mov = create_Move(new UnassignedReg(type), dst);
	MachineInstruction *alu = NULL;

	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
		break;
	case Type::DoubleTypeID:
		alu = new DivSDInst(
			Src2Op(new UnassignedReg(type)),
			DstSrc1Op(dst));
		break;
	default:
		ABORT_MSG("x86_64: Lowering not supported",
			"Inst: " << I << " type: " << type);
	}
	dag->add(mov);
	dag->add(alu);
	dag->set_input(1,alu,1);
	dag->set_input(0,mov,0);
	dag->set_result(alu);
	return dag;
}


template<>
LoweredInstDAG* BackendBase<X86_64>::lowerRETURNInst(RETURNInst *I) const {
	assert(I);
	Type::TypeID type = I->get_type();
	switch (type) {
	case Type::ByteTypeID:
	case Type::IntTypeID:
	case Type::LongTypeID:
	{
		LoweredInstDAG *dag = new LoweredInstDAG(I);
		MachineInstruction *reg = new MovInst(
			SrcOp(new UnassignedReg(type)),
			DstOp(new NativeRegister(type,&RAX)),
			get_OperandSize_from_Type(type));
		LeaveInst *leave = new LeaveInst();
		RetInst *ret = new RetInst(get_OperandSize_from_Type(type));
		dag->add(reg);
		dag->add(leave);
		dag->add(ret);
		dag->set_input(reg);
		dag->set_result(ret);
		return dag;
	}
	case Type::DoubleTypeID:
	{
		LoweredInstDAG *dag = new LoweredInstDAG(I);
		MachineInstruction *reg = new MovSDInst(
			SrcOp(new UnassignedReg(type)),
			DstOp(new NativeRegister(type,&XMM0)));
		LeaveInst *leave = new LeaveInst();
		RetInst *ret = new RetInst(get_OperandSize_from_Type(type));
		dag->add(reg);
		dag->add(leave);
		dag->add(ret);
		dag->set_input(reg);
		dag->set_result(ret);
		return dag;
	}
	default: break;
	}
	ABORT_MSG("x86_64 Lowering not supported",
		"Inst: " << I << " type: " << type);
	return NULL;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerCASTInst(CASTInst *I) const {
  assert(I);
  LoweredInstDAG *dag = new LoweredInstDAG(I);
  Type::TypeID from = I->get_operand(0)->to_Instruction()->get_type();
  Type::TypeID to = I->get_type();

  switch (from) {
  case Type::IntTypeID:
	  switch (to) {
	  case Type::LongTypeID:
	  {
		  MachineInstruction *mov = new MovSXInst(
			  SrcOp(new UnassignedReg(from)),
			  DstOp(new VirtualRegister(to)),
			  GPInstruction::OS_32, GPInstruction::OS_64);
		  dag->add(mov);
		  dag->set_input(mov);
		  dag->set_result(mov);
		  return dag;
	  }
	  default: break;
	  }
	  break;
  case Type::LongTypeID:
	  switch (to) {
	  case Type::DoubleTypeID:
	  {
		  MachineInstruction *mov = new CVTSI2SDInst(
			  SrcOp(new UnassignedReg(from)),
			  DstOp(new VirtualRegister(to)),
			  GPInstruction::OS_64, GPInstruction::OS_64);
		  dag->add(mov);
		  dag->set_input(mov);
		  dag->set_result(mov);
		  return dag;
	  }
	  default: break;
	  }
	  break;
  default: break;
  }
  ABORT_MSG("x86_64 Cast not supported!", "From " << from << " to " << to );
  return NULL;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerGETSTATICInst(GETSTATICInst *I) const {
	assert(0);
}

template<>
compiler2::RegisterFile*
BackendBase<X86_64>::get_RegisterFile(Type::TypeID type) const {
	return new x86_64::RegisterFile(type);
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
