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
	Instruction* split_at;

	bool belongs_to_second_bb(Instruction* I)
	{
		return I->get_BeginInst() == first_bb && I->to_EndInst() == NULL;
	}

	void add_to_second_bb(Instruction* I)
	{
		LOG3("Adding " << I << " to second bb " << second_bb << nl);

		I->begin = second_bb;

		if (first_bb->get_EndInst() == I) {
			LOG3("Setting end inst of second bb " << I << nl);
			second_bb->set_EndInst(I->to_EndInst());
		}

		// This replacement has to be done here, so the SourceStateInst knows the correct basic block it is in
		LOG3("Replacing " << first_bb << " dep for " << second_bb << " in " << I << nl);
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

	void fix_scheduling_information(){
		auto M = first_bb->get_Method();
		for(auto it = M->begin(); it != M->end(); it++){
			auto I = *it;
			if(I->get_BeginInst() == second_bb){
				// this dependency is now given via basic block successor/predecessor information
				LOG3("Replacing " << split_at << " dep for " << second_bb << " in " << I << nl);
				I->replace_dep(split_at, second_bb);
			}
		}
	}

public:
	SplitBasicBlockOperation(BeginInst* bb, Instruction* split_at) : first_bb(bb), split_at(split_at)
	{
		second_bb = new BeginInst();
	}

	BeginInst* execute()
	{
		LOG3("Splitting bb " << first_bb << " by " << split_at << " into itself and " << second_bb
		                    << nl);
		first_bb->get_Method()->add_bb(second_bb);

		// add end inst of call site bb
		auto end_inst = first_bb->get_EndInst();
		LOG3("new end inst for new bb " << end_inst << nl);
		end_inst->begin = second_bb;
		second_bb->set_EndInst(end_inst);

		add_dependent(split_at);
		fix_scheduling_information();

		for (auto it = end_inst->succ_begin(); it != end_inst->succ_end(); it++) {
			auto succ = (*it).get();
			succ->replace_predecessor(first_bb, second_bb);
		}

		LOG3("Split end" << nl);

		return second_bb;
	}
};

BeginInst* HIRManipulations::split_basic_block(BeginInst* bb, Instruction* split_at)
{
	return SplitBasicBlockOperation(bb, split_at).execute();
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

	LOG3("Rewriting next bb of " << source << " to " << target << nl);
	auto end_inst = new GOTOInst(source, target);
	source->set_EndInst(end_inst);
	source->get_Method()->add_Instruction(end_inst);
}

