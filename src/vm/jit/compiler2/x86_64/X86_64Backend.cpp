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
MachineMoveInst* BackendBase<X86_64>::create_Move(MachineOperand *dst,
		MachineOperand* src) const {
	return new MovInst(
		SrcOp(dst),
		DstOp(src));
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
	VirtualRegister *dst = new VirtualRegister();
	MachineMoveInst *move = create_Move(MMD[I->get_index()],dst);
	dag->add(move);
	dag->set_result(move);
	return dag;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerIFInst(IFInst *I) const {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	CmpInst *cmp = new CmpInst(
		Src2Op(UnassignedReg::factory()),
		Src1Op(UnassignedReg::factory()));

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
		err() << Red << "Error: " << reset_color << "Conditioal not supported: "
		      << bold << I->get_condition() << reset_color << nl;
		assert(0);
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

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerADDInst(ADDInst *I) const {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister();
	MachineMoveInst *mov = create_Move(UnassignedReg::factory(),dst);
	AddInst *add = new AddInst(
		Src2Op(UnassignedReg::factory()),
		DstSrc1Op(dst));
	dag->add(mov);
	dag->add(add);
	dag->set_input(1,add,1);
	dag->set_input(0,mov,0);
	dag->set_result(add);
	return dag;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerSUBInst(SUBInst *I) const {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister();
	MachineMoveInst *mov = create_Move(UnassignedReg::factory(), dst);
	SubInst *sub = new SubInst(
		Src2Op(UnassignedReg::factory()),
		DstSrc1Op(dst));
	dag->add(mov);
	dag->add(sub);
	dag->set_input(1,sub,1);
	dag->set_input(0,mov,0);
	dag->set_result(sub);
	return dag;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerMULInst(MULInst *I) const {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister();
	MachineMoveInst *mov = create_Move(UnassignedReg::factory(), dst);
	IMulInst *mul = new IMulInst(
		Src2Op(UnassignedReg::factory()),
		DstSrc1Op(dst));
	dag->add(mov);
	dag->add(mul);
	dag->set_input(1,mul,1);
	dag->set_input(0,mov,0);
	dag->set_result(mul);
	return dag;
}


template<>
LoweredInstDAG* BackendBase<X86_64>::lowerRETURNInst(RETURNInst *I) const {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	MachineMoveInst *reg = create_Move(UnassignedReg::factory(), &RAX);
	LeaveInst *leave = new LeaveInst();
	RetInst *ret = new RetInst();
	dag->add(reg);
	dag->add(leave);
	dag->add(ret);
	dag->set_input(reg);
	dag->set_result(ret);
	return dag;
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
		  MachineInstruction *mov = new MovSXInst(SrcOp(UnassignedReg::factory()),
			  DstOp(new VirtualRegister()),GPRegister::R32, GPRegister::R64);
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
  ABORT_MSG("Cast not supported!", "From " << from << " to " << to );
  return NULL;
}

template<>
compiler2::RegisterFile* BackendBase<X86_64>::get_RegisterFile() const {
	return x86_64::RegisterFile::factory();
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
