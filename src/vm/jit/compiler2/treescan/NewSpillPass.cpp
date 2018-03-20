/* src/vm/jit/compiler2/treescan/NewSpillPass.cpp - NewSpillPass

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

#include "vm/jit/compiler2/treescan/NewSpillPass.hpp"

#include <deque>

#include "toolbox/logging.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineDominatorPass.hpp"
#include "vm/jit/compiler2/MachineInstruction.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/ReversePostOrderPass.hpp"
#include "vm/jit/compiler2/treescan/LogHelper.hpp"
#include "vm/jit/compiler2/treescan/LoopPressurePass.hpp"
#include "vm/jit/compiler2/treescan/NewLivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/treescan/SSAReconstructor.hpp"

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

SpillInfo::SpilledOperand& SpillInfo::get_spilled_operand(MachineOperand* operand)
{
	auto iter = spilled_operands.find(operand->get_id());
	if (iter == spilled_operands.end()) {
		auto spilled_op = std::make_unique<SpilledOperand>(operand);
		iter = spilled_operands.emplace(operand->get_id(), std::move(spilled_op)).first;
	}
	return *iter->second;
}

void SpillInfo::add_spill(MachineOperand* operand, const MIIterator& position)
{
	// Check if other spills dominate - or are dominated by - this spill
	auto& spilled_op = get_spilled_operand(operand);
	for (auto i = spilled_op.spill_positions.begin(); i != spilled_op.spill_positions.end();) {
		const auto& existing_spill_pos = *i;

		// No need to add this spill if it is dominated by another
		if (sp->strictly_dominates(existing_spill_pos, position)) {
			LOG3("dominated by other spill, not added!\n");
			return;
		}

		// Remove spills that the new position dominates
		if (sp->strictly_dominates(position, existing_spill_pos)) {
			LOG3("removing old spill since it is dominated by another!\n");
			i = spilled_op.spill_positions.erase(i);
		}
		else {
			++i;
		}
	}
	spilled_op.spill_positions.push_back(position);
}

void SpillInfo::add_reload(MachineOperand* operand, const MIIterator& position)
{
	auto& spilled_operand = get_spilled_operand(operand);
	spilled_operand.reload_positions.push_back(position);
}

void SpillInfo::add_spill_phi(MachinePhiInst* instruction)
{
	auto operand = instruction->get_result().op;
	auto iter = spilled_operands.find(operand->get_id());
	assert_msg(iter == spilled_operands.end(),
	           "Multiple spill positions for same variable not implemented!");

	auto spilled_op =
	    std::make_unique<SpilledOperand>(operand, instruction->get_block()->mi_first());
	spilled_op->spilled_phi = true;
	spilled_op->phi_instruction = instruction;
	spilled_operands.emplace(operand->get_id(), std::move(spilled_op));

	// Add all arguments as separate spills
	auto block = instruction->get_block();
	for (auto i = block->pred_begin(), e = block->pred_end(); i != e; ++i) {
		auto predecessor = *i;
		auto idx = block->get_predecessor_index(predecessor);
		auto argument_op = instruction->get(idx).op;

		add_spill(argument_op, predecessor->mi_last_insertion_point());
	}
}

void SpillInfo::insert_instructions(const Backend& backend, StackSlotManager& ssm) const
{
	// Assign a spill slot to each spilled_operands
	// TODO: Replace this with VirtualStackslots so we can do "register assignment" on them
	//       and re-use spill slots (especially for phis)

	// Currently we simply assume that program is in CSSA-form, so PHI arguments do not
	// interfere and we can assign the same stackslot to all arguments and the result of a phi
	for (const auto& pair : spilled_operands) {
		if (!pair.second->spilled_phi)
			continue;

		pair.second->stackslot = ssm.create_slot(pair.second->operand->get_type());
		for (const auto& desc : *pair.second->phi_instruction) {
			auto& spilled_phi_argument = *spilled_operands.at(desc.op->get_id());
			spilled_phi_argument.stackslot = pair.second->stackslot;
		}
	}

	// Assign remaining stackslots, again, this should be replaced by virtual ones, and then
	// done during RegisterAssignmentPass
	for (const auto& pair : spilled_operands) {
		if (pair.second->stackslot)
			continue;
		pair.second->stackslot = ssm.create_slot(pair.second->operand->get_type());
	}

	// Collect all spilled_opperands in a new list and partition it that non-phi related
	// spills/reloads come first
	// TODO: This fixes a certain example, we have to show that this is either necessary and/or
	// enough
	std::vector<SpilledOperand*> spilled_operands_list;
	for (const auto& pair : spilled_operands) {
		spilled_operands_list.push_back(pair.second.get());
	}
	std::stable_partition(spilled_operands_list.begin(), spilled_operands_list.end(),
	                      [](auto spilled_op) { return !spilled_op->spilled_phi; });

	SSAReconstructor ssa_reconstructor(sp);
	for (auto* spilled_op_ptr : spilled_operands_list) {
		auto& spilled_op = *spilled_op_ptr;
		auto operand = spilled_op.operand;
		std::vector<Occurrence> new_definitions;

		LOG1(nl << Yellow << "Inserting spill/reload instructions for " << operand << reset_color
		        << nl);

		// For non-PHIs we simply insert a move from the operand to the stackslot
		// before spill position. For PHIs we replace all operands of the PHI with
		// their stackslots and let the deconstruction do the move
		if (!spilled_op.spilled_phi) {
			for (const auto& spill_position : spilled_op.spill_positions) {
				auto spill_instr = backend.create_Move(operand, spilled_op.stackslot);

				LOG2("Inserting " << spill_instr << nl);
				insert_after(spill_position, spill_instr);
			}
		}
		else {
			LOG2("Phi instruction BEFORE replace: " << *spilled_op.phi_instruction << nl);
			for (auto& desc : *spilled_op.phi_instruction) {
				desc.op = spilled_operands.at(desc.op->get_id())->stackslot;
			}
			spilled_op.phi_instruction->get_result().op = spilled_op.stackslot;
			LOG2("Phi instruction AFTER replace:  " << *spilled_op.phi_instruction << nl);
		}

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

		ssa_reconstructor.add_new_definitions(original_definition, new_definitions);
	}
	replace_registers_with_stackslots_for_deopt();
	ssa_reconstructor.reconstruct();
}

void SpillInfo::replace_registers_with_stackslots_for_deopt() const
{
	for (auto block : *sp->get_Artifact<ReversePostOrderPass>()) {
		for (auto i = block->begin(), e = block->end(); i != e; ++i) {
			auto instruction = *i;
			if (instruction->to_MachineReplacementPointInst() == nullptr)
				continue;

			for (auto& desc : *instruction) {
				auto iter = spilled_operands.find(desc.op->get_id());
				if (iter == spilled_operands.end())
					continue;

				// Found variable that is spilled, check if there are any spill_positions
				// that dominate this ReplacementPointInst
				auto current_pos = block->convert(i);
				auto& spilled_op = *iter->second;
				auto dominates =
				    std::any_of(spilled_op.spill_positions.begin(),
				                spilled_op.spill_positions.end(), [&](auto& spill_pos) {
					                return sp->strictly_dominates(spill_pos, current_pos);
				                });

				if (dominates) {
					LOG1(Cyan << "Replacing " << desc.op << " with " << spilled_op.stackslot
					          << " in MachineReplacementPointInst\n"
					          << reset_color);
					desc.op = spilled_op.stackslot;
				}
			}
		}
	}
}

void NewSpillPass::limit(OperandSet& workset,
                         OperandSet& spillset,
                         const MIIterator& mi_iter,
                         MIIterator mi_spill_before,
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
			spill_info.add_spill(operand, --mi_spill_before);
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

NewSpillPass::Availability NewSpillPass::available_in_all_preds(MachineBasicBlock* block,
                                                                MachineOperand* op,
                                                                MachinePhiInst* phi_instr)
{
	bool available_everywhere = true;
	bool available_nowhere = true;

	for (auto i = block->pred_begin(), e = block->pred_end(); i != e; ++i) {
		MachineOperand* operand = op;
		if (phi_instr) {
			auto idx = block->get_predecessor_index(*i);
			operand = phi_instr->get(idx).op;
		}

		bool found = false;
		for (const auto& pred_op : worksets_exit.at(*i)) {
			if (!pred_op.aquivalent(*operand))
				continue;

			found = true;
			available_nowhere = false;
			break;
		}

		available_everywhere &= found;
	}

	if (available_everywhere) {
		return Availability::Everywhere;
	}
	else if (available_nowhere) {
		return Availability::Nowhere;
	}
	else {
		return Availability::Partly;
	}
}

NewSpillPass::Location NewSpillPass::to_take_or_not_to_take(MachineBasicBlock* block,
                                                            MachineOperand* operand,
                                                            Availability available)
{
	Location location;
	location.time_type = Location::TimeType::Infinity;
	location.operand = operand;
	location.spilled = false;

	auto LTA = get_Artifact<NewLivetimeAnalysisPass>();
	auto next_use_set_ptr = LTA->next_use_set_from(block->front());
	auto next_use_set = *next_use_set_ptr;

	if (!next_use_set.contains(*operand)) {
		LOG3("\t" << operand << " not taken (dead)\n");
		return location;
	}

	location.time_type = Location::TimeType::Normal;
	location.time = next_use_set[*operand];
	if (available == Availability::Everywhere) {
		LOG3("\t" << operand << " taken (" << location.time << ", live in all pred)\n");
		return location;
	}
	else if (available == Availability::Nowhere) {
		LOG3("\t" << operand << " not taken (" << location.time << ", live in no pred)\n");
		location.time_type = Location::TimeType::Infinity;
		return location;
	}

	return location;
}

OperandSet NewSpillPass::compute_workset(MachineBasicBlock* block)
{
	auto workset = mof->EmptySet();

	auto MLP = get_Artifact<MachineLoopPass>();
	auto LTA = get_Artifact<NewLivetimeAnalysisPass>();

	bool all_pred_known = std::all_of(block->pred_begin(), block->pred_end(), [&](const auto pred) {
		return worksets_exit.find(pred) != worksets_exit.end();
	});

	auto starters = mof->EmptySet();
	auto delayed = mof->EmptySet();

	// Handle PHI instructions first, if all operands of a PHI are available,
	// put the result into "take"
	for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
		auto phi_instr = *i;

		if (!current_rc->handles_type(phi_instr->get_result().op->get_type()))
			continue;

		Availability available = Availability::Unknown;
		if (all_pred_known) {
			available = available_in_all_preds(block, nullptr, phi_instr);
		}

		auto operand = phi_instr->get_result().op;
		auto location = to_take_or_not_to_take(block, operand, available);
		if (location.time_type != Location::TimeType::Infinity) {
			if (location.time_type == Location::TimeType::Pending) {
				delayed.add(operand);
			}
			else {
				starters.add(operand);
			}
		}
		else {
			LOG3("Spilling PHI instruction: " << *phi_instr << nl);
			spill_info.add_spill_phi(phi_instr);
		}
	}

	// Handle Live-ins now, since we already handled phi definitions before,
	// we can ignore them now
	auto live_in = LTA->get_live_in(block) - LTA->mbb_phi_defs(block);
	std::for_each(live_in.begin(), live_in.end(), [&](auto& operand) {
		if (!current_rc->handles_type(operand.get_type()))
			return;

		Availability available = Availability::Unknown;
		if (all_pred_known) {
			available = available_in_all_preds(block, &operand, nullptr);
		}

		auto location = to_take_or_not_to_take(block, &operand, available);
		if (location.time_type != Location::TimeType::Infinity) {
			if (location.time_type == Location::TimeType::Pending) {
				delayed.add(&operand);
			}
			else {
				starters.add(&operand);
			}
		}
	});

	const unsigned regs_count = current_rc->count();
	unsigned loop_pressure = 0;
	auto iter_pair = MLP->get_Loops_from_header(block);
	for (auto i = iter_pair.first, e = iter_pair.second; i != e; ++i) {
		auto current_loop_pressure = (*i)->get_register_pressures()[current_rc_idx];
		loop_pressure = std::max(loop_pressure, current_loop_pressure);
	}

	assert_msg(delayed.size() <= loop_pressure,
	           "Delayed set contains more operands then the loop pressure indicates");

	int free_slots = regs_count - starters.size();
	int free_pressure_slots = regs_count - (loop_pressure - delayed.size());
	free_slots = std::min(free_slots, free_pressure_slots);

	LOG1("Loop pressure " << loop_pressure << ", taking " << free_slots << " delayed operands\n");

	// If the loop pressure inside the loop is low, we can take additional values and let
	// them live through the loop
	auto& def_use_chains = LTA->get_def_use_chains();

	if (free_slots > 0) {
		auto delayed_list_ptr = delayed.ToList();
		auto& delayed_list = *delayed_list_ptr;
		sort_by_next_use(delayed_list, block->front());

		std::for_each(delayed_list.begin(), delayed_list.end(), [&](auto& operand) {
			if (free_slots == 0)
				return;

			auto& occurrence = def_use_chains.get_definition(operand);
			if (occurrence.phi_instr != nullptr) {
				LOG1("\tdelayed PHI " << operand << " taken\n");
				starters.add(operand);
				--free_slots;
			}
			else {
				// Do not use values that are dead in a known predecessor
				bool should_take =
				    std::all_of(block->pred_begin(), block->pred_end(), [&](auto pred) {
					    auto ws_iter = worksets_exit.find(pred);
					    if (ws_iter == worksets_exit.end())
						    return true;

					    if (!ws_iter->second.contains(operand)) {
						    LOG1("\tdelayed " << operand << " not live in pred " << *pred << nl);
						    return false;
					    }
					    return true;
				    });

				if (should_take) {
					LOG1("\tdelayed operand " << operand << " taken\n");
					starters.add(operand);
					--free_slots;
				}
			}
		});
	}

	// Spill the remaining PHIs that are in the delayed list
	std::for_each(delayed.begin(), delayed.end(), [&](auto& operand) {
		auto& occurrence = def_use_chains.get_definition(&operand);
		if (occurrence.phi_instr == nullptr || occurrence.block() != block) {
			return;
		}

		LOG1("Spilling delayed PHI: " << occurrence.phi_instr << nl);
		spill_info.add_spill_phi(occurrence.phi_instr);
	});

	auto starters_list_ptr = starters.ToList();
	auto& starters_list = *starters_list_ptr;
	sort_by_next_use(starters_list, block->front());

	// Copy the best ones from starters to the workset
	unsigned workset_count = std::min<unsigned>(starters.size(), regs_count);
	for (std::size_t i = 0; i < workset_count; ++i) {
		workset.add(starters_list[i]);
	}

	// Spill the remaining PHIs that are in the starters list but not in the workset
	for (std::size_t i = workset_count, e = starters.size(); i < e; ++i) {
		auto& occurrence = def_use_chains.get_definition(starters_list[i]);
		if (occurrence.phi_instr == nullptr || occurrence.block() != block) {
			continue;
		}

		LOG1("Spilling PHI: " << occurrence.phi_instr << nl);
		spill_info.add_spill_phi(occurrence.phi_instr);
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

	OperandSet workset_without_phis(workset);
	for (const auto& operand : workset) {
		if (is_defined_by_phi(&operand, block)) {
			workset_without_phis.remove(&operand);
		}
	}

	return spillset_union & workset_without_phis;
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
		if (!reg_alloc_considers_instruction(instruction)) {
			// Ignore MDeopts
			// TODO: Check if its ok that variables wont get reloaded, but instead
			//       the deopt instruction uses the stackslot where the value resides currently
			continue;
		}

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

			auto& ws_exit = worksets_exit.at(predecessor);
			auto& spill_exit = spillsets_exit.at(predecessor);

			LOG3_NAMED_CONTAINER("workset entry:  ", ws_entry);
			LOG3_NAMED_CONTAINER("workset exit:   ", ws_exit);
			LOG3_NAMED_CONTAINER("spillset entry: ", spill_entry);
			LOG3_NAMED_CONTAINER("spillset_exit:  ", spill_exit);

			// Variables that are in the working set of the predecessor, but not in
			// the entry set of this block, might need additional spilling if they are
			// live-in
			auto difference = ws_exit - ws_entry;

			for (auto& operand : difference) {
				bool is_live_in =
				    get_Artifact<NewLivetimeAnalysisPass>()->get_live_in(block).contains(&operand);
				if (is_live_in && !spill_exit.contains(&operand)) {
					auto insert_pos = block->mi_first();
					if (block->pred_size() > 1)
						insert_pos = --predecessor->mi_last_insertion_point();
					LOG3("Spilling " << operand << nl);
					spill_info.add_spill(&operand, insert_pos);
				}
			}

			// Variables that are in the entry spillset of the block, but not in the
			// exit spill set of the predecessor, need to be spilled if they are also in the
			// exit workset of the predecessor
			for (auto& operand : ws_entry) {
				auto phi_instr = is_defined_by_phi(&operand, block);
				auto& op = phi_instr ? *phi_instr->get(block->get_predecessor_index(predecessor)).op
				                     : operand;

				if (ws_exit.contains(&op)) {
					// We might have to spill on this path
					if (spill_entry.contains(&op) && !spill_exit.contains(&op)) {
						LOG3("Spilling " << op << nl);
						spill_info.add_spill(&op, --predecessor->mi_last_insertion_point());
					}
				}
				else {
					LOG3("Reloading " << op << nl);
					spill_info.add_reload(&op, predecessor->mi_last_insertion_point());
				}
			}
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

/**
 * Returns true, iff a instruction dominates the other.
 * If the basic block is the same, the MIIterator less then comparison is used,
 * otherwise we look it up in the dominator tree
 */
bool NewSpillPass::strictly_dominates(const MIIterator& a, const MIIterator& b)
{
	auto block_a = (*a)->get_block();
	auto block_b = (*b)->get_block();

	if (block_a != block_b) {
		auto mdom = get_Artifact<MachineDominatorPass>();
		auto result = mdom->dominates(block_a, block_b);
		LOG3("Does " << *block_a << " dominate " << *block_b << "?: " << result << nl);
		return mdom->dominates(block_a, block_b);
	}

	return a < b;
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
