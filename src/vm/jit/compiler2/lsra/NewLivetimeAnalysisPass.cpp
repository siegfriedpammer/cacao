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
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"
#include "vm/jit/compiler2/ReversePostOrderPass.hpp"
#include "vm/jit/compiler2/lsra/LogHelper.hpp"

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

/// Merges the set given in the second parameter into the set given in
/// the first parameter. If an operand is present in boths sets, the
/// minimum of both distances is used
/// Returns true if anything has changed in target
static bool merge(NextUseSet& target, const NextUseSet& src)
{
	bool change = false;

	for (const auto& operand : src) {
		if (!target.contains(operand)) {
			auto distance = src[operand];
			target.insert(operand, distance);
			change = true;
		}
		else {
			auto old_distance = target[operand];
			target[operand] = std::min(old_distance, src[operand]);

			if (old_distance != target[operand])
				change = true;
		}
	}

	return change;
}

/// Adds the given distance to all distances in the set
static void add_to_all(NextUseSet& set, unsigned distance)
{
	if (distance == 0)
		return;

	for (const auto& operand : set) {
		set[operand] += distance;
	}
}

} // namespace

void NextUseInformation::initialize_empty_sets_for(MachineBasicBlock* block)
{	
	next_use_ins.emplace(block, LTA.MOF->EmptyExtendedSet<unsigned>());
	next_use_outs.emplace(block, LTA.MOF->EmptyExtendedSet<unsigned>());
}

void NextUseInformation::add_operands(NextUseSet& set, OperandSet& live, unsigned distance)
{
	std::for_each(live.cbegin(), live.cend(), [&](const auto& operand) {
		set.insert_or_update(operand, distance);
	});
}

void NextUseInformation::copy_out_to_in(MachineBasicBlock* block)
{
	auto& next_use_in = *next_use_ins.at(block);
	auto& next_use_out = *next_use_outs.at(block);

	next_use_in.clear();
	next_use_in = next_use_out;
}

void NextUseInformation::transfer(NextUseSet& next_use_set,
                                  MachineInstruction* instruction,
                                  unsigned block_distance) const
{
	OperandSet updated_operands = LTA.MOF->EmptySet();

	auto defs_lambda = [&](const auto& descriptor) {
		if (!reg_alloc_considers_operand(*descriptor.op))
			return;

		next_use_set.remove(*descriptor.op);
		updated_operands.add(descriptor.op);
	};
	std::for_each(instruction->results_begin(), instruction->results_end(), defs_lambda);

	auto uses_lambda = [&](const auto& descriptor) {
		if (!reg_alloc_considers_operand(*descriptor.op))
			return;

		auto& operand = *descriptor.op;
		next_use_set.insert_or_update(operand, block_distance);
		updated_operands.add(&operand);
	};
	std::for_each(instruction->begin(), instruction->end(), uses_lambda);
	std::for_each(instruction->dummy_begin(), instruction->dummy_end(), uses_lambda);

	for (const auto& operand : next_use_set) {
		if (!updated_operands.contains(&operand)) {
			next_use_set[operand]++;
		}
	}
}

NextUseSet& NextUseInformation::get_next_use_out(MachineBasicBlock* block)
{
	assert_msg(block, "Do not pass nullptrs around!");
	assert_msg(next_use_outs.find(block) != next_use_outs.end(), "Requesting non existing NextUseSet for " << *block);
	return *next_use_outs.at(block);
}

NextUseSet& NextUseInformation::get_next_use_in(MachineBasicBlock* block)
{
	return *next_use_ins.at(block);
}

DefUseChains::DefUseChain& DefUseChains::get(const MachineOperand* operand)
{
	auto iter = chains.find(operand->get_id());
	if (iter == chains.end()) {
		auto chain = std::make_unique<DefUseChain>();
		chain->operand = operand;

		iter = chains.emplace(operand->get_id(), std::move(chain)).first;
	}
	return *iter->second;
}

const DefUseChains::DefUseChain& DefUseChains::get(const MachineOperand* operand) const
{
	auto iter = chains.find(operand->get_id());
	assert(iter != chains.end() && "Const version of 'get' assumes the chain is there!");
	return *iter->second;
}

void DefUseChains::add_definition(MachineOperandDesc* desc, const MIIterator& iter)
{
	auto& chain = get(desc->op);
	chain.definition = std::make_unique<Occurrence>(desc, iter);
}

void DefUseChains::add_use(MachineOperandDesc* desc, const MIIterator& iter)
{
	auto& chain = get(desc->op);
	chain.uses.emplace_back(desc, iter);
}

void DefUseChains::add_phi_definition(MachineOperandDesc* desc, MachinePhiInst* phi)
{
	auto& chain = get(desc->op);
	chain.definition = std::make_unique<Occurrence>(desc, phi);
}

