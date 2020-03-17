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
#include "vm/jit/compiler2/HIRUtils.hpp"

#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/HIRManipulations"

namespace cacao {
namespace jit {
namespace compiler2 {

class HIRManipulations::SplitBasicBlockOperation {
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

		I->begin = second_bb;

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
		end_inst->begin = second_bb;
		second_bb->set_EndInst(end_inst);

		add_dependent(split_at);

		if (HIRUtils::is_state_change_for_other_instruction(split_at)) {
			auto last_state_change = split_at->get_last_state_change();
			auto dependent_inst = HIRUtils::get_depending_instruction(split_at);
			LOG("Replacing last state change for " << dependent_inst << " to " << second_bb
			                                       << "." << nl);
			dependent_inst->replace_state_change_dep(second_bb);
		}

		for(auto it = end_inst->succ_begin(); it != end_inst->succ_end(); it++){
			auto succ = (*it).get();
			succ->replace_predecessor(first_bb, second_bb);
		}

		return second_bb;
	}
};

BeginInst* HIRManipulations::split_basic_block(BeginInst* bb, Instruction* split_at)
{
	return SplitBasicBlockOperation(bb).execute(split_at);
}

void correct_scheduling_edges(Instruction* I, Instruction* schedule_after)
{
	LOG ("Correcting scheduling edges for " << I << nl);
	auto state_change = I->get_last_state_change();
	
	if(!state_change){
		LOG ("Stop correcting scheduling edges for instruction without last state change" << nl);
		return;
	}

	LOG ("Last state change " << state_change << nl);
	auto is_root_of_local_scheduling_graph = state_change->to_BeginInst() != NULL;
	auto is_leaf_of_local_scheduling_graph = !HIRUtils::is_state_change_for_other_instruction(I);

	// If the call site is the last state change for an instruction, this instruction now needs to
	// depend on the last state changing instruction of the inlined region, or the state change
	// before the invocation.
	if (is_leaf_of_local_scheduling_graph) {
		auto first_dependency_after_inlined_region =
		    HIRUtils::get_depending_instruction(schedule_after);
		if(first_dependency_after_inlined_region) {
			LOG("Setting last state change for " << first_dependency_after_inlined_region << " to " << I
												<< nl);
			first_dependency_after_inlined_region->replace_state_change_dep(I);
		} else {
			LOG("No dependency after " << schedule_after << " (except moved instructions)." << nl);
		}
	}

	// If the last state change points to the basic block, then there is no state changing
	// instruction before this instruction in this bb. Therefore the new state change has to be the
	// last state changing instruction before the initial call site (or the new bb).
	if (is_root_of_local_scheduling_graph) {
		LOG("Setting last state change for " << I << " to " << schedule_after << nl);
		I->replace_state_change_dep(schedule_after);
	}
}

void HIRManipulations::move_instruction_to_bb(Instruction* to_move,
                                              BeginInst* target_bb,
                                              Instruction* schedule_after)
{
	LOG("Moving " << to_move << "(" << to_move->get_BeginInst() << ") into " << target_bb << nl);
	to_move->replace_dep(to_move->get_BeginInst(), target_bb);

	if (to_move->is_floating()) {
		LOG("Moving floating instruction " << to_move << " into " << target_bb << nl);
		to_move->set_BeginInst(target_bb);
		return;
	}

	// Source state instructions get the beginInst from their scheduling dependency graph.
	if (to_move->get_opcode() != Instruction::SourceStateInstID){
		LOG("Moving non-floating instruction " << to_move << " into " << target_bb << nl);
		to_move->begin = target_bb; 
		correct_scheduling_edges(to_move, schedule_after);
	}
}

void HIRManipulations::move_instruction_to_method(Instruction* to_move, Method* target_method)
{
	if (to_move->get_opcode() == Instruction::BeginInstID) {
		target_method->add_bb(to_move->to_BeginInst());
		return;
	}

	target_method->add_Instruction(to_move);
}

void HIRManipulations::connect_with_jump(BeginInst* source, BeginInst* target)
{
	assert(source);
	assert(target);
	if (source->get_Method() != target->get_Method()) {
		throw std::runtime_error("HIRManipulations: source and target must be in same method!");
	}

	LOG("Rewriting next bb of " << source << " to " << target << nl);
	auto end_inst = new GOTOInst(source, target);
	source->set_EndInst(end_inst);
	source->get_Method()->add_Instruction(end_inst);
}

void HIRManipulations::remove_instruction(Instruction* to_remove)
{
	LOG("removing " << to_remove << nl);
	assert(to_remove->user_size() <= 1);
	if(to_remove->user_size() == 1){
		auto source_state = (*to_remove->user_begin())->to_SourceStateInst();
		assert(source_state);
		HIRManipulations::remove_instruction(source_state);
	}
	assert(to_remove->rdep_size() == 0);

	/* TODO Inlining: necessary?
	LOG("Removing reverse deps " << to_remove << nl);
	auto it = to_remove->rdep_begin();
	while (it != to_remove->rdep_end()) {
		auto I = *it;
		LOG("rdep " << I << nl);
		I->remove_dep(to_remove);
		it = to_remove->rdep_begin();
	}*/

	LOG("Removing from method " << to_remove << nl);
	if (to_remove->get_opcode() == Instruction::BeginInstID) {
		to_remove->get_Method()->remove_bb(to_remove->to_BeginInst());
	}
	else {
		to_remove->get_Method()->remove_Instruction(to_remove);
	}
}


class HIRManipulations::CoalesceBasicBlocksOperation {
private:
	std::list<BeginInst*> visited;

