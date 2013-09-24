/* src/vm/jit/compiler2/Backend.cpp - Backend

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

#include "vm/jit/compiler2/Backend.hpp"

#include "Target.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

Backend* Backend::factory(JITData *JD) {
	return new BackendBase<Target>(JD);
}

void LoweringVisitorBase::visit(BeginInst* I) {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	MachineInstruction *label = new MachineLabelStub(I);
	dag->add(label);
	dag->set_result(label);
	set_dag(dag);
}

void LoweringVisitorBase::visit(GOTOInst* I) {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	MachineInstruction *jump = backend->create_Jump(I->get_target());
	dag->add(jump);
	dag->set_result(jump);
	set_dag(dag);
}

void LoweringVisitorBase::visit(PHIInst* I) {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	MachinePhiInst *phi = new MachinePhiInst(I->op_size(),I->get_type());
	dag->add(phi);
	dag->set_input(phi);
	dag->set_result(phi);
	set_dag(dag);
}

void LoweringVisitorBase::visit(CONSTInst* I) {
	assert(I);
	LoweredInstDAG *dag = new LoweredInstDAG(I);
	VirtualRegister *reg = new VirtualRegister(I->get_type());
	Immediate *imm = new Immediate(I);
	MachineInstruction *move = backend->create_Move(imm,reg);
	dag->add(move);
	dag->set_result(move);
	set_dag(dag);
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
