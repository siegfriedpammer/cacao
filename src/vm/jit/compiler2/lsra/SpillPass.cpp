/* src/vm/jit/compiler2/lsra/SpillPass.cpp - SpillPass

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

#include "vm/jit/compiler2/lsra/SpillPass.hpp"

#include "toolbox/logging.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/MachineInstruction.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"

#define DEBUG_NAME "compiler2/SpillPass"

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

const unsigned kNRegs = 40; //< Number of avail. registers TODO: Get this from Backend

} // end namespace

void SpillPass::build_reverse_postorder(MachineBasicBlock* block)
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

/**
 * Checks whether an operand is available in all its predecessors.
 * If its a PHI instruction, the operand param is ignored and the correct
 * phi operand is looked up and used for each corresponding predecessor
 */
Available
SpillPass::available_in_all_preds(MachineBasicBlock* block,
                                  const std::map<MachineBasicBlock*, const Workset*>& pred_worksets,
                                  MachineInstruction* instruction,
                                  MachineOperand* op,
                                  bool is_phi)
{
	assert(pred_worksets.size() > 0);

	bool available_everywhere = true;
	bool available_nowhere = true;
	MachineOperand* operand = nullptr;

	for (const auto& pred_workset : pred_worksets) {
		if (is_phi) {
			auto idx = block->get_predecessor_index(pred_workset.first);
			auto phi_instr = instruction->to_MachinePhiInst();
			operand = phi_instr->get(idx).op;
		}
		else {
			operand = op;
		}

		bool found = false;
		for (const auto& location : *pred_workset.second) {
			if (!operand->aquivalent(*location.operand)) {
				continue;
			}

			found = true;
			available_nowhere = false;
			break;
		}

		available_everywhere &= found;
	}

	if (available_everywhere) {
		assert(!available_nowhere);
		return Available::Everywhere;
	}
	else if (available_nowhere) {
		return Available::Nowhere;
	}

	return Available::Partly;
}

Location SpillPass::to_take_or_not_to_take(MachineBasicBlock* block,
                                           MachineOperand* operand,
                                           bool is_phi_result,
                                           Available available)
{
	Location location;
	location.time = USES_INFINITY;
	location.operand = operand;
	location.spilled = false;

	// TODO: Implement architecture depending "dont spill" flag

	NextUseTime next_use_time = next_use.get_next_use(block, operand);
	if (uses_is_infinite(next_use_time.time)) {
		// operands marked as live in should not be dead, so it must be a phi
		assert(is_phi_result);
		location.time = USES_INFINITY;
		LOG3("\t" << *operand << " not taken (dead)\n");
		return location;
	}

	location.time = next_use_time.time;
	if (available == Available::Everywhere) {
		LOG3("\t" << *operand << " taken (" << location.time << ", live in all preds)\n");
		return location;
	}
	else if (available == Available::Nowhere) {
		LOG3("\t" << *operand << " not taken (" << location.time << ", live in no pred)\n");
		location.time = USES_INFINITY;
		return location;
	}

	auto MLP = get_Pass<MachineLoopPass>();
	auto loopdepth = MLP->loop_nest(MLP->get_Loop(block)) + 1;
	if ((signed)next_use_time.outermost_loop >= loopdepth) {
		LOG3("\t" << *operand << " taken (" << location.time << ", loop "
		          << next_use_time.outermost_loop << ")\n");
	}
	else {
		location.time = USES_PENDING;
		LOG3("\t" << *operand << " delayed (outerdepth " << next_use_time.outermost_loop
		          << " < loopdeth " << loopdepth << ")\n");
	}

	return location;
}