void DefUseChains::add_phi_use(MachineOperandDesc* desc, MachinePhiInst* phi)
{
	auto& chain = get(desc->op);
	chain.uses.emplace_back(desc, phi);
}

const Occurrence& DefUseChains::get_definition(const MachineOperand* operand) const
{
	auto& chain = get(operand);
	return *chain.definition;
}

const std::list<Occurrence>& DefUseChains::get_uses(const MachineOperand* operand) const
{
	auto& chain = get(operand);
	return chain.uses;
}

void NewLivetimeAnalysisPass::initialize_blocks()
{
	LOG1("Marking loop exits with higher distance\n");

	auto RPO = get_Artifact<ReversePostOrderPass>();
	auto MLP = get_Artifact<MachineLoopPass>();
	for (const auto block : *RPO) {
		block->mark_unprocessed();
		next_use.initialize_empty_sets_for(block);

		auto loop = MLP->get_Loop(block);
		if (!loop)
			continue;
		auto loopdepth = MLP->loop_nest(loop) + 1;

		for (auto i = get_successor_begin(block), e = get_successor_end(block); i != e; ++i) {
			auto successor = *i;
			auto successor_loop = MLP->get_Loop(successor);
			auto successor_loopdepth = MLP->loop_nest(successor_loop) + 1;

			if (successor_loopdepth < loopdepth) {
				LOG1(*successor << " is a loop exit block\n");
				successor->mark_loop_exit();
			}
		}
	}
}

void NewLivetimeAnalysisPass::initialize_instructions() const
{
	LOG1("Setting 'step' for each instruction in a basic block\n");
	auto RPO = get_Artifact<ReversePostOrderPass>();

	for (const auto block : *RPO) {
		unsigned step = 0;
		for (auto instruction : *block) {
			instruction->set_step(step++);
		}
	}
}

/**
 * Performs a postorder traversal while ignoring loop-back edges
 */
void NewLivetimeAnalysisPass::dag_dfs(MachineBasicBlock* block)
{
	auto loop_tree = get_Artifact<MachineLoopPass>();
	for (auto i = get_successor_begin(block), e = get_successor_end(block); i != e; ++i) {
		auto succ = *i;
		// Only visit the successor if it is unproccessed and not a loop-back
		// edge
		if (!succ->is_processed() && !loop_tree->is_backedge(block, succ)) {
			dag_dfs(succ);
		}
	}

	process_block(block);
}

void NewLivetimeAnalysisPass::process_block(MachineBasicBlock* block)
{
	LOG2(nl << Yellow << "Processing " << *block << reset_color << nl);

	calculate_liveout(block);
	calculate_livein(block);

	block->mark_processed();
}

void NewLivetimeAnalysisPass::calculate_liveout(MachineBasicBlock* block)
{
	auto loop_tree = get_Artifact<MachineLoopPass>();
	auto live = mbb_phi_uses(block, true);

	auto& next_use_out = next_use.get_next_use_out(block);
	next_use.add_operands(next_use_out, live, 0);

	for (auto i = get_successor_begin(block), e = get_successor_end(block); i != e; ++i) {
		auto successor = *i;
		if (!loop_tree->is_backedge(block, successor)) {
			// Live = Live u (LiveIn(S) \ PhiDefs(S))
			auto phi_def = mbb_phi_defs(successor, true);
			auto& live_in = get_live_in(successor);

			live |= (live_in - phi_def);

			NextUseSet next_use_in_copy = next_use.get_next_use_in(successor);
			next_use_in_copy.remove(phi_def);
			merge(next_use_out, next_use_in_copy);
		}
	}
	add_to_all(next_use_out, block->get_distance());

	get_live_out(block) = live;

	if (DEBUG_COND_N(2)) {
		LOG2_NAMED_CONTAINER("Live out:     ", get_live_out(block));
		LOG2_NAMED_CONTAINER("Next use out: ", next_use_out);
		LOG2(nl);
	}
}

void NewLivetimeAnalysisPass::calculate_livein(MachineBasicBlock* block)
{
	auto live = get_live_out(block);

	next_use.copy_out_to_in(block);
	auto& next_use_in = next_use.get_next_use_in(block);

	// MachineInstructions in "block" in reverse order
	for (auto i = block->rbegin(), e = block->rend(); i != e; ++i) {
		live = liveness_transfer(live, block->convert(i), true);
		next_use.transfer(next_use_in, *i, block->get_distance());
	}

	// LiveIn(B) = Live u PhiDefs(B)
	auto phi_def = mbb_phi_defs(block, true);
	get_live_in(block) = (live | phi_def);

	next_use.add_operands(next_use_in, phi_def, 0);

	if (DEBUG_COND_N(2)) {
		LOG2_NAMED_CONTAINER("Live in:      ", get_live_in(block));
		LOG2_NAMED_CONTAINER("Next use in:  ", next_use_in);
		LOG2(nl);
	}
}

