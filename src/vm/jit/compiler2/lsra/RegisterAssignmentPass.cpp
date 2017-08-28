/* src/vm/jit/compiler2/lsra/RegisterAssignmentPass.cpp - RegisterAssignmentPass

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

#include "vm/jit/compiler2/lsra/RegisterAssignmentPass.hpp"

#include "vm/jit/compiler2/MachineDominatorPass.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/lsra/SpillPass.hpp"

#define DEBUG_NAME "compiler2/RegisterAssignment"

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

void RegisterAssignmentPass::add_interval(MachineOperand* operand,
                                          unsigned step,
                                          bool def,
                                          bool real)
{
	LivetimeInterval interval{operand, step, def, real};

	assert(current_list);
	current_list->push_front(interval);

	LOG3("\t\t" << (def ? "def" : "use") << " adding " << *operand << ", step = " << step << nl);
}

void RegisterAssignmentPass::create_borders(MachineBasicBlock* block)
{
	update_current_list(block);
	unsigned step = 0;

	auto LTA = get_Pass<NewLivetimeAnalysisPass>();
	auto live = LTA->get_live_out(block);

	// Make final uses of all values live out of the block.
	// They are necessary to build up real intervals.
	LTA->for_each_live_out(block, [&](const auto& operand) {
		LOG3("\tMaking live: " << *operand << nl);
		this->add_use(operand, step, false);
	});
	++step;

	// Determine the last uses of a value inside the block, since
	// they are relevant for the interval
	auto handle_instruction([&](const auto& instruction) {
		LOG2("\tInstruction: " << *instruction << nl);
		if (!reg_alloc_considers_instruction(instruction)) {
			LOG2("\t\tskipped!\n");
			return;
		}

		auto result_operand = instruction->get_result().op;
		if (reg_alloc_considers_operand(*result_operand)) {
			live[result_operand->get_id()] = false;
			this->add_def(result_operand, step, 1);
		}

		if (!instruction->to_MachinePhiInst()) {
			auto use_lambda = [&](const auto& op_desc) {
				auto operand = op_desc.op;
				if (!reg_alloc_considers_operand(*operand))
					return;
				char msg = '-';

				if (!live[operand->get_id()]) {
					live[operand->get_id()] = true;
					this->add_use(operand, step, true);
					msg = 'X';
				}

				LOG3("\t\t" << msg << " use: " << *operand << nl);
			};

			std::for_each(instruction->begin(), instruction->end(), use_lambda);
			std::for_each(instruction->dummy_begin(), instruction->dummy_end(), use_lambda);
		}
		++step;
	});

	std::for_each(block->rbegin(), --block->rend(), handle_instruction);
	std::for_each(block->phi_begin(), block->phi_end(), handle_instruction);

	// Process live-ins last. In particular all phis are handled before, so
	// when iterating the intervals the live-ins come before all Phis
	LTA->for_each(live, [&](const auto& operand) { this->add_def(operand, step, false); });
}

void RegisterAssignmentPass::assign(MachineBasicBlock* block)
{
	LOG1(nl << Yellow << "Assigning colors for block " << *block << reset_color << nl);
	LOG2("\tusedef chain for block\n");
	if (DEBUG_COND_N(2)) {
		for (const auto& interval : interval_map[block]) {
			LOG2("\t" << (interval.is_def ? "def " : "use ") << *interval.operand << nl);
		}
	}

	// Mind that the sequence of defs from back to front defines a perfect
	// elimination order. So, coloring the definitions from first to last will work

}

void RegisterAssignmentPass::build_reverse_postorder(MachineBasicBlock* block)
{
	auto loop_tree = get_Pass<MachineLoopPass>();
	for (auto i = get_successor_begin(block), e = get_successor_end(block); i != e; ++i) {
		auto succ = *i;
		// Only visit the successor if it is unproccessed and not a loop-back edge
		if (!loop_tree->is_backedge(block, succ)) {
			build_reverse_postorder(succ);
		}
	}

	reverse_postorder.push_front(block);
}

bool RegisterAssignmentPass::run(JITData& JD)
{
	LOG(nl << BoldYellow << "Running RegisterAssignmentPass" << reset_color << nl);
	auto MIS = get_Pass<MachineInstructionSchedulingPass>();
	auto MDT = get_Pass<MachineDominatorPass>();
	auto LTA = get_Pass<NewLivetimeAnalysisPass>();
	build_reverse_postorder(MIS->front());
	variables_size = LTA->get_num_operands();
/*
	for (auto& block : reverse_postorder) {
		create_borders(block);
	}

	for (auto& block : reverse_postorder) {
		assign(block);
	}
*/
	// Initialize available registers for assignment
	backend = JD.get_Backend();
	OperandFile op_file;
	backend->get_OperandFile(op_file, new VirtualRegister(Type::IntTypeID));
	colors_size = op_file.size();
	assignable_colors = std::make_unique<AssignableColors>(colors_size);
	unsigned idx = 0;
	for (auto& operand : op_file) {
		assignable_colors->set_operand(idx++, operand);
	}

	for (auto& block : reverse_postorder) {
		auto idom = MDT->get_idominator(block);
		auto& allocated_variables = create_for(block, idom);

		LOG1(Yellow << "\nProcessing block " << *block);
		if (idom)
			LOG1(" (immediate dominator = " << *idom << ")");
		LOG1(reset_color << nl);

		// Remove all allocated variables that are not in the live in set of this block
		auto& block_live_in = LTA->get_live_in(block);
		allocated_variables.intersect(block_live_in);

		LOG2("LiveIn(" << *block << "): ");
		DEBUG2(LTA->log_livety(dbg(), block_live_in));
		LOG2(nl);

		// Get available colors
		boost::dynamic_bitset<> available_colors(colors_size, 0ul);
		available_colors.set();
		available_colors -= allocated_variables.get_colors();

		// Reverse instruction iteration to calculate live out sets for each instruction
		std::list<std::pair<MachineInstruction*, LiveTy>> live_outs;
		LiveTy live_out = LTA->get_live_out(block);
		for (auto i = block->rbegin(), e = --block->rend(); i != e; ++i) {
			auto instruction = *i;
			auto pair = std::make_pair(instruction, live_out);
			live_outs.push_front(pair);

			live_out = LTA->liveness_transfer(live_out, instruction);
		}

		// Add PHIs at the front (???)
		for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
			auto instruction = *i;
			auto pair = std::make_pair(instruction, live_out);
			live_outs.push_front(pair);
		}

		// Forward iteration over instructions + live_out data
		for (const auto& pair : live_outs) {
			auto instruction = pair.first;
			auto& live_out = pair.second;

			if (!reg_alloc_considers_instruction(instruction))
				continue;

			LOG2(Magenta << "\tProcessing instruction: " << *instruction << reset_color << nl);
			LOG2("\t                        LiveOut: ");
			DEBUG2(LTA->log_livety(dbg(), live_out));
			LOG2(nl);

			if (!instruction->to_MachinePhiInst()) {

				auto uses_lambda = [&](const auto& op_desc) {
					auto operand = op_desc.op;
					if (!reg_alloc_considers_operand(*operand))
						return;

					if (live_out[operand->get_id()])
						return;

					auto assigned_color = allocated_variables.get_color(operand);

					LOG1("\t\tMaking " << *assignable_colors->get(assigned_color) << " available.\n");

					available_colors[assigned_color] = true;
					allocated_variables.remove_variable(operand->get_id());
				};

				std::for_each(instruction->begin(), instruction->end(), uses_lambda);
				std::for_each(instruction->dummy_begin(), instruction->dummy_end(), uses_lambda);
			}

			auto result_operand = instruction->get_result().op;
			if (!reg_alloc_considers_operand(*result_operand))
				continue;

			assert(available_colors.any() && "No more free colors available. SpillPass messed up!");
			auto color = available_colors.find_first();
			allocated_variables.set_color(result_operand->get_id(), color);
			available_colors[color] = false;

			LOG1("\t\tAssigning " << *result_operand << " = " << *assignable_colors->get(color) << nl);

			if (!live_out[result_operand->get_id()]) {
				LOG1("\t\tReleasing " << *result_operand << " since it is dead after this.\n");
				available_colors[color] = true;
				allocated_variables.remove_variable(result_operand->get_id());
			}
		}
	}

	for (const auto& block : reverse_postorder) {
		auto& vars = allocated_variables_map.at(block);
		for (const auto& instruction : *block) {
			for (auto& op_desc : *instruction) {
				if (!op_desc.op->is_virtual()) continue;
				op_desc.op = get(vars, op_desc.op);
			}

			if (instruction->get_result().op->is_virtual()) {
				instruction->get_result().op = get(vars, instruction->get_result().op);
			}
		}
	}

	return true;
}

unsigned AllocatedVariables::get_color(MachineOperand* operand) const
{
	LOG2("\t\tRequesting currently assigned color for " << *operand << nl);
	auto operand_id = operand->get_id();
	if (operand->is_virtual()) {
		// assert(variables[operand_id] && "Color for unallocated operand requested!");
		return colors[operand_id];
	}

	// Fixed operand
	//variables[operand_id] = true;
	//colors[operand_id] = operand_id;
	return colors[operand_id];
}

PassUsage& RegisterAssignmentPass::get_PassUsage(PassUsage& PU) const
{
	PU.add_requires<MachineDominatorPass>();
	PU.add_requires<MachineInstructionSchedulingPass>();
	PU.add_requires<MachineLoopPass>();
	PU.add_requires<NewLivetimeAnalysisPass>();
	PU.add_requires<SpillPass>();

	PU.add_modifies<MachineInstructionSchedulingPass>();
	return PU;
}

// register pass
static PassRegistry<RegisterAssignmentPass> X("RegisterAssignmentPass");

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