WorksetUPtrTy SpillPass::decide_start_workset(MachineBasicBlock* block)
{
	std::map<MachineBasicBlock*, const Workset*> pred_worksets;
	bool all_preds_known = true;

	for (auto i = block->pred_begin(), e = block->pred_end(); i != e; ++i) {
		auto predecessor = *i;
		auto iter = block_infos.find(predecessor);
		if (iter == block_infos.end()) {
			all_preds_known = false;
		}
		else {
			pred_worksets[predecessor] = iter->second.end_workset.get();
		}
	}

	std::vector<Location> starters;
	std::vector<Location> delayed;

	LOG3("Living at the start of block [" << *block << "]\n");

	// do PHis first
	std::vector<MachineOperand*> phi_results;
	for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
		auto phi = *i;

		Available available = Available::Unknown;
		if (all_preds_known) {
			available = available_in_all_preds(block, pred_worksets, phi, nullptr, true);
		}

		Location location = to_take_or_not_to_take(block, phi->get_result().op, true, available);
		if (!uses_is_infinite(location.time)) {
			if (uses_is_pending(location.time)) {
				delayed.push_back(location);
			}
			else {
				starters.push_back(location);
			}
		}
		else {
			// TODO: Spill phi
			assert(false && "Spill phi not implemented!");
		}

		phi_results.push_back(phi->get_result().op);
	}

	// check all live-ins
	auto LTA = get_Pass<NewLivetimeAnalysisPass>();
	LTA->for_each_live_in(block, [&](auto operand) {
		// Phi definitions are also in live_in but we do not handle them again
		for (const auto& phi_result : phi_results) {
			if (phi_result->aquivalent(*operand)) {
				LOG3("Operand " << *operand << " is a phi definition (skipping).\n");
				return;
			}
		}

		LOG3("Operand " << *operand << " is live in " << *block << nl);

		Available available = Available::Unknown;
		if (all_preds_known) {
			available = this->available_in_all_preds(block, pred_worksets, nullptr, operand, false);
		}

		Location location = this->to_take_or_not_to_take(block, operand, false, available);
		if (!uses_is_infinite(location.time)) {
			if (uses_is_pending(location.time)) {
				delayed.push_back(location);
			}
			else {
				starters.push_back(location);
			}
		}
	});

	auto MLP = get_Pass<MachineLoopPass>();
	auto loop = MLP->get_Loop(block);
	assert(loop && "Wtf am i going to do now???");
	unsigned pressure = loop->get_register_pressure();
	assert(delayed.size() <= pressure);

	int free_slots = kNRegs - starters.size();
	int free_pressure_slots = kNRegs - (pressure - delayed.size());
	free_slots = std::min(free_slots, free_pressure_slots);

	// so far we only put values into the starters list taht are used inside
	// the loop. If register pressure in the loop is low then we can take some
	// values and let them live through the loop
	LOG2("Loop pressure " << pressure << ", taking " << free_slots << " delayed vals\n");
	if (free_slots > 0) {
		std::sort(delayed.begin(), delayed.end());

		for (const auto& location : delayed) {
			if (free_slots <= 0) {
				break;
			}

			LOG3("    " << *location.operand << " location");
		}

		if (delayed.size() > 0)
			assert(false && "Delayed values in free registers not implemented!");
	}

	// Spill phis (the actual phis not just their values) that are in this
	// block but not in the start workset
	for (const auto& location : delayed) {
		assert(false && "Spilling delayed PHIs not implemented!");
	}

	std::sort(starters.begin(), starters.end());

	// Copy best starters to start workset
	unsigned workset_count = std::min<unsigned>(starters.size(), kNRegs);
	auto workset = std::make_unique<Workset>();
	std::copy_n(starters.begin(), workset_count, std::back_inserter(*workset));

	// Spill phis (the actual phis not just their values) that are in this
	// block but not in the start workset
	for (unsigned i = workset_count, len = starters.size(); i < len; ++i) {
		assert(false && "Spilling of starter PHIs not implemented!");
	}

	// Determine spill status of the values: If there is 1 predecessor block
	// (which is no backedge) where the value is spilled then we must set it to
	// spilled here
	for (auto& location : *workset) {
		// Phis from this block are not spilled
		auto instr = location.operand->get_defining_instruction();
		if (instr && *instr->get_block() == *block) {
			assert(instr->to_MachinePhiInst());
			location.spilled = false;
			continue;
		}

		// determine if value was spilled on any predecessor
		assert(false && "Not implemented!");
	}

	return workset;
}

#define TIME_UNDEFINED 6666

// TODO: Workset really needs to be its own class and this a member method
static void workset_insert(Workset& workset, MachineOperand* operand, bool spilled)
{
	// If it is already in the set only update spilled flag
	for (auto& location : workset) {
		if (location.operand->aquivalent(*operand)) {
			if (spilled) {
				location.spilled = true;
			}
			return;
		}
	}

	Location location;
	location.operand = operand;
	location.spilled = spilled;
	location.time = TIME_UNDEFINED;

	workset.push_back(location);
}

// TODO: Workset really needs to be its own class and this a member method
static Location* workset_contains(Workset& workset, MachineOperand* operand)
{
	for (auto& location : workset) {
		if (location.operand->aquivalent(*operand)) {
			return &location;
		}
	}
	return nullptr;
}

// TODO: Workset really needs to be its own class and this a member method
static void workset_remove(Workset& workset, MachineOperand* operand)
{
	for (auto i = workset.begin(), e = workset.end(); i != e; ++i) {
		auto location = *i;
		if (location.operand->aquivalent(*operand)) {
			workset.erase(i);
			return;
		}
	}
}

