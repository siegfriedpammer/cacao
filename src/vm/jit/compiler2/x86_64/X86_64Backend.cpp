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

#include "toolbox/OStream.hpp"
#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/x86_64"

namespace cacao {
namespace jit {
namespace compiler2 {

template<>
const char* BackendBase<X86_64>::get_name() const {
	return "x86_64";
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerLOADInst(LOADInst *I) const {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	//MachineInstruction *minst = loadParameter(I->get_index(), I->get_type());
	const MethodDescriptor &MD = I->get_Method()->get_MethodDescriptor();
	const X86_64MachineMethodDescriptor MMD(MD);
	VirtualRegister *dst = new VirtualRegister();
	MachineMoveInst *move = new MachineMoveInst(dst,MMD[I->get_index()]);
	dag->add(move);
	dag->set_result(move);
	return dag;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerIFInst(IFInst *I) const {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	X86_64CmpInst *cmp = new X86_64CmpInst(UnassignedReg::factory(),UnassignedReg::factory());
	X86_64CondJumpInst *cjmp = NULL;

	switch (I->get_condition()) {
	case Conditional::EQ:
		cjmp = new X86_64CondJumpInst(X86_64Cond::E);
		break;
	case Conditional::LT:
		cjmp = new X86_64CondJumpInst(X86_64Cond::L);
		break;
	case Conditional::LE:
		cjmp = new X86_64CondJumpInst(X86_64Cond::LE);
		break;
	case Conditional::GE:
		cjmp = new X86_64CondJumpInst(X86_64Cond::GE);
		break;
	default:
		err() << Red << "Error: " << reset_color << "Conditioal not supported: "
		      << bold << I->get_condition() << reset_color << nl;
		assert(0);
	}
	dag->add(cmp);
	dag->add(cjmp);

	dag->set_input(cmp);
	dag->set_result(cjmp);
	return dag;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerADDInst(ADDInst *I) const {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister();
	MachineMoveInst *mov = new MachineMoveInst(dst, UnassignedReg::factory());
	X86_64AddInst *add = new X86_64AddInst(dst,dst,UnassignedReg::factory());
	dag->add(mov);
	dag->add(add);
	dag->set_input(0,mov,0);
	dag->set_input(1,add,1);
	dag->set_result(add);
	return dag;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerSUBInst(SUBInst *I) const {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister();
	MachineMoveInst *mov = new MachineMoveInst(dst, UnassignedReg::factory());
	X86_64SubInst *sub = new X86_64SubInst(dst,dst,UnassignedReg::factory());
	dag->add(mov);
	dag->add(sub);
	dag->set_input(0,mov,0);
	dag->set_input(1,sub,1);
	dag->set_result(sub);
	return dag;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerRETURNInst(RETURNInst *I) const {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	X86_64RetInst *ret = new X86_64RetInst();
	MachineMoveInst *reg = new MachineMoveInst(&RAX, UnassignedReg::factory());
	dag->add(reg);
	dag->add(ret);
	dag->set_input(reg);
	dag->set_result(ret);
	return dag;
}

template<>
LoweredInstDAG* BackendBase<X86_64>::lowerMULInst(MULInst *I) const {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *dst = new VirtualRegister();
	MachineMoveInst *mov = new MachineMoveInst(dst, UnassignedReg::factory());
	X86_64IMulInst *sub = new X86_64IMulInst(dst,dst,UnassignedReg::factory());
	dag->add(mov);
	dag->add(sub);
	dag->set_input(0,mov,0);
	dag->set_input(1,sub,1);
	dag->set_result(sub);
	return dag;
}

template<>
RegisterFile* BackendBase<X86_64>::get_RegisterFile() const {
	return X86_64RegisterFile::factory();
}

template<>
void BackendBase<X86_64>::emit_Move(const MachineMoveInst *mov, CodeMemory* CM) const {
	MachineOperand *src = mov->get(0).op;
	MachineOperand *dst = mov->get_result().op;
	Register *src_reg = src->to_Register();
	Register *dst_reg = dst->to_Register();
	MachineRegister *src_mreg = src_reg->to_MachineRegister();
	MachineRegister *dst_mreg = dst_reg->to_MachineRegister();

	if (dst_mreg) {
		if (src_mreg) {
			// reg to reg move
			X86_64MovInst move(dst_mreg, src_mreg);
			move.emit(CM);
			return;
		}
	}
	ABORT_MSG("x86_64 TODO","non reg-to-reg moves not yet implemented");
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