void HIRManipulations::remove_instruction(Instruction* to_remove)
{
	assert(to_remove);
	LOG("removing " << to_remove << nl);

	// This is primarily for deleting SourceStateInsts and should probably removed
	// when correct SourceState handling is implemented.
	auto it_rdep = to_remove->rdep_begin();
	LOG3("rdep size " << to_remove->rdep_size() << nl);
	while (it_rdep != to_remove->rdep_end()) {
		auto I = *it_rdep;
		LOG(Yellow << "Removing dep from " << I << " to " << to_remove << reset_color << nl);
		I->remove_dep(to_remove);
		it_rdep = to_remove->rdep_begin();
	}

	auto it_user = to_remove->user_begin();
	LOG3("user size " << to_remove->user_size() << nl);
	while (it_user != to_remove->user_end()) {
		auto I = *it_user;
		LOG(Yellow << "Removing op from " << I << " to " << to_remove << reset_color << nl);
		I->remove_op(to_remove);
		it_user = to_remove->user_begin();
	}

	LOG3("Removing from method " << to_remove << nl);
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

	void append_leafs_of_local_scheduling_graph(std::list<Instruction*>* append_to, Instruction* I)
	{
		if (I->rdep_size() == 0 && I->get_opcode() != Instruction::SourceStateInstID) {
			append_to->push_back(I);
			return;
		}

		for (auto it = I->rdep_begin(); it != I->rdep_end(); it++) {
			append_leafs_of_local_scheduling_graph(append_to, *it);
		}
	}

	void coalesce(BeginInst* first, BeginInst* second)
	{
		LOG3("coalesce " << first << " " << second << nl);
		auto method = first->get_Method();

		auto new_end_inst = second->get_EndInst();
		LOG3("New end inst for " << first << " is " << new_end_inst << nl);
		first->set_EndInst(new_end_inst);
		second->set_EndInst(NULL);

		std::list<Instruction*> leafs;
		append_leafs_of_local_scheduling_graph(&leafs, first);

		if(leafs.size() == 0){
			leafs.push_back(first);
		}

		LOG3("Leafs: " << nl);
		for(auto leaf_it = leafs.begin(); leaf_it != leafs.end(); leaf_it++){
			LOG3("  " << *leaf_it << nl);
		}

		for (auto it = method->begin(); it != method->end(); it++) {
			auto inst = *it;

			if (inst->get_opcode() == Instruction::SourceStateInstID) continue;
			if (inst->get_BeginInst() != second || inst == second) continue;
			
			if(inst->is_floating()){
				LOG2("Moving floating instruction " << inst << " into " << first << nl);
				inst->set_BeginInst(first);
				continue;
			}

			LOG2("Moving non-floating instruction " << inst << " into " << first << nl);
			inst->begin = first;
		}

		auto rdep_it = second->rdep_begin();
		while (rdep_it != second->rdep_end()){
			auto rdep = *rdep_it;
			rdep->replace_dep(second, first);
			for(auto leaf_it = leafs.begin(); leaf_it != leafs.end(); leaf_it++){
				auto leaf = *leaf_it;
				rdep->append_dep(leaf);
			}
			rdep_it = second->rdep_begin();
		}

		LOG3("merging " << first << " with second: " << second << nl);
		for (auto it = new_end_inst->succ_begin(); it != new_end_inst->succ_end(); it++) {
			auto succ = (*it).get();
			LOG3(" succ " << succ << nl);
			succ->replace_predecessor(second, first);
		}
		LOG("merged " << first << " with second: " << second << nl);
	}

public:
	void coalesce_if_possible(BeginInst* begin_inst)
	{
		LOG3("coalesce_if_possible " << begin_inst << nl);
		if (std::find(visited.begin(), visited.end(), begin_inst) != visited.end()) {
			LOG2(begin_inst << " already visited" << nl);
			return;
		}

		auto end_inst = begin_inst->get_EndInst();
		if (end_inst->succ_size() == 0) {
			LOG("Basic block " << begin_inst << " has no successors. Nothing to merge." << nl);
		}
		else if (end_inst->succ_size() > 1) {
			LOG("Basic block " << begin_inst << " has multiple successors. Nothing to merge."
			                   << nl);
		}
		else if (end_inst->succ_begin()->get()->pred_size() > 1) {
			auto first_suc = end_inst->succ_begin()->get();
			LOG("Basic block " << first_suc << " has multiple predecessors. Nothing to merge."
			                   << nl);
		}
		else {
			BeginInst* first_suc = end_inst->succ_begin()->get();
			LOG("Basic blocks " << begin_inst << " and " << first_suc << " eligible for merge."
			                    << nl);
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
		LOG3("invoking for successors for " << end_inst << nl);
		for (auto it = end_inst->succ_begin(); it != end_inst->succ_end(); it++) {
			coalesce_if_possible(it->get());
		}
	}
};

void HIRManipulations::coalesce_basic_blocks(Method* M)
{
	LOG("Coealescing Starting" << nl);
	CoalesceBasicBlocksOperation().coalesce_if_possible(M->get_init_bb());
	LOG("Coealescing Finished" << nl);
}

void HIRManipulations::replace_value_without_source_states(Value* old_value, Value* new_value)
{
	LOG3("HIRManipulations::replace_value_without_source_states(this=" << old_value << ",v="
	                                                                  << new_value << ")" << nl);
	std::list<Value*> replace_for;
	for (auto it = old_value->user_begin(); it != old_value->user_end(); it++) {
		auto I = *it;
		assert(I);
		if (!I->to_SourceStateInst()) {
			replace_for.push_back(I);
		}
	}
	for (auto it = replace_for.begin(); it != replace_for.end(); it++) {
		auto I = (*it)->to_Instruction();
		LOG3("replacing value " << old_value << " with " << new_value << " in " << I << nl);
		I->replace_op(old_value, new_value);
	}
}

void HIRManipulations::force_set_beginInst(Instruction* I, BeginInst* new_bb){
	I->begin = new_bb;
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
