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

Available
SpillPass::available_in_all_preds(MachineBasicBlock* block,
                                  const std::map<MachineBasicBlock*, const Workset*>& pred_worksets,
                                  MachineInstruction* instruction,
                                  bool is_phi)
{
	assert(pred_worksets.size() > 0);

	bool available_everywhere = true;
	bool available_nowhere = true;

	for (const auto& pred_workset : pred_worksets) {
		MachineOperandDesc* operand;
		if (is_phi) {
			auto idx = block->get_predecessor_index(pred_workset.first);
			auto phi_instr = instruction->to_MachinePhiInst();
			operand = &phi_instr->get(idx);
		}
		else {
			// TODO: If its not a phi, instruction should really be a used MO passed in
		}

		bool found = false;
		for (const auto& location : *pred_workset.second) {
			if (!operand->op->aquivalent(*location.op_desc->op)) {
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

#define USES_INFINITY 10000000

static bool uses_is_infinite(unsigned time)
{
	return time >= USES_INFINITY;
}

Location SpillPass::to_take_or_not_to_take(MachineBasicBlock* block,
                                           MachineInstruction* instruction,
                                           Available available)
{
	Location location;
	location.time = USES_INFINITY;
	location.instruction = instruction;
	location.spilled = false;

	// TODO: Implement architecture depending "dont spill" flag

	NextUseTime next_use_time = next_use.get_next_use(block, instruction);
	if (uses_is_infinite(next_use_time.time)) {
		// operands marked as live in should not be dead, so it must be a phi
		assert(instruction->to_MachinePhiInst());
		location.time = USES_INFINITY;
		LOG3("\t" << *instruction << " not taken (dead)\n");
		return location;
	}
}

void SpillPass::decide_start_workset(MachineBasicBlock* block)
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
			pred_worksets[predecessor] = &iter->second.end();
		}
	}

	std::vector<Location> starters;
	std::vector<Location> delayed;

	LOG3("Living at the start of block [" << *block << "]\n");

	// do PHis first
	for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
		auto phi = *i;

		Available available = Available::Unknown;
		if (all_preds_known) {
			available = available_in_all_preds(block, pred_worksets, phi, true);
		}

		Location location = to_take_or_not_to_take(block, phi, available);
	}
}

bool SpillPass::run(JITData& JD)
{
	LOG(nl << BoldYellow << "Running SpillPass" << reset_color << nl);
	next_use.initialize(this);
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
			workset = std::make_unique<Workset>(pred_info.end());
		}
		else {
			// multiple predecessors, calculate the start workset
			decide_start_workset(block);
			ABORT_MSG("decide_start_workset not implemented", "");
		}
	}

	ABORT_MSG("This is the end.", "Do not continue");
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