	Instruction* get_leaf_of_local_scheduling_graph(Instruction* start)
	{
		if (HIRUtils::is_state_change_for_other_instruction(start)) {
			auto dependent = HIRUtils::get_depending_instruction(start);
			return get_leaf_of_local_scheduling_graph(dependent);
		}
		return start;
	}

	void coalesce(BeginInst* first, BeginInst* second)
	{
		LOG("coalesce " << first << " " << second << nl);
		auto method = first->get_Method();

		auto new_end_inst = second->get_EndInst();
		LOG("New end inst for " << first << " is " << new_end_inst << nl);
		first->set_EndInst(new_end_inst);
		second->set_EndInst(NULL);

		for (auto it = method->begin(); it != method->end(); it++) {
			auto inst = *it;
			if (inst->get_BeginInst() == second && inst != second) {
				auto last_state_change = get_leaf_of_local_scheduling_graph(first);
				LOG("State change leaf for " << inst << ": " << last_state_change << nl);
				HIRManipulations::move_instruction_to_bb(inst, first, last_state_change);
			}
		}

		LOG("merging " << first << " with second: " << second << nl);
		for (auto it = new_end_inst->succ_begin(); it != new_end_inst->succ_end(); it++) {
			auto succ = (*it).get();
			succ->replace_predecessor(second, first);
		}
		LOG("merged " << first << " with second: " << second << nl);
	}

public:	
	void coalesce_if_possible(BeginInst* begin_inst)
	{
		LOG("coalesce_if_possible " << begin_inst << nl);
		if(std::find(visited.begin(), visited.end(), begin_inst) != visited.end()){
			LOG(begin_inst << " already visited" << nl);
			return;
		}

		auto end_inst = begin_inst->get_EndInst();
		if (end_inst->succ_size() == 0) {
			LOG("Basic block " << begin_inst << " has no successors. Nothing to merge." << nl);
		}
		else if (end_inst->succ_size() > 1) {
			LOG("Basic block " << begin_inst << " has multiple successors. Nothing to merge." << nl);
		}
		else if (end_inst->succ_begin()->get()->pred_size() > 1) {
			auto first_suc = end_inst->succ_begin()->get();
			LOG("Basic block " << first_suc << " has multiple predecessors. Nothing to merge." << nl);
		}
		else {
			BeginInst* first_suc = end_inst->succ_begin()->get();
			LOG("Basic blocks " << begin_inst << " and " << first_suc << " eligible for merge." << nl);
			auto old_end_inst = begin_inst->get_EndInst();
			coalesce(begin_inst, first_suc);
			HIRManipulations::remove_instruction(first_suc);
			HIRManipulations::remove_instruction(old_end_inst);
			first_suc = NULL;
			old_end_inst = NULL;
			coalesce_if_possible(begin_inst);
			return;
		}

		visited.push_back(begin_inst);
		LOG("invoking for successors for " << end_inst << nl);
		for (auto it = end_inst->succ_begin(); it != end_inst->succ_end(); it++) {
			coalesce_if_possible(it->get());
		}
	}
};

void HIRManipulations::coalesce_bbs(Method* M)
{
	LOG("Coealescing Starting"<<nl);
	CoalesceBasicBlocksOperation().coalesce_if_possible(M->get_init_bb());
	LOG("Coealescing Finished"<<nl);
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
