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

# include "vm/jit/replace.hpp"

#include "Target.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

Backend* Backend::factory(JITData *JD) {
	return new BackendBase<Target>(JD, new RegisterInfoBase<Target>());
}

void LoweringVisitorBase::visit(BeginInst* I, bool copyOperands) {
	assert(I);
	MachineInstruction *label = new MachineLabelInst();
	get_current()->push_back(label);
	//set_op(I,label->get_result().op);
}

void LoweringVisitorBase::visit(GOTOInst* I, bool copyOperands) {
	assert(I);
	MachineInstruction *jump = backend->create_Jump(get(I->get_target().get()));
	get_current()->push_back(jump);
	set_op(I,jump->get_result().op);
}

void LoweringVisitorBase::visit(PHIInst* I, bool copyOperands) {
	assert(I);
	MachinePhiInst *phi = new MachinePhiInst(I->op_size(),I->get_type(),I);
	phi->set_block(get_current()); // TODO: This shoudl really happen in the MBB
	//get_current()->push_back(phi);
	get_current()->insert_phi(phi);
	set_op(I,phi->get_result().op);
}

void LoweringVisitorBase::visit(CONSTInst* I, bool copyOperands) {
	assert(I);
	VirtualRegister *reg = new VirtualRegister(I->get_type());
	Immediate *imm = new Immediate(I);
	MachineInstruction *move = backend->create_Move(imm,reg);
	get_current()->push_back(move);
	set_op(I,move->get_result().op);
}

void LoweringVisitorBase::lower_source_state_dependencies(MachineReplacementPointInst *MI,
		SourceStateInst *source_state) {
	assert(MI);
	assert(source_state);

	int op_index = 0;

	// lower javalocal dependencies
	for (SourceStateInst::const_javalocal_iterator i = source_state->javalocal_begin(),
			e = source_state->javalocal_end(); i != e; i++) {
		SourceStateInst::Javalocal local = *i;
		Instruction *op = local.value->to_Instruction();
		assert(op);
		MachineOperand *mop = get_op(op);
		MI->set_operand(op_index, mop);
		MI->set_javalocal_index(op_index, local.index);
		op_index++;
	}

	// lower stackvar dependencies
	for (SourceStateInst::const_stackvar_iterator i = source_state->stackvar_begin(),
			e = source_state->stackvar_end(); i != e; i++) {
		Instruction *op = (*i)->to_Instruction();
		assert(op);
		MachineOperand *mop = get_op(op);
		MI->set_operand(op_index, mop);
		MI->set_javalocal_index(op_index, RPLALLOC_STACK);
		op_index++;
	}

	// lower call-site parameter dependencies
	for (SourceStateInst::const_param_iterator i = source_state->param_begin(),
			e = source_state->param_end(); i != e; i++) {
		Instruction *op = (*i)->to_Instruction();
		assert(op);
		MachineOperand *mop = get_op(op);
		MI->set_operand(op_index, mop);
		MI->set_javalocal_index(op_index, RPLALLOC_PARAM);
		op_index++;
	}
}

void LoweringVisitorBase::place_deoptimization_marker(SourceStateAwareInst *I) {
	SourceStateInst *source_state = I->get_source_state();

	if (source_state) {
		MachineDeoptInst *deopt = new MachineDeoptInst(
				source_state->get_source_location(), source_state->op_size());
		lower_source_state_dependencies(deopt, source_state);
		get_current()->push_back(deopt);
	}
}

void LoweringVisitorBase::visit(SourceStateInst* I, bool copyOperands) {
	// A SouceStateInst is just an artificial instruction for holding metadata
	// for ReplacementPointInsts. It has no direct pendant on LIR level and
	// hence needs no lowering logic.
}

void LoweringVisitorBase::visit(ReplacementEntryInst* I, bool copyOperands) {
	SourceStateInst *source_state = I->get_source_state();
	assert(source_state);
	MachineReplacementEntryInst *MI = new MachineReplacementEntryInst(
			source_state->get_source_location(), source_state->op_size());
	lower_source_state_dependencies(MI, source_state);
	get_current()->push_back(MI);
}

MachineBasicBlock* LoweringVisitorBase::new_block() const {
	return *schedule->insert_after(get_current()->self_iterator(),MBBBuilder());
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
