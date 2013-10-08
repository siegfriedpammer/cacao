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
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"

#include "Target.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

Backend* Backend::factory(JITData *JD) {
	return new BackendBase<Target>(JD);
}

void LoweringVisitorBase::visit(BeginInst* I) {
	assert(I);
	MachineInstruction *label = new MachineLabelInst();
	get_current()->push_back(label);
	//set_op(I,label->get_result().op);
}

void LoweringVisitorBase::visit(GOTOInst* I) {
	assert(I);
	MachineInstruction *jump = backend->create_Jump(get(I->get_target().get()));
	get_current()->push_back(jump);
	set_op(I,jump->get_result().op);
}

void LoweringVisitorBase::visit(PHIInst* I) {
	assert(I);
	MachinePhiInst *phi = new MachinePhiInst(I->op_size(),I->get_type(),I);
	get_current()->push_back(phi);
	get_current()->insert_phi(phi);
	set_op(I,phi->get_result().op);
}

void LoweringVisitorBase::visit(CONSTInst* I) {
	assert(I);
	VirtualRegister *reg = new VirtualRegister(I->get_type());
	Immediate *imm = new Immediate(I);
	MachineInstruction *move = backend->create_Move(imm,reg);
	get_current()->push_back(move);
	set_op(I,move->get_result().op);
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
