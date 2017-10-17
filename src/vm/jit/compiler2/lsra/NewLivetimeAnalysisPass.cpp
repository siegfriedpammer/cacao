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

static NextUse2&
get_or_create(NextUseSet& set, MachineOperand* operand, unsigned default_distance = 0)
{
	auto iter = find(set, operand);
	if (iter == set.end()) {
		iter = set.insert(set.end(), {operand, default_distance});
	}
	return *iter;
}

static void set(NextUseSet& set, MachineOperand* operand, unsigned distance)
{
	auto& next_use = get_or_create(set, operand);
	next_use.distance = distance;
}

static void remove(NextUseSet& set, MachineOperand* operand)
{
	auto iter = find(set, operand);
	if (iter != set.end()) {
		set.erase(iter);
	}
}

static bool contains(const std::vector<MachineOperand*>& list, MachineOperand* operand)
{
	auto iter = std::find_if(list.begin(), list.end(), [operand](const auto& element) {
		return operand->aquivalent(*element);
	});
	return iter != list.end();
}

static bool next_use_cmp(const NextUse2& lhs, const NextUse2& rhs)
{
	return lhs.operand->aquivalence_less(*rhs.operand);
}

/// Merges the set given in the second parameter into the set given
/// the first parameter. If an operand is present in boths sets, the
/// minimum of both distances is used
/// Returns true if anything has changed in target
static bool merge(NextUseSet& target, const NextUseSet& src)
{
	bool change = false;

	for (const auto& next_use_src : src) {
		auto iter = find(target, next_use_src.operand);
		if (iter == target.end()) {
			target.push_back(next_use_src);
			change = true;
		}
		else {
			auto& next_use_dst = *iter;
			auto old_distance = next_use_dst.distance;
			next_use_dst.distance = std::min(next_use_src.distance, next_use_dst.distance);

			if (old_distance != next_use_dst.distance)
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

	for (auto& next_use : set) {
		next_use.distance += distance;
	}
}

/// Converts a NextUseSet to a LiveSet (for easier comparison)
static LiveTy convert(unsigned initial_size, const NextUseSet& set)
{
	LiveTy live(initial_size);

	for (const auto& next_use : set) {
		live[next_use.operand->get_id()] = true;
	}

	return live;
}

} // namespace

NextUseSet::iterator find(NextUseSet& set, MachineOperand* operand)
{
	return std::find_if(set.begin(), set.end(), [operand](const auto& next_use) {
		return operand->aquivalent(*next_use.operand);
	});
}

void NextUseInformation::initialize_empty_sets_for(MachineBasicBlock* block)
{
	next_use_ins.emplace(block, std::make_unique<NextUseSet>());
	next_use_outs.emplace(block, std::make_unique<NextUseSet>());
}

void NextUseInformation::add_operands(NextUseSet& set, const LiveTy& live, unsigned distance)
{
	LTA.for_each(live, [&](const auto operand) {
		if (find(set, operand) == set.end())
			set.push_back({operand, distance});
	});
}

void NextUseInformation::remove_operands(NextUseSet& set, const LiveTy& live)
{
	LTA.for_each(live, [&](const auto operand) { remove(set, operand); });
}

void NextUseInformation::copy_out_to_in(MachineBasicBlock* block)
{
	auto& next_use_in = *next_use_ins.at(block);
	auto& next_use_out = *next_use_outs.at(block);

	next_use_in.clear();
	std::copy(next_use_out.begin(), next_use_out.end(), std::back_inserter(next_use_in));
}

void NextUseInformation::transfer(NextUseSet& next_use_set,
                                  MachineInstruction* instruction,
                                  unsigned block_distance) const
{
	std::vector<MachineOperand*> updated_operands;

	auto defs_lambda = [&](const auto& descriptor) {
		if (!reg_alloc_considers_operand(*descriptor.op))
			return;

		remove(next_use_set, descriptor.op);
		updated_operands.push_back(descriptor.op);
	};
	std::for_each(instruction->results_begin(), instruction->results_end(), defs_lambda);

	auto uses_lambda = [&](const auto& descriptor) {
		if (!reg_alloc_considers_operand(*descriptor.op))
			return;

		set(next_use_set, descriptor.op, block_distance);
		updated_operands.push_back(descriptor.op);
	};
	std::for_each(instruction->begin(), instruction->end(), uses_lambda);
	std::for_each(instruction->dummy_begin(), instruction->dummy_end(), uses_lambda);

	for (auto& next_use : next_use_set) {
		if (!contains(updated_operands, next_use.operand)) {
			next_use.distance++;
		}
	}
}

NextUseSet& NextUseInformation::get_next_use_out(MachineBasicBlock* block)
{
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

/*void DefUseChains::add_use(MachineOperandDesc* desc, MachinePhiInst* phi)
{
	auto& chain = get(desc->op);
	chain.uses.emplace_back(desc, phi);
}*/

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

void NewLivetimeAnalysisPass::initialize_blocks() const
{
	LOG1("Marking loop exits with higher distance\n");

	auto RPO = get_Pass<ReversePostOrderPass>();
	auto MLP = get_Pass<MachineLoopPass>();
	for (const auto block : *RPO) {
		block->mark_unprocessed();

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
	auto RPO = get_Pass<ReversePostOrderPass>();

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
	auto loop_tree = get_Pass<MachineLoopPass>();
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

	next_use.initialize_empty_sets_for(block);

	calculate_liveout(block);
	calculate_livein(block);

	block->mark_processed();
}

void NewLivetimeAnalysisPass::calculate_liveout(MachineBasicBlock* block)
{
	auto loop_tree = get_Pass<MachineLoopPass>();
	auto live = phi_uses(block);

	auto& next_use_out = next_use.get_next_use_out(block);
	next_use.add_operands(next_use_out, live, 0);

	for (auto i = get_successor_begin(block), e = get_successor_end(block); i != e; ++i) {
		auto successor = *i;
		if (!loop_tree->is_backedge(block, successor)) {
			// Live = Live u (LiveIn(S) \ PhiDefs(S))
			auto phi_def = phi_defs(successor);
			auto live_in = get_live_in(successor);

			live |= (live_in - phi_def);

			NextUseSet next_use_in_copy = next_use.get_next_use_in(successor);
			next_use.remove_operands(next_use_in_copy, phi_def);
			merge(next_use_out, next_use_in_copy);
		}
	}
	add_to_all(next_use_out, block->get_distance());

	get_live_out(block) = live;

	LOG2("Live out:     ");
	DEBUG2(log_livety(dbg(), get_live_out(block)));
	LOG2(nl);

	LOG2("Next use out: ");
	DEBUG2(std::sort(next_use_out.begin(), next_use_out.end(), next_use_cmp));
	DEBUG2(print_container(dbg(), next_use_out.begin(), next_use_out.end()));
	LOG2("\n\n");
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
	auto phi_def = phi_defs(block);
	get_live_in(block) = (live | phi_def);

	next_use.add_operands(next_use_in, phi_def, 0);

	LOG2("Live in:      ");
	DEBUG2(log_livety(dbg(), get_live_in(block)));
	LOG2(nl);

	LOG2("Next use in:  ");
	DEBUG2(std::sort(next_use_in.begin(), next_use_in.end(), next_use_cmp));
	DEBUG2(print_container(dbg(), next_use_in.begin(), next_use_in.end()));
	LOG2(nl);
}

LiveTy NewLivetimeAnalysisPass::phi_uses(MachineBasicBlock* block)
{
	LiveTy phi_uses(num_operands);

	std::for_each(get_successor_begin(block), get_successor_end(block), [&](auto successor) {
		std::for_each(successor->phi_begin(), successor->phi_end(), [&](auto phi_instr) {
			auto idx = successor->get_predecessor_index(block);
			auto operand = phi_instr->get(idx).op;
			if (!reg_alloc_considers_operand(*operand)) {
				return;
			}
			phi_uses[operand->get_id()] = true;

			LOG3("\tphi operand " << *operand << " is used in " << *block << " (phi instr in "
			                      << *successor << ")" << nl);
			operands[operand->get_id()] = operand;
		});
	});

	return phi_uses;
}

LiveTy NewLivetimeAnalysisPass::phi_defs(MachineBasicBlock* block)
{
	LiveTy phi_defs(num_operands);

	std::for_each(block->phi_begin(), block->phi_end(), [&](auto phi_instr) {
		auto operand = phi_instr->get_result().op;
		if (!reg_alloc_considers_operand(*operand)) {
			return;
		}
		phi_defs[operand->get_id()] = true;

		LOG3("\tphi operand " << *operand << " is defined in " << *block << nl);
		operands[operand->get_id()] = operand;
	});

	return phi_defs;
}

#if !defined(NDEBUG)

void NewLivetimeAnalysisPass::log_livety(OStream& OS, const LiveTy& liveSet)
{
	bool loggedElement = false;

	OS << "[";
	if (liveSet.none()) {
		OS << "<empty>";
	}

	for (std::size_t i = 0; i < liveSet.size(); ++i) {
		auto operand = operands[i];

		if (liveSet[i]) {
			if (loggedElement)
				OS << ", ";
			OS << operand;
			loggedElement = true;
		}
	}
	OS << "]";
}

#endif

void NewLivetimeAnalysisPass::looptree_dfs(MachineLoop* loop, const LiveTy& parent_live_loop)
{
	LOG2(nl << Yellow << "Processing loop: " << loop);
	if (loop->get_parent()) {
		LOG2(" [Parent: " << loop->get_parent() << "]");
	}
	LOG2(reset_color << nl);

	// LiveLoop = LiveIn(Head) \ PhiDefs(Head)
	LiveTy live_loop = (get_live_in(loop->get_header()) - phi_defs(loop->get_header()));
	live_loop |= parent_live_loop;

	for (auto i = loop->child_begin(), e = loop->child_end(); i != e; ++i) {
		auto child = *i;

		if (DEBUG_COND_N(2)) {
			auto diff_live_in = live_loop - get_live_in(child);
			auto diff_live_out = live_loop - get_live_out(child);

			LOG2("Live in added from loop (" << *child << "):  ");
			DEBUG2(log_livety(dbg(), diff_live_in));
			LOG2(nl);

			LOG2("Live out added from loop (" << *child << "): ");
			DEBUG2(log_livety(dbg(), diff_live_out));
			LOG2(nl);
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
		auto RPO = get_Pass<ReversePostOrderPass>();
		for (auto i = RPO->rbegin(), e = RPO->rend(); i != e; ++i) {
			auto block = *i;

			auto& live_out = get_live_out(block);
			auto next_use_out_ops = convert(num_operands, next_use.get_next_use_out(block));
			auto missing_out_ops = live_out - next_use_out_ops;

			auto& live_in = get_live_in(block);
			auto next_use_in_ops = convert(num_operands, next_use.get_next_use_in(block));
			auto missing_in_ops = live_in - next_use_in_ops;

			if (missing_in_ops.none() && missing_out_ops.none())
				continue;

			LOG2(Cyan << "\nProcessing block " << *block << reset_color << nl);

			if (DEBUG_COND_N(3)) {
				LOG3("Missing operands in next_use_out: ");
				DEBUG3(log_livety(dbg(), missing_out_ops));
				LOG3(nl);

				LOG3("Missing operands in next_use_in:  ");
				DEBUG3(log_livety(dbg(), missing_in_ops));
				LOG3(nl);
			}

			// Merge live-in of successors again, this time use back-edges
			auto& next_use_out = next_use.get_next_use_out(block);
			for (auto i = get_successor_begin(block), e = get_successor_end(block); i != e; ++i) {
				auto successor = *i;
				// Live = Live u (LiveIn(S) \ PhiDefs(S))
				auto phi_def = phi_defs(successor);

				NextUseSet next_use_in_copy = next_use.get_next_use_in(successor);
				next_use.remove_operands(next_use_in_copy, phi_def);
				changed = merge(next_use_out, next_use_in_copy) || changed;
			}

			// Add block distance to all operands that were missing and are now present
			if (block->get_distance() > 0) {
				for_each(missing_out_ops, [&](const auto operand) {
					auto iter = find(next_use_out, operand);
					if (iter != next_use_out.end()) {
						iter->distance += block->get_distance();
					}
				});
			}

			if (DEBUG_COND_N(3)) {
				LOG3("New next use out: ");
				DEBUG3(std::sort(next_use_out.begin(), next_use_out.end(), next_use_cmp));
				DEBUG3(print_container(dbg(), next_use_out.begin(), next_use_out.end()));
				LOG3(nl);
			}

			// The missing live-ins should be added by simply looking them up in the live-out set
			// and adding the length of the block
			// TODO: Check that this is indeed the correct way to handle it
			auto& next_use_in = next_use.get_next_use_in(block);
			for_each(missing_in_ops, [&](const auto operand) {
				auto iter = find(next_use_out, operand);
				if (iter != next_use_out.end()) {
					unsigned distance = iter->distance + block->size();
					next_use_in.push_back({operand, distance});
					changed = true;
				}
			});

			if (DEBUG_COND_N(3)) {
				LOG3("New next use in:  ");
				DEBUG3(std::sort(next_use_in.begin(), next_use_in.end(), next_use_cmp));
				DEBUG3(print_container(dbg(), next_use_in.begin(), next_use_in.end()));
				LOG3(nl);
			}
		}

		iteration++;
	} while (changed);
}

bool NewLivetimeAnalysisPass::run(JITData& JD)
{
	LOG(nl << BoldYellow << "Running NewLivetimeAnalysisPass" << reset_color << nl);
	num_operands = MachineOperand::get_id_counter();
	operands = std::vector<MachineOperand*>(num_operands, nullptr);
	LOG2("Initializing bitsets with size " << num_operands << " (number of operands)" << nl);

	MachineInstructionSchedulingPass* MIS = get_Pass<MachineInstructionSchedulingPass>();
	MachineLoopPass* ML = get_Pass<MachineLoopPass>();

	// Reset all basic blocks and set their distance (for next-use)
	initialize_blocks();
	initialize_instructions();

	LOG1("Running first phase: post-order traversel over CFG for initial live sets\n");
	dag_dfs(MIS->front());

	LOG1("\nRunning second phase: Propagate liveness from loop-headers "
	     "down to all BB within loops\n");
	for (auto i = ML->loop_begin(), e = ML->loop_end(); i != e; ++i) {
		looptree_dfs(*i, boost::dynamic_bitset<>(num_operands));
	}

	LOG1("\nRunning fixed-point analysis for next-uses\n");
	next_use_fixed_point();

	return true;
}

LiveTy NewLivetimeAnalysisPass::liveness_transfer(const LiveTy& live,
                                                  const MIIterator& mi_iter,
                                                  bool record_defuse)
{
	auto instruction = *mi_iter;
	LiveTy result(live);

	assert(!instruction->to_MachinePhiInst() &&
	       "Arguments of a phi functions are not live at the beginning of a block!");

	if (!reg_alloc_considers_instruction(instruction))
		return result;

	auto result_operands_transfer = [&](auto& descriptor) {
		auto operand = descriptor.op;
		if (!reg_alloc_considers_operand(*operand))
			return;

		result[operand->get_id()] = false;
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

		result[operand->get_id()] = true;
		operands[operand->get_id()] = operand;

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
	PU.add_requires<MachineLoopPass>();
	PU.add_requires<MachineInstructionSchedulingPass>();
	PU.add_requires<ReversePostOrderPass>();
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
