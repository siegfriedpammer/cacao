/* src/vm/jit/compiler2/ExamplePass.cpp - ExamplePass

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

#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"

#include <boost/dynamic_bitset.hpp>

#include "toolbox/logging.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"

#include "vm/jit/compiler2/alloc/map.hpp"

#define DEBUG_NAME "compiler2/NewLivetimeAnalysisPass"

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
} // namespace

/**
 * Performs a postorder traversal while ignoring loop-back edges
 */
void NewLivetimeAnalysisPass::dag_dfs(MachineBasicBlock* block)
{
	auto loop_tree = get_Pass<MachineLoopPass>();
	for (auto i = get_successor_begin(block), e = get_successor_end(block); i != e; ++i) {
		auto succ = *i;
		// Only visit the successor if it is unproccessed and not a loop-back
		// edge
		if (!succ->is_processed() && !loop_tree->is_backedge(block, succ)) {
			dag_dfs(succ);
		}
	}

	LOG2(nl << "Processing " << *block << nl);

	boost::dynamic_bitset<> live = phi_uses(block);

	for (auto i = get_successor_begin(block), e = get_successor_end(block); i != e; ++i) {
		auto successor = *i;
		if (!loop_tree->is_backedge(block, successor)) {
			// Live = Live u (LiveIn(S) \ PhiDefs(S))
			LiveTy phi_def = phi_defs(successor);
			LiveTy live_in(
			    get_live_in(successor)); // Use copy constructor so we can safely modify it

			live_in -= phi_def;
			live |= live_in;
		}
	}
	LiveTy& live_out = get_live_out(block);
	live_out = live;

	LOG2("LiveOut(" << *block << "): ");
	DEBUG2(log_livety(dbg(), live_out));
	LOG2(nl);

	// MachineInstructions in "block" in reverse order
	for (auto i = block->rbegin(), e = block->rend(); i != e; ++i) {
		auto instruction = *i;

		// phis are handled separately
		if (instruction->is_phi())
			continue;

		// Remove operands defined in this instruction from live
		live[instruction->get_result().op->get_id()] = false;

		// Add used operands in this instruction to live (also for dummy
		// operands)
		auto lambda_used_op = [&](auto op_desc) {
			if (!op_desc.op->needs_allocation())
				return;

			live[op_desc.op->get_id()] = true;
#if !defined(NDEBUG)
			operandMap[op_desc.op->get_id()] = op_desc.op;
#endif
		};

		std::for_each(instruction->begin(), instruction->end(), lambda_used_op);
		std::for_each(instruction->dummy_begin(), instruction->dummy_end(), lambda_used_op);
	}

	// LiveIn(B) = Live u PhiDefs(B)
	LiveTy phi_def = phi_defs(block);
	LiveTy& live_in = get_live_in(block);
	live_in = (live | phi_def);

	LOG2("LiveIn(" << *block << "): ");
	DEBUG2(log_livety(dbg(), live_in));
	LOG2(nl);

	block->mark_processed();
}

boost::dynamic_bitset<> NewLivetimeAnalysisPass::phi_uses(MachineBasicBlock* block)
{
	boost::dynamic_bitset<> phi_uses(num_operands);

	std::for_each(get_successor_begin(block), get_successor_end(block), [&](auto successor) {
		std::for_each(successor->phi_begin(), successor->phi_end(), [&](auto phi_instr) {
			auto idx = successor->get_predecessor_index(block);
			auto operand = phi_instr->get(idx).op;
			phi_uses[operand->get_id()] = true;

			LOG3("\tphi operand " << *operand << " is used in " << *block << " (phi instr in "
			                      << *successor << ")" << nl);
#if !defined(NDEBUG)
			operandMap[operand->get_id()] = operand;
#endif
		});
	});

	return phi_uses;
}

boost::dynamic_bitset<> NewLivetimeAnalysisPass::phi_defs(MachineBasicBlock* block)
{
	boost::dynamic_bitset<> phi_defs(num_operands);

	std::for_each(block->phi_begin(), block->phi_end(), [&](auto phi_instr) {
		auto operand = phi_instr->get_result().op;
		phi_defs[operand->get_id()] = true;

		LOG3("\tphi operand " << *operand << " is defined in " << *block << nl);
#if !defined(NDEBUG)
		operandMap[operand->get_id()] = operand;
#endif
	});

	return phi_defs;
}

#if !defined(NDEBUG)

void NewLivetimeAnalysisPass::log_livety(OStream& OS, const LiveTy& liveSet)
{
	bool loggedElement = false;

	OS << "[";
	for (std::size_t i = 0; i < liveSet.size(); ++i) {
		auto operand = operandMap[i];

		if (liveSet[i]) {
			if (loggedElement)
				OS << ", ";
			OS << *operand;
			loggedElement = true;
		}
	}
	OS << "]";
}

#endif

void NewLivetimeAnalysisPass::looptree_dfs(MachineBasicBlock* header)
{
	// Since this method is called for all children of a loop, we only really
	// care about loop headers
	MachineLoopPass* ML = get_Pass<MachineLoopPass>();
	if (!ML->is_loop_header(header))
		return;

	LOG2(nl << "Processing loop header: " << *header << nl);

	// LiveLoop = LiveIn(Head) \ PhiDefs(Head)
	LiveTy live_loop = (get_live_in(header) - phi_defs(header));

	auto loop = ML->get_Loop(header);
	for (auto i = loop->child_begin(), e = loop->child_end(); i != e; ++i) {
		if (*i == header)
			continue;

		auto child = *i;
		LiveTy& live_in = get_live_in(child);
		live_in |= live_loop;

		LiveTy& live_out = get_live_out(child);
		live_out |= live_loop;

		LOG2("LiveIn(" << *child << "): ");
		DEBUG2(log_livety(dbg(), live_in));
		LOG2(nl);

		LOG2("LiveOut(" << *child << "): ");
		DEBUG2(log_livety(dbg(), live_out));
		LOG2(nl);

		looptree_dfs(child);
	}
}

bool NewLivetimeAnalysisPass::run(JITData& JD)
{
	LOG(nl << BoldYellow << "Running NewLivetimeAnalysisPass" << reset_color << nl);
	num_operands = MachineOperand::get_id_counter();
	LOG2("Initializing bitsets with size " << num_operands << " (number of operands)" << nl);

	MachineInstructionSchedulingPass* MIS = get_Pass<MachineInstructionSchedulingPass>();
	MachineLoopPass* ML = get_Pass<MachineLoopPass>();

	// Reset all basic blocks
	for (auto BB : *MIS) {
		BB->mark_unprocessed();
	}
	LOG1("Running first phase: post-order traversel over CFG for initial live "
	     "sets"
	     << nl);
	dag_dfs(MIS->front());
	LOG1(nl << "Running second phase: Propagate liveness from loop-headers "
	           "down to all BB within loops"
	        << nl);
	for (auto i = ML->loop_begin(), e = ML->loop_end(); i != e; ++i) {
		looptree_dfs((*i)->get_header());
	}

	return true;
}

// pass usage
PassUsage& NewLivetimeAnalysisPass::get_PassUsage(PassUsage& PU) const
{
	PU.add_requires<MachineLoopPass>();
	PU.add_requires<MachineInstructionSchedulingPass>();
	return PU;
}

// register pass
static PassRegistry<NewLivetimeAnalysisPass> X("NewLivetimeAnalysisPass");

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
