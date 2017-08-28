/* src/vm/jit/compiler2/lsra/NextUse.cpp - NextUse

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

#include "vm/jit/compiler2/lsra/NextUse.hpp"

#include <limits>

#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/lsra/SpillPass.hpp"

#define DEBUG_NAME "compiler2/SpillPass/NextUse"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

MachineInstruction::successor_iterator get_successor_begin(MachineBasicBlock* MBB)
{
	return MBB->back()->successor_begin();
}

MachineInstruction::successor_iterator get_successor_end(MachineBasicBlock* MBB)
{
	return MBB->back()->successor_end();
}

} // end namespace

void NextUse::initialize(SpillPass* sp)
{
	SP = sp;
	auto MIS = sp->get_Pass<MachineInstructionSchedulingPass>();
	for (const auto& BB : *MIS) {
		std::size_t step = 0;
		for (auto& instruction : *BB) {
			instruction->set_step(step++);
		}

		for (auto i = BB->phi_begin(), e = BB->phi_end(); i != e; ++i) {
			auto phi_instruction = *i;
			phi_instruction->set_step(0);
		}
	}
}

/**
 * Returns the use for the given definition in the given block if it
 * exists, else creates it.
 */
UseTime* NextUse::get_or_set_use_block(MachineBasicBlock* block, MachineOperand* operand)
{
	BlockOperand key{block, operand};
	auto search_result = next_use_map.find(key);
	UseTime* result = nullptr;

	UseTime tmp;
	tmp.block = block;
	tmp.operand = operand;
	if (search_result == next_use_map.end()) {
		tmp.next_use = USES_INFINITY;
		tmp.outermost_loop = UNKNOWN_OUTERMOST_LOOP;
		tmp.visited = 0;
		result = &(next_use_map.insert(std::make_pair(key, tmp)).first->second);
	}

	if (result->outermost_loop == UNKNOWN_OUTERMOST_LOOP && result->visited < visited_counter) {
		result->visited = visited_counter;
		NextUseTime next_use = get_next_use_(block, operand);
		if (next_use.outermost_loop != UNKNOWN_OUTERMOST_LOOP) {
			result->next_use = next_use.time;
			result->outermost_loop = next_use.outermost_loop;
			LOG3("\t\tSetting nextuse of " << *operand << " in block " << *block << " to "
			                               << result->next_use << " (outermostloop "
			                               << result->outermost_loop << ")\n");
		}
	}

	return result;
}

/**
 * Check if a value of the given definition is used in the given block
 * as a phi argument.
 */
static bool is_phi_argument(MachineBasicBlock* block, MachineOperand* operand)
{
	for (const auto& phi_instruction : *block) {
		for (const auto& phi_op : *phi_instruction) {
			if (phi_op.op->aquivalent(*operand)) {
				return true;
			}
		}
	}
	return false;
}

NextUseTime NextUse::get_next_use(MachineBasicBlock* from, MachineOperand* def)
{
	++visited_counter;
	return get_next_use_(from, def);
}

NextUseTime NextUse::get_next_use_(MachineBasicBlock* from, MachineOperand* defined_value)
{
	MachineInstruction* next_use_instr = nullptr;
	auto next_use_step = std::numeric_limits<std::size_t>::max();
	auto timestep = from->front()->get_step();

	for (const auto& instruction : *from) {
		for (const auto& op : *instruction) {
			if (defined_value->aquivalent(*op.op)) {
				next_use_instr = instruction;
				next_use_step = instruction->get_step();
				break;
			}
		}

		if (next_use_instr)
			break;
	}

	auto MLP = SP->get_Pass<MachineLoopPass>();

	if (next_use_instr) {
		NextUseTime result;
		result.time = next_use_step - timestep;
		result.outermost_loop = MLP->loop_nest(MLP->get_Loop(from)) + 1;
		result.before = next_use_instr;
		return result;
	}

	auto last_instruction = from->back();
	unsigned step = last_instruction->get_step() + 1 + timestep;

	if (is_phi_argument(from, defined_value)) {
		NextUseTime result;
		result.time = step;
		result.outermost_loop = MLP->loop_nest(MLP->get_Loop(from)) + 1;
		result.before = from->front();
		return result;
	}

	auto LTA = SP->get_Pass<NewLivetimeAnalysisPass>();

	NextUseTime result;
	result.before = nullptr;
	auto loop = MLP->get_Loop(from);
	auto loopdepth = MLP->loop_nest(loop) + 1;
	bool found_visited = false;
	bool found_use = false;
	auto outermost_loop = loopdepth;
	unsigned next_use = USES_INFINITY;
	for (auto i = get_successor_begin(from), e = get_successor_end(from); i != e; ++i) {
		auto successor = *i;
		LOG3("\t\tChecking successor of block " << *from << ": " << *successor << " (for use of "
		                                        << *defined_value << ")\n");

		if (!LTA->is_live_in(successor, defined_value)) {
			LOG3("\t\t\t not live in\n");
			continue;
		}

		auto use = get_or_set_use_block(successor, defined_value);
		LOG3("\t\tFound " << use->next_use << " (loopdepth " << use->outermost_loop
		                  << ") (we're in block " << *from << ")\n");
		if (uses_is_infinite(use->next_use)) {
			if (use->outermost_loop == UNKNOWN_OUTERMOST_LOOP) {
				found_visited = true;
			}
			continue;
		}

		found_use = true;
		unsigned use_dist = use->next_use;

		auto succ_depth = MLP->loop_nest(MLP->get_Loop(successor)) + 1;
		if (succ_depth < loopdepth) {
			unsigned factor = (loopdepth - succ_depth) * 5000;
			LOG3("\t\tIncrease usestep because of loop out edge " << factor << nl);
			use_dist += factor;
		}

		if (use_dist < next_use) {
			next_use = use_dist;
			outermost_loop = use->outermost_loop;
			// result.before = use->operand_desc->get_MachineInstruction();
		}
	}

	if (loopdepth < outermost_loop) {
		outermost_loop = loopdepth;
	}

	result.time = next_use + step;
	result.outermost_loop = outermost_loop;

	if (!found_use && found_visited) {
		result.outermost_loop = UNKNOWN_OUTERMOST_LOOP;
	}

	LOG3("\tResult: " << result.time << " (outerloop: " << result.outermost_loop << ")\n");
	return result;
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
