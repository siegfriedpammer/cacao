/* src/vm/jit/compiler2/lsra/NewSpillPass.cpp - NewSpillPass

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

#include "vm/jit/compiler2/lsra/NewSpillPass.hpp"

#include <deque>

#include "toolbox/logging.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineDominatorPass.hpp"
#include "vm/jit/compiler2/MachineInstruction.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/ReversePostOrderPass.hpp"
#include "vm/jit/compiler2/lsra/LogHelper.hpp"
#include "vm/jit/compiler2/lsra/LoopPressurePass.hpp"
#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"

#define DEBUG_NAME "compiler2/NewSpillPass"

namespace cacao {
namespace jit {
namespace compiler2 {

static OperandSet used_operands(const MachineOperandFactory* MOF,
                                const RegisterClass* rc,
                                MachineInstruction* instruction)
{
	auto set = MOF->EmptySet();

	auto uses_lambda = [&](const auto& descriptor) {
		if (!rc->handles_type(descriptor.op->get_type()))
			return;

		if (descriptor.op->is_virtual()) {
			set.add(descriptor.op);
		}
	};

	std::for_each(instruction->begin(), instruction->end(), uses_lambda);
	std::for_each(instruction->dummy_begin(), instruction->dummy_end(), uses_lambda);

	return set;
}

static OperandSet defined_operands(const MachineOperandFactory* MOF,
                                   const RegisterClass* rc,
                                   MachineInstruction* instruction)
{
	auto set = MOF->EmptySet();

	auto def_lambda = [&](const auto& descriptor) {
		if (!rc->handles_type(descriptor.op->get_type()))
			return;

		if (descriptor.op->is_virtual()) {
			set.add(descriptor.op);
		}
	};

	std::for_each(instruction->results_begin(), instruction->results_end(), def_lambda);

	return set;
}

/// \todo Maybe can be refactored by using Def-Use chains since the link
///       to the definition should reveal if its a PHI or not
static MachinePhiInst* is_defined_by_phi(const MachineOperand* operand, MachineBasicBlock* block)
{
	for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
		auto phi_instr = *i;
		if (phi_instr->get_result().op->aquivalent(*operand)) {
			return phi_instr;
		}
	}
	return nullptr;
}

void SpillInfo::add_spill(MachineOperand* operand, const MIIterator& position)
{
	auto iter = spilled_operands.find(operand->get_id());
	assert(iter == spilled_operands.end() &&
	       "Multiple spill positions for same variable not implemented!");

	auto spilled_op = std::make_unique<SpilledOperand>(operand, position);
	spilled_operands.emplace(operand->get_id(), std::move(spilled_op));
}

void SpillInfo::add_reload(MachineOperand* operand, const MIIterator& position)
{
	auto iter = spilled_operands.find(operand->get_id());
	assert(iter != spilled_operands.end() && "Reload for unspilled variable called!");

	auto& spilled_operand = *iter->second;
	spilled_operand.reload_positions.push_back(position);
}

void SpillInfo::insert_instructions(const Backend& backend, StackSlotManager& ssm) const
{
	for (const auto& pair : spilled_operands) {
		auto& spilled_op = *pair.second;
		auto operand = spilled_op.operand;
		std::vector<Occurrence> new_definitions;

		LOG1(nl << Yellow << "Inserting spill/reload instructions for " << operand << reset_color
		        << nl);

		spilled_op.stackslot = ssm.create_slot(operand->get_type());
		auto spill_instr = backend.create_Move(operand, spilled_op.stackslot);

		LOG2("Inserting " << spill_instr << nl);
		insert_before(spilled_op.spill_position, spill_instr);

		for (const auto& reload_position : spilled_op.reload_positions) {
			auto new_def =
			    backend.get_JITData()->get_MachineOperandFactory()->CreateVirtualRegister(
			        operand->get_type());
			auto reload_instr = backend.create_Move(spilled_op.stackslot, new_def);

			LOG2("Inserting " << reload_instr << nl);
			auto iter = insert_before(reload_position, reload_instr);

			new_definitions.emplace_back(&reload_instr->get_result(), iter);
		}

		auto LTA = sp->get_Artifact<NewLivetimeAnalysisPass>();
		auto& chains = LTA->get_def_use_chains();
		const auto& original_definition = chains.get_definition(operand);

		// sp->reconstruct_ssa(original_definition, new_definitions);
		sp->ssa_reconstructor.reconstruct(original_definition, new_definitions);
	}
}

void SSAReconstructor::reconstruct(const Occurrence& original_def,
                                   const std::vector<Occurrence>& definitions)
{
	current_def.clear();
	current_type = original_def.operand->get_type();

	std::vector<const MachineOperand*> defined_ops;
	std::for_each(definitions.cbegin(), definitions.cend(),
	              [&](const auto occurrence) { defined_ops.push_back(occurrence.operand); });

	LOG1(nl << Cyan << "Reconstructing SSA for " << original_def.operand << reset_color << nl);
	LOG1_NAMED_PTR_CONTAINER("New definitions: ", defined_ops);

	write_variable(original_def.block(), original_def.operand);
	for (auto& occurrence : definitions) {
		write_variable(occurrence.block(), occurrence.operand);
	}

	auto LTA = sp->get_Artifact<NewLivetimeAnalysisPass>();
	LTA->get_def_use_chains().for_each_use(original_def.operand, [&](const auto& occurrence) {
		auto operand = this->read_variable(occurrence.block());
		// LOG3("Found definition in " << *occurrence.block() << " = " << operand << nl);

		if (!original_def.operand->aquivalent(*operand)) {
			LOG3("Replacing " << original_def.operand << " with " << operand << nl);
			occurrence.descriptor->op = operand;
		}
	});

	// assert(false && "SSA Reconstruction not yet implemented!");
}

void SSAReconstructor::write_variable(MachineBasicBlock* block, MachineOperand* operand)
{
	current_def[block] = operand;
	LOG3("write_variable " << *block << " = " << operand << nl);
}

MachineOperand* SSAReconstructor::read_variable(MachineBasicBlock* block)
{
	auto iter = current_def.find(block);
	if (iter != current_def.end()) {
		// Local value numbering
		return iter->second;
	}
	// Global value numbering
	return read_variable_recursive(block);
}

MachineOperand* SSAReconstructor::read_variable_recursive(MachineBasicBlock* block)
{
	MachineOperand* operand = nullptr;

	// Do not create a PHI if block has only one predecessor
	if (block->pred_size() == 1) {
		operand = read_variable(*block->pred_begin());
	}
	else {
		auto instr = new MachinePhiInst(block->pred_size(), current_type, nullptr, sp->mof);
		instr->set_block(block);

		write_variable(block, instr->get_result().op);
		operand = add_phi_operands(instr);

		if (operand->aquivalent(*instr->get_result().op)) {
			block->insert_phi(instr);
		}
	}

	write_variable(block, operand);

	return operand;
}

MachineOperand* SSAReconstructor::add_phi_operands(MachinePhiInst* instr)
{
	auto block = instr->get_block();
	for (std::size_t i = 0, e = block->pred_size(); i < e; ++i) {
		auto operand = read_variable(block->get_predecessor(i));
		instr->get(i).op = operand;

		LOG3("\tPredecessor " << *block->get_predecessor(i) << " has operand " << operand << nl);
	}
	LOG3("After adding operands to PHI " << instr << nl);
	return try_remove_trivial_phi(instr);
}

MachineOperand* SSAReconstructor::try_remove_trivial_phi(MachinePhiInst* instr)
{
	MachineOperand* same = nullptr;
	for (const auto& descriptor : *instr) {
		auto operand = descriptor.op;
		if (operand->aquivalent(*instr->get_result().op) || (same && operand->aquivalent(*same)) ||
		    operand->aquivalent(NoOperand))
			continue;
		if (same)
			return instr->get_result().op; // The phi merges at least two values
		same = operand;
	}

	if (!same) {
		// The phi is unreachalbe or in the start block
		same = &NoOperand;
	}

	return same;
}

void NewSpillPass::limit(OperandSet& workset,
                         OperandSet& spillset,
                         const MIIterator& mi_iter,
                         const MIIterator& mi_spill_before,
                         unsigned m)
{
	LOG3("limit called with m = " << m << nl);
	LOG3_NAMED_CONTAINER("Workset:        ", workset);
	LOG3_NAMED_CONTAINER("Spillset:       ", spillset);

	if (workset.size() <= m)
		return;

	auto instruction = *mi_iter;
	auto workset_as_list = workset.ToList();
	sort_by_next_use(*workset_as_list, instruction);
	LOG3_NAMED_PTR_CONTAINER("Sorted workset: ", *workset_as_list);

	auto LTA = get_Artifact<NewLivetimeAnalysisPass>();
	auto next_use_set = LTA->next_use_set_from(instruction);
	auto iter = std::next(workset_as_list->begin(), m);
	for (auto i = iter, e = workset_as_list->end(); i != e; ++i) {
		const auto operand = *i;

		if (!spillset.contains(operand) && next_use_set->contains(*operand)) {
			LOG2(Yellow << "Spilling " << operand << reset_color << nl);
			spill_info.add_spill(operand, mi_spill_before);
		}
		spillset.remove(operand);
	}
	workset_as_list->erase(iter, workset_as_list->end());
	workset = *workset_as_list; // Copy assignement implemented for this
}

void NewSpillPass::sort_by_next_use(OperandList& list, MachineInstruction* instruction) const
{
	auto LTA = get_Artifact<NewLivetimeAnalysisPass>();
	auto next_use_set_ptr = LTA->next_use_set_from(instruction);
	auto next_use_set = *next_use_set_ptr;

	LOG3_NAMED_CONTAINER("Next use set for sort call: ", next_use_set);

	std::sort(list.begin(), list.end(), [&](const auto lhs, const auto rhs) {
		auto lhs_distance = next_use_set.contains(*lhs) ? next_use_set[*lhs] : kInfinity;
		auto rhs_distance = next_use_set.contains(*rhs) ? next_use_set[*rhs] : kInfinity;

		return lhs_distance < rhs_distance;
	});
}

OperandSet NewSpillPass::compute_workset(MachineBasicBlock* block)
{
	auto workset = mof->EmptySet();

	auto MLP = get_Artifact<MachineLoopPass>();
	auto LTA = get_Artifact<NewLivetimeAnalysisPass>();

	if (MLP->is_loop_header(block)) {
		auto used = used_in_loop(block);
		LOG2_NAMED_CONTAINER("Operands used in loop: ", used);

		auto& live_in = LTA->get_live_in(block);
		OperandSet live_through = mof->EmptySet();
		std::for_each(live_in.cbegin(), live_in.cend(), [&](const auto& operand) {
			if (current_rc->handles_type(operand.get_type()) && !used.contains(&operand))
				live_through.add(&operand);
		});
		LOG2_NAMED_CONTAINER("Live-through: ", live_through);

		const unsigned regs_count = current_rc->count();

		// Max loop pressure is the maximum of all loops where block is a header
		unsigned loop_pressure = 0;
		auto iter_pair = MLP->get_Loops_from_header(block);
		for (auto i = iter_pair.first, e = iter_pair.second; i != e; ++i) {
			auto current_loop_pressure = (*i)->get_register_pressures()[current_rc_idx];
			loop_pressure = std::max(loop_pressure, current_loop_pressure);
		}

		OperandSet add = mof->EmptySet();
		if (used.size() < regs_count) {
			unsigned free_loop_regs = regs_count - loop_pressure + live_through.size();
			auto live_through_as_list = live_through.ToList();
			sort_by_next_use(*live_through_as_list, block->front());

			unsigned live_through_count = std::min<unsigned>(live_through.size(), free_loop_regs);
			LOG2("Adding " << live_through_count << " live-through variables to workset.\n");

			auto to_iter = std::next(live_through_as_list->begin(), live_through_count);
			std::for_each(live_through_as_list->begin(), to_iter,
			              [&](const auto operand) { add.add(operand); });
		}
		else {
			auto used_as_list = used.ToList();
			sort_by_next_use(*used_as_list, block->front());
			used_as_list->erase(std::next(used_as_list->begin(), regs_count), used_as_list->end());
			used = *used_as_list;
		}

		workset = (used | add);
	}
	else {
		std::map<std::size_t, unsigned> frequencies;
		OperandSet take = mof->EmptySet();
		OperandSet cand = mof->EmptySet();

		// Handle PHI instructions first, if all operands of a PHI are available,
		// put the result into "take"
		for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
			auto phi_instr = *i;

			if (!current_rc->handles_type(phi_instr->get_result().op->get_type()))
				continue;

			bool available_everywhere = true;
			bool available_nowhere = true;
			for (auto j = block->pred_begin(), je = block->pred_end(); j != je; ++j) {
				auto predecessor = *j;

				auto idx = block->get_predecessor_index(predecessor);
				auto operand = phi_instr->get(idx).op;

				// Is the operand in the workset of the predecessor?
				bool found = false;
				for (const auto& pred_op : worksets_exit.at(predecessor)) {
					if (!pred_op.aquivalent(*operand))
						continue;

					found = true;
					available_nowhere = false;
					break;
				}

				available_everywhere &= found;
			}

			if (available_everywhere) {
				take.add(phi_instr->get_result().op);
			}
			else if (!available_nowhere) {
				cand.add(phi_instr->get_result().op);
			}
			else {
				assert(false && "Spilling phis not implemented!");
			}
		}

		// Handle Live-ins now, since we already handled phi definitions before,
		// we can ignore them now
		auto live_in = LTA->get_live_in(block) - LTA->mbb_phi_defs(block);
		std::for_each(live_in.cbegin(), live_in.cend(), [&](const auto& operand) {
			if (!current_rc->handles_type(operand.get_type()))
				return;

			bool available_everywhere = true;
			bool available_nowhere = true;

			for (auto j = block->pred_begin(), je = block->pred_end(); j != je; ++j) {
				auto predecessor = *j;

				// Is the operand in the workset of the predecessor?
				bool found = false;
				for (const auto& pred_op : worksets_exit.at(predecessor)) {
					if (!pred_op.aquivalent(operand))
						continue;

					found = true;
					available_nowhere = false;
					break;
				}

				available_everywhere &= found;
			}

			if (available_everywhere) {
				take.add(&operand);
			}
			else {
				cand.add(&operand);
			}

		});

		workset = take;

		auto cand_as_list = cand.ToList();
		sort_by_next_use(*cand_as_list, block->front());

		const unsigned copy_count =
		    std::min(cand_as_list->size(), current_rc->count() - take.size());
		auto from_iter = std::next(cand_as_list->begin(), copy_count);
		cand_as_list->erase(from_iter, cand_as_list->end());
		cand = *cand_as_list;

		workset |= cand;
	}

	return workset;
}

OperandSet NewSpillPass::compute_spillset(MachineBasicBlock* block, const OperandSet& workset)
{
	auto spillset_union = mof->EmptySet();

	for (auto i = block->pred_begin(), e = block->pred_end(); i != e; ++i) {
		const auto predecessor = *i;
		auto iter = spillsets_exit.find(predecessor);
		if (iter == spillsets_exit.end())
			continue;

		spillset_union |= iter->second;
	}

	return spillset_union & workset;
}

OperandSet NewSpillPass::used_in_loop(MachineBasicBlock* block)
{
	auto MLP = get_Artifact<MachineLoopPass>();
	auto LTA = get_Artifact<NewLivetimeAnalysisPass>();

	auto used_operands = mof->EmptySet();
	auto live_in = LTA->get_live_in(block);

	auto iter_pair = MLP->get_Loops_from_header(block);
	for (auto i = iter_pair.first, e = iter_pair.second; i != e; ++i) {
		used_operands |= used_in_loop(*i, live_in);
	}

	return used_operands;
}

OperandSet NewSpillPass::used_in_loop(MachineLoop* loop, OperandSet& live_loop)
{
	auto used_operands = mof->EmptySet();

	if (live_loop.empty())
		return used_operands;

	LOG2(nl << Yellow << "Processing loop: " << loop);
	if (loop->get_parent()) {
		LOG2(" [Parent: " << loop->get_parent() << "]");
	}
	LOG2(reset_color << nl);

	auto LTA = get_Artifact<NewLivetimeAnalysisPass>();
	auto& chains = LTA->get_def_use_chains();

	for (auto i = loop->child_begin(), e = loop->child_end(); i != e; ++i) {
		auto child = *i;

		OperandSet found_uses(live_loop);

		std::for_each(live_loop.begin(), live_loop.end(), [&](auto& used_operand) {
			if (!current_rc->handles_type(used_operand.get_type()))
				return;

			const auto& occurrences = chains.get_uses(&used_operand);
			for (const auto& occurrence : occurrences) {
				auto usage_block = occurrence.block();
				if (*child == *usage_block) {
					used_operands.add(&used_operand);
					found_uses.remove(&used_operand);
					return;
				}
			}
		});

		// Remove all found_uses from the live set so we avoid unnecessary lookups
		live_loop &= found_uses;
	}

	// Process nested loops
	for (auto i = loop->loop_begin(), e = loop->loop_end(); i != e; ++i) {
		used_operands |= used_in_loop(*i, live_loop);
	}

	return used_operands;
}

void NewSpillPass::process_block(MachineBasicBlock* block)
{
	LOG1(nl << Yellow << "Processing block " << *block << " ");
	DEBUG1(print_ptr_container(dbg(), block->pred_begin(), block->pred_end()));
	LOG1(reset_color << nl);

	unsigned predecessor_count = block->pred_size();
	if (predecessor_count == 0) {
		worksets_exit.emplace(block, mof->EmptySet());
		spillsets_exit.emplace(block, mof->EmptySet());
	}
	else if (predecessor_count == 1) {
		auto predecessor = *block->pred_begin();

		worksets_exit.emplace(block, worksets_exit.at(predecessor));
		spillsets_exit.emplace(block, spillsets_exit.at(predecessor));
	}
	else {
		auto workset = compute_workset(block);

		worksets_exit.emplace(block, workset);
		spillsets_exit.emplace(block, compute_spillset(block, workset));
	}

	// Make a copy of the current workset/spillset and place them as entry for the
	// current block
	auto& workset = worksets_exit.at(block);
	auto& spillset = spillsets_exit.at(block);

	worksets_entry.emplace(block, workset);
	spillsets_entry.emplace(block, spillset);

	LOG2_NAMED_CONTAINER("Workset:  ", workset);
	LOG2_NAMED_CONTAINER("Spillset: ", spillset);

	for (auto i = block->begin(), e = block->end(); i != e; ++i) {
		auto instruction = *i;
		LOG2(Magenta << "\nProcessing instruction: " << *instruction << reset_color << nl);

		auto used_ops = used_operands(mof, current_rc, instruction);
		auto defined_ops = defined_operands(mof, current_rc, instruction);

		auto uses = used_ops - workset;

		workset |= uses;
		spillset |= uses;

		const unsigned reg_count = current_rc->count();
		auto mi_iter = block->convert(i);
		auto mi_iter_next = block->convert(std::next(i));
		limit(workset, spillset, mi_iter, mi_iter, reg_count);
		limit(workset, spillset, mi_iter_next, mi_iter, reg_count - defined_ops.size());

		workset |= defined_ops;

		for (auto& operand : uses) {
			LOG2(Yellow << "Reloading " << operand << reset_color << nl);
			spill_info.add_reload(&operand, block->convert(i));
		}
	}
}

void NewSpillPass::fix_block_boundaries()
{
	LOG1(nl << Yellow << "Fixing block boundaries" << reset_color << nl);
	auto RPO = get_Artifact<ReversePostOrderPass>();

	for (const auto block : *RPO) {
		LOG2(nl << Yellow << "Processing block " << *block << " ");
		DEBUG2(print_ptr_container(dbg(), block->pred_begin(), block->pred_end()));
		LOG2(reset_color << nl);

		auto& ws_entry = worksets_entry.at(block);
		auto spill_entry = mof->EmptySet();
		for (auto i = block->pred_begin(), e = block->pred_end(); i != e; ++i) {
			spill_entry |= spillsets_exit.at(*i);
		}
		spill_entry &= ws_entry;

		for (auto i = block->pred_begin(), e = block->pred_end(); i != e; ++i) {
			auto predecessor = *i;
			LOG2(nl << Yellow << "\t Predecessor: " << *predecessor << reset_color << nl);

			// Variables that are in the entry set of block, but not in the exit set
			// of the predecessor need to be reloaded
			auto& ws_exit = worksets_exit.at(predecessor);
			auto reload_ops = ws_entry - ws_exit;

			auto& spill_exit = spillsets_exit.at(predecessor);

			LOG3_NAMED_CONTAINER("workset entry:  ", ws_entry);
			LOG3_NAMED_CONTAINER("workset exit:   ", ws_exit);
			LOG3_NAMED_CONTAINER("spillset entry: ", spill_entry);
			LOG3_NAMED_CONTAINER("spillset_exit:  ", spill_exit);

			// Variables that are in the working set of the predecessor, but not in
			// the entry set of this block, might need additional spilling if they are
			// live-in
			auto difference = ws_exit - ws_entry;

			for (const auto& operand : difference) {
				bool is_live_in =
				    get_Artifact<NewLivetimeAnalysisPass>()->get_live_in(block).contains(&operand);
				if (is_live_in && !spill_exit.contains(&operand)) {
					assert(false && "Operand needs additioanl spilling!");
				}
			}

			// Variables that are in the entry spillset of the block, but not in the
			// exit spill set of the predecessor, need to be spilled if they are also in the
			// exit workset of the predecessor
			auto spill_ops = (spill_entry - spill_exit) & ws_exit;

			for (auto& operand : ws_entry) {
				auto phi_instr = is_defined_by_phi(&operand, block);
				auto& op = phi_instr ? *phi_instr->get(block->get_predecessor_index(predecessor)).op
				                     : operand;

				if (ws_exit.contains(&op)) {
					// We might have to spill on this path
					if (spill_entry.contains(&op) && !spill_exit.contains(&op)) {
						assert(false && "TOOD: We might need top spill some shit no this path!");
					}
				}
				else {
					LOG3("Reloading " << op << nl);
					spill_info.add_reload(&op, predecessor->mi_last());
				}
			}

			// LOG3_NAMED_CONTAINER("operands to reload:  ", reload_ops);
			// LOG3_NAMED_CONTAINER("operands to spill:   ", spill_ops);
		}
	}
}

bool NewSpillPass::run(JITData& JD)
{
	LOG1(nl << BoldYellow << "Running SpillPass" << reset_color << nl);

	backend = JD.get_Backend();
	ssm = JD.get_StackSlotManager();
	mof = JD.get_MachineOperandFactory();

	auto registerinfo = backend->get_RegisterInfo();
	auto RPO = get_Artifact<ReversePostOrderPass>();

	LOG1("Running pass " << registerinfo->class_count() << " times. Once for each register class.");

	for (unsigned i = 0; i < registerinfo->class_count(); ++i) {
		current_rc = &registerinfo->get_class(i);
		current_rc_idx = i;

		for (const auto block : *RPO) {
			process_block(block);
		}

		fix_block_boundaries();

		worksets_exit.clear();
		spillsets_exit.clear();

		worksets_entry.clear();
		spillsets_entry.clear();
	}

	spill_info.insert_instructions(*backend, *ssm);

	return true;
}

// pass usage
PassUsage& NewSpillPass::get_PassUsage(PassUsage& PU) const
{
	PU.requires<NewLivetimeAnalysisPass>();
	PU.requires<ReversePostOrderPass>();
	PU.requires<MachineDominatorPass>();
	PU.requires<MachineLoopPass>();
	PU.requires<LoopPressurePass>();

	PU.modifies<LIRInstructionScheduleArtifact>();
	
	return PU;
}

// register pass
static PassRegistry<NewSpillPass> X("NewSpillPass");

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