OperandSet NewLivetimeAnalysisPass::mbb_phi_uses(MachineBasicBlock* block, bool record_defuse)
{
	auto phi_uses = MOF->EmptySet();

	std::for_each(get_successor_begin(block), get_successor_end(block), [&](auto successor) {
		std::for_each(successor->phi_begin(), successor->phi_end(), [&](auto phi_instr) {
			auto idx = successor->get_predecessor_index(block);
			auto operand = phi_instr->get(idx).op;
			if (!reg_alloc_considers_operand(*operand))
				return;

			phi_uses.add(operand);

			if (record_defuse)
				this->def_use_chains.add_phi_use(&phi_instr->get(idx), phi_instr);

			LOG3("\tphi operand " << operand << " is used in " << *block << " (phi instr in "
			                      << *successor << ")" << nl);
		});
	});

	return phi_uses;
}

OperandSet NewLivetimeAnalysisPass::mbb_phi_defs(MachineBasicBlock* block, bool record_defuse)
{
	auto phi_defs = MOF->EmptySet();

	std::for_each(block->phi_begin(), block->phi_end(), [&](auto phi_instr) {
		auto operand = phi_instr->get_result().op;
		if (!reg_alloc_considers_operand(*operand))
			return;

		phi_defs.add(operand);

		if (record_defuse)
			this->def_use_chains.add_phi_definition(&phi_instr->get_result(), phi_instr);

		LOG3("\tphi operand " << operand << " is defined in " << *block << nl);
	});

	return phi_defs;
}

OperandSet& NewLivetimeAnalysisPass::get_liveset(LiveMapTy& map, MachineBasicBlock* block)
{
	auto it = map.find(block);
	if (it == map.end()) {
		it = map.emplace(block, MOF->EmptySet()).first;
	}
	return it->second;
}

void NewLivetimeAnalysisPass::looptree_dfs(MachineLoop* loop, const OperandSet& parent_live_loop)
{
	LOG2(nl << Yellow << "Processing loop: " << loop);
	if (loop->get_parent()) {
		LOG2(" [Parent: " << loop->get_parent() << "]");
	}
	LOG2(reset_color << nl);

	// LiveLoop = LiveIn(Head) \ PhiDefs(Head)
	auto live_loop = (get_live_in(loop->get_header()) - mbb_phi_defs(loop->get_header()));
	live_loop |= parent_live_loop;

	for (auto i = loop->child_begin(), e = loop->child_end(); i != e; ++i) {
		auto child = *i;

		if (DEBUG_COND_N(2)) {
			auto diff_live_in = live_loop - get_live_in(child);
			auto diff_live_out = live_loop - get_live_out(child);

			LOG2_NAMED_CONTAINER("Live in added from loop (" << *child << "):  ", diff_live_in);
			LOG2_NAMED_CONTAINER("Live out added from loop (" << *child << "): ", diff_live_out);
		}

		get_live_in(child) |= live_loop;
		get_live_out(child) |= live_loop;
	}

	for (auto i = loop->loop_begin(), e = loop->loop_end(); i != e; ++i) {
		looptree_dfs(*i, live_loop);
	}
}

void NewLivetimeAnalysisPass::next_use_fixed_point()
{
	bool changed = false;
	unsigned iteration = 1;

	do {
		LOG2(Yellow << "Running iteration " << iteration << " of next-use fixed-point analysis\n"
		            << reset_color);

		changed = false;
		auto RPO = get_Artifact<ReversePostOrderPass>();
		for (auto i = RPO->rbegin(), e = RPO->rend(); i != e; ++i) {
			auto block = *i;

			auto& live_out = get_live_out(block);
			auto next_use_out_ops = next_use.get_next_use_out(block).GetOperandSet();
			auto missing_out_ops = live_out - next_use_out_ops;

			auto& live_in = get_live_in(block);
			auto next_use_in_ops = next_use.get_next_use_in(block).GetOperandSet();
			auto missing_in_ops = live_in - next_use_in_ops;

			if (missing_in_ops.empty() && missing_out_ops.empty())
				continue;

			LOG2(Cyan << "\nProcessing block " << *block << reset_color << nl);

			if (DEBUG_COND_N(3)) {
				LOG3_NAMED_CONTAINER("Missing operands in next_use_out: ", missing_out_ops);
				LOG3_NAMED_CONTAINER("Missing operands in next_use_in:  ", missing_in_ops);
			}

			// Merge live-in of successors again, this time use back-edges
			auto& next_use_out = next_use.get_next_use_out(block);
			for (auto i = get_successor_begin(block), e = get_successor_end(block); i != e; ++i) {
				auto successor = *i;
				// Live = Live u (LiveIn(S) \ PhiDefs(S))
				auto phi_def = mbb_phi_defs(successor);

				NextUseSet next_use_in_copy = next_use.get_next_use_in(successor);
				next_use_in_copy.remove(phi_def);
				changed = merge(next_use_out, next_use_in_copy) || changed;
			}

			// Add block distance to all operands that were missing and are now present
			if (block->get_distance() > 0) {
				std::for_each(missing_out_ops.begin(), missing_out_ops.end(), [&](auto& operand) {
					if (next_use_out.contains(operand)) {
						next_use_out[operand] += block->get_distance();
					}
				});
			}

			if (DEBUG_COND_N(3)) {
				LOG3_NAMED_CONTAINER("New next use out: ", next_use_out);
			}

			// The missing live-ins should be added by simply looking them up in the live-out set
			// and adding the length of the block
			// TODO: Check that this is indeed the correct way to handle it
			auto& next_use_in = next_use.get_next_use_in(block);
			std::for_each(missing_in_ops.begin(), missing_in_ops.end(), [&](auto& operand) {
				if (next_use_out.contains(operand)) {
					unsigned distance = next_use_out[operand] + block->size();
					next_use_in.insert(operand, distance);
					changed = true;
				}
			});

			if (DEBUG_COND_N(3)) {
				LOG3_NAMED_CONTAINER("New next use in:  ", next_use_in);
			}
		}

		iteration++;
	} while (changed);
}

