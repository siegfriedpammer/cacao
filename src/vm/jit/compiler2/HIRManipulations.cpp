/* src/vm/jit/compiler2/HIRManipulations.cpp - HIRManipulations

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

#include "vm/jit/compiler2/HIRManipulations.hpp"

#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/HIRManipulations"

namespace cacao {
namespace jit {
namespace compiler2 {
    
static bool is_source_state_or_begin_inst (Instruction* I){
    auto op_code = I->get_opcode();
    return op_code != Instruction::SourceStateInstID && op_code != Instruction::BeginInstID;
}

bool HIRManipulations::is_state_change_for_other_instruction (Instruction* I){
    return std::any_of(I->rdep_begin(), I->rdep_end(), is_source_state_or_begin_inst);
}

Instruction* HIRManipulations::get_depending_instruction (Instruction* I){
    return *std::find_if(I->rdep_begin(), I->rdep_end(), is_source_state_or_begin_inst);
}

class SplitBasicBlockOperation {
private:
	BeginInst* first_bb;
	BeginInst* second_bb;

	bool belongs_to_second_bb(Instruction* I)
	{
		return I->get_BeginInst() == first_bb && I->to_EndInst() == NULL;
	}

	void add_to_second_bb(Instruction* I)
	{
		LOG("Adding " << I << " to second bb " << second_bb << nl); 

		I->set_BeginInst_unsafe(second_bb);

		if (first_bb->get_EndInst() == I) {
			LOG("Setting end inst of second bb " << I << nl);
			second_bb->set_EndInst(I->to_EndInst());
		}

		LOG("Replacing " << first_bb << " dep for " << second_bb << " in " << I << nl);
		I->replace_dep(first_bb, second_bb);
	}

	void add_dependent(Instruction* I)
	{
		auto add_rec = [&](Instruction* inst) {
			if (belongs_to_second_bb(inst)) {
				add_to_second_bb(inst);
				add_dependent(inst);
			}
		};
		auto add_rec_op = [&](Value* v) {
			Instruction* inst = v->to_Instruction();
			if (inst && belongs_to_second_bb(inst)) {
				add_to_second_bb(inst);
				add_dependent(inst);
			}
		};

		std::for_each(I->rdep_begin(), I->rdep_end(), add_rec);
		std::for_each(I->user_begin(), I->user_end(), add_rec_op);
	}

public:
	SplitBasicBlockOperation(BeginInst* bb)
	{
		first_bb = bb;
		second_bb = new BeginInst();
	}

	BeginInst* execute(Instruction* split_at)
	{
		LOG("Splitting bb " << first_bb << " by " << split_at << " into itself and " << second_bb
		                    << nl);
		first_bb->get_Method()->add_bb(second_bb);

		// add end inst of call site bb
		auto end_inst = first_bb->get_EndInst();
		LOG("new end inst for new bb " << end_inst << nl);
		end_inst->set_BeginInst_unsafe(second_bb);
		second_bb->set_EndInst(end_inst);

		add_dependent(split_at);

		if (HIRManipulations::is_state_change_for_other_instruction(split_at)) {
			auto last_state_change = split_at->get_last_state_change();
			auto dependent_inst = HIRManipulations::get_depending_instruction(split_at);
			LOG("Replacing last state change for " << dependent_inst << " to " << last_state_change
			                                       << "." << nl);
			dependent_inst->replace_state_change_dep(last_state_change);
		}

		return second_bb;
	}
};

BeginInst* HIRManipulations::split_basic_block(BeginInst* bb, Instruction* split_at)
{
	return SplitBasicBlockOperation(bb).execute(split_at);
}

void HIRManipulations::move_instruction_to_bb (Instruction* I, BeginInst* target_bb){
	if(I->is_floating()){
		LOG("Moving floating instruction " << I << " into " << target_bb << nl);
		I->set_BeginInst(target_bb);
		return;
	}
}

void HIRManipulations::move_instruction_to_method (Instruction* I, Method* target_method){
	if(I->get_opcode() == Instruction::BeginInstID) {
		target_method->add_bb(I->to_BeginInst());
		return;
	}

	target_method->add_Instruction(I);
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