static void log_workset(const Workset& workset)
{
	for (const auto& location : workset) {
		LOG3("   Location operand: " << *location.operand << nl);
	}
}

/**
 * Performs the actions necessary to grant the request that:
 * - new_vals can be held in registers
 * - as few as possible other values are disposed
 * - the worst values get disposed
 *
 * is_usage indicates that the values are used (not defined)
 * In this case reloads must be performed
 */
void SpillPass::displace(Workset& workset,
                         const Workset& new_vals,
                         bool const is_usage,
                         MachineInstruction* instruction)
{
	std::vector<MachineOperand*> to_insert(kNRegs, nullptr);
	std::vector<bool> spilled(kNRegs, false);

	// 1. Identify the number of needed slots and the values to reload
	unsigned demand = 0;

	for (const auto& location : new_vals) {
		auto operand = location.operand;
		bool reloaded = false;

		if (workset_contains(workset, operand) == nullptr) {
			LOG3("\t\tinsert " << *operand << nl);
			if (is_usage) {
				LOG3("\tReload " << *operand << " before " << *instruction << nl);
				// TODO: Add reload of value before the given instruction
				reloaded = true;
			}
		}
		else {
			LOG3("\t\t" << *operand << " already in workset\n");
			// TODO: Can we really get rid of this assert? 
			// assert(is_usage);

			// Remove the value from the current workset so it is not accidently spilled
			workset_remove(workset, operand);
		}
		spilled[demand] = reloaded;
		to_insert[demand] = location.operand;
		++demand;
	}

	// 2. make room for at least 'demand' slots
	unsigned len = workset.size();
	int spills_needed = len + demand - kNRegs;

	if (spills_needed > 0) {
		// TODO: Implement spilling
		assert(false && "Spilling not implemented in displace!!");
	}

	// 3. Insert the new values into the workset
	for (unsigned i = 0; i < demand; ++i) {
		workset_insert(workset, to_insert[i], spilled[i]);
	}
}

bool SpillPass::run(JITData& JD)
{
	LOG(nl << BoldYellow << "Running SpillPass" << reset_color << nl);
	next_use.initialize(this);
	loop_pressure.initialize(this);
	auto MIS = get_Pass<MachineInstructionSchedulingPass>();

	build_reverse_postorder(MIS->front());
	for (auto block : reverse_postorder) {
		LOG3("Processing block " << *block << nl);

		WorksetUPtrTy workset;

		int arity = block->pred_size();
		if (arity == 0) {
			// No predecessor -> empty set
			workset = std::make_unique<Workset>();
		}
		else if (arity == 1) {
			// one predecessor, copy its end workset
			auto& pred_info = block_infos[*block->pred_begin()];
			workset = std::make_unique<Workset>(*pred_info.end_workset);
		}
		else {
			// multiple predecessors, calculate the start workset
			workset = decide_start_workset(block);
		}

		if (DEBUG_COND_N(3)) {
			LOG3("\nStart workset for " << *block << ":\n");
			for (const auto& location : *workset) {
				LOG3("  " << *location.operand << " (" << location.time << ")\n");
			}
			LOG3(nl);
		}

		BlockInfo block_info;
		block_info.start_workset = std::make_unique<Workset>(*workset);

		for (const auto& instruction : *block) {
			// allocate all values _used_ by this instruction
			Workset new_uses;
			for (auto& operand_desc : *instruction) {
				if (reg_alloc_considers_operand(*operand_desc.op)) {
					workset_insert(new_uses, operand_desc.op, false);
				}
			}
			displace(*workset, new_uses, true, instruction);

			// allocate the value _defined_ by this instruction
			Workset new_def;
			auto& operand_desc = instruction->get_result();
			if (reg_alloc_considers_operand(*operand_desc.op)) {
				workset_insert(new_def, operand_desc.op, false);
			}
			displace(*workset, new_def, false, instruction);
		}

		// Remember end-workset for this block
		block_info.end_workset = std::make_unique<Workset>(*workset);
		if (DEBUG_COND_N(3)) {
			LOG3("\nEnd workset for " << *block << ":\n");
			for (const auto& location : *workset) {
				LOG3("  " << *location.operand << " (" << location.time << ")\n");
			}
			LOG3(nl);
		}

		block_infos[block] = std::move(block_info);
	}

	return true;
}

PassUsage& SpillPass::get_PassUsage(PassUsage& PU) const
{
	PU.add_requires<MachineInstructionSchedulingPass>();
	PU.add_requires<MachineLoopPass>();
	PU.add_requires<NewLivetimeAnalysisPass>();
	return PU;
}

// register pass
static PassRegistry<SpillPass> X("SpillPass");

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