bool NewLivetimeAnalysisPass::run(JITData& JD)
{
	LOG(nl << BoldYellow << "Running NewLivetimeAnalysisPass" << reset_color << nl);

	MOF = JD.get_MachineOperandFactory();

	MachineInstructionSchedulingPass* MIS = get_Artifact<LIRInstructionScheduleArtifact>()->MIS;
	MachineLoopPass* ML = get_Artifact<MachineLoopPass>();

	// Reset all basic blocks and set their distance (for next-use)
	initialize_blocks();
	initialize_instructions();

	LOG1("Running first phase: post-order traversel over CFG for initial live sets\n");
	dag_dfs(MIS->front());

	LOG1("\nRunning second phase: Propagate liveness from loop-headers "
	     "down to all BB within loops\n");
	for (auto i = ML->loop_begin(), e = ML->loop_end(); i != e; ++i) {
		looptree_dfs(*i, MOF->EmptySet());
	}

	LOG1("\nRunning fixed-point analysis for next-uses\n");
	next_use_fixed_point();

	return true;
}

OperandSet NewLivetimeAnalysisPass::liveness_transfer(const OperandSet& live,
                                                      const MIIterator& mi_iter,
                                                      bool record_defuse)
{
	auto instruction = *mi_iter;
	OperandSet result(live);

	assert(!instruction->to_MachinePhiInst() &&
	       "Arguments of a phi functions are not live at the beginning of a block!");

	auto result_operands_transfer = [&](auto& descriptor) {
		auto operand = descriptor.op;
		if (!reg_alloc_considers_operand(*operand))
			return;

		result.remove(operand);
		assert(!operand->has_embedded_operands() && "Embedded result operands not implemented!");

		if (record_defuse)
			def_use_chains.add_definition(&descriptor, mi_iter);
	};
	std::for_each(instruction->results_begin(), instruction->results_end(),
	              result_operands_transfer);

	auto used_operands_transfer = [&](auto& descriptor) {
		auto operand = descriptor.op;
		if (!reg_alloc_considers_operand(*operand))
			return;

		result.add(operand);

		if (record_defuse)
			def_use_chains.add_use(&descriptor, mi_iter);
	};

	std::for_each(instruction->begin(), instruction->end(), used_operands_transfer);
	std::for_each(instruction->dummy_begin(), instruction->dummy_end(), used_operands_transfer);

	return result;
}

NextUseSetUPtrTy NewLivetimeAnalysisPass::next_use_set_from(MachineInstruction* instruction)
{
	auto block = instruction->get_block();
	auto set = std::make_unique<NextUseSet>(next_use.get_next_use_out(block));
	for (auto i = block->rbegin(), e = block->rend(); i != e; ++i) {
		next_use.transfer(*set, *i, block->get_distance());

		if (instruction->get_id() == (*i)->get_id()) {
			break;
		}
	}

	return set;
}

// pass usage
PassUsage& NewLivetimeAnalysisPass::get_PassUsage(PassUsage& PU) const
{
	PU.provides<NewLivetimeAnalysisPass>();
	PU.requires<MachineLoopPass>();
	PU.requires<LIRInstructionScheduleArtifact>();
	PU.requires<ReversePostOrderPass>();
	return PU;
}

// register pass
static PassRegistry<NewLivetimeAnalysisPass> X("NewLivetimeAnalysisPass");
static ArtifactRegistry<NewLivetimeAnalysisPass> Y("NewLivetimeAnalysisPass");

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
