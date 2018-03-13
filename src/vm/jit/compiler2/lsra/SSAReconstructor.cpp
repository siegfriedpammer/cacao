/* src/vm/jit/compiler2/lsra/LoopPressurePass.cpp - LoopPressurePass

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

#include "vm/jit/compiler2/lsra/SSAReconstructor.hpp"

#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/compiler2/MachineOperand.hpp"
#include "vm/jit/compiler2/ReversePostOrderPass.hpp"
#include "vm/jit/compiler2/lsra/LogHelper.hpp"
#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/lsra/NewSpillPass.hpp"

#define DEBUG_NAME "compiler2/SSAReconstructor"

namespace cacao {
namespace jit {
namespace compiler2 {

SSAReconstructor::SSAReconstructor(NewSpillPass* pass) : sp(pass) {}

void SSAReconstructor::add_new_definitions(const Occurrence& original_def,
                                           const std::vector<Occurrence>& new_definitions)
{
	definitions.emplace_back(original_def, new_definitions);

	for (const auto& occurrence : new_definitions) {
		reverse_def.emplace(occurrence.operand->get_id(), original_def.operand);
	}
}

void SSAReconstructor::prepare_operand_sets()
{
	for (const auto& definition : definitions) {
		original_definitions_set.push_back(definition.first.operand->get_id());
		for (const auto& occurrence : definition.second) {
			new_definitions_set.push_back(occurrence.operand->get_id());
		}
	}
}

void SSAReconstructor::reconstruct()
{
	prepare_operand_sets();

	if (DEBUG_COND_N(1)) {
		for (const auto& definition : definitions) {
			std::vector<const MachineOperand*> defined_ops;
			for (const auto& occurrence : definition.second) {
				defined_ops.push_back(occurrence.operand);
			}

			LOG1(Cyan << "Reconstructing SSA for " << definition.first.operand << reset_color
			          << nl);
			LOG1_NAMED_PTR_CONTAINER("New definitions: ", defined_ops);
		}
	}

	auto RPO = sp->get_Artifact<ReversePostOrderPass>();
	for (auto block : *RPO) {
		LOG1(Yellow << "Handling block " << *block << reset_color << nl);
		for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
			LOG2(Blue << "Handling instruction: " << *(*i) << reset_color << nl);
			handle_definitions(*i);
		}

		for (auto instruction : *block) {
			LOG2(Blue << "Handling instruction: " << *instruction << reset_color << nl);
			handle_uses(instruction);
			handle_definitions(instruction);
		}

		finished_blocks.insert(block->get_id());
		try_seal_block(block);
		std::for_each(block->back()->successor_begin(), block->back()->successor_end(),
		              [&](auto successor) { this->try_seal_block(successor); });
	}
}

template <typename Container, typename ValueType>
static bool contains(const Container& c, const ValueType& v)
{
	return std::find(std::begin(c), std::end(c), v) != std::end(c);
}

void SSAReconstructor::handle_uses(MachineInstruction* instruction)
{
	auto uses_lambda = [&](auto& desc) {
		if (!desc.op->to_Register())
			return;

		if (!contains(original_definitions_set, desc.op->get_id()))
			return;

		auto operand = this->read_variable(desc.op, instruction->get_block());
		if (!desc.op->aquivalent(*operand)) {
			LOG(Cyan << "Replacing " << desc.op << " with " << operand << " in " << *instruction
			         << reset_color << nl);
			desc.op = operand;
			uses[operand->get_id()].push_back(&desc);
		}
	};
	std::for_each(instruction->begin(), instruction->end(), uses_lambda);
	std::for_each(instruction->dummy_begin(), instruction->dummy_end(), uses_lambda);
}

void SSAReconstructor::handle_definitions(MachineInstruction* instruction)
{
	auto def_lambda = [&](auto& desc) {
		if (!desc.op->to_Register())
			return;

		if (contains(original_definitions_set, desc.op->get_id())) {
			write_variable(desc.op, instruction->get_block(), desc.op);
			return;
		}

		if (!contains(new_definitions_set, desc.op->get_id()))
			return;

		auto original_def = this->reverse_def.at(desc.op->get_id());
		write_variable(original_def, instruction->get_block(), desc.op);
	};
	std::for_each(instruction->results_begin(), instruction->results_end(), def_lambda);
}

void SSAReconstructor::write_variable(MachineOperand* variable,
                                      MachineBasicBlock* block,
                                      MachineOperand* operand)
{
	current_def[variable->get_id()][block->get_id()] = operand;
	LOG3("write_variable [" << variable << "][" << *block << "] = " << operand << nl);
}

MachineOperand* SSAReconstructor::read_variable(MachineOperand* variable, MachineBasicBlock* block)
{
	LOG3("read_variable [" << variable << "][" << *block << "]\n");

	auto& variable_current_def = current_def[variable->get_id()];
	auto iter = variable_current_def.find(block->get_id());
	if (iter != variable_current_def.end()) {
		// Local value numbering
		return iter->second;
	}
	// Global value numbering
	return read_variable_recursive(variable, block);
}

MachineOperand* SSAReconstructor::read_variable_recursive(MachineOperand* variable,
                                                          MachineBasicBlock* block)
{
	LOG3("read_variable_recursive [" << variable << "][" << *block << "]\n");

	MachineOperand* operand = nullptr;

	if (!is_sealed(block)) {
		auto instr = new MachinePhiInst(block->pred_size(), variable->get_type(), nullptr, sp->mof);
		instr->set_block(block);
		block->insert_phi(instr);

		operand = instr->get_result().op;
		incomplete_phis[block->get_id()].emplace(variable, instr);
	}
	else if (block->pred_size() == 1) {
		operand = read_variable(variable, *block->pred_begin());
	}
	else {
		auto instr = new MachinePhiInst(block->pred_size(), variable->get_type(), nullptr, sp->mof);
		instr->set_block(block);
		block->insert_phi(instr);

		write_variable(variable, block, instr->get_result().op);
		operand = add_phi_operands(variable, instr);
	}

	write_variable(variable, block, operand);

	return operand;
}

MachineOperand* SSAReconstructor::add_phi_operands(MachineOperand* variable, MachinePhiInst* instr)
{
	auto block = instr->get_block();
	for (std::size_t i = 0, e = block->pred_size(); i < e; ++i) {
		auto operand = read_variable(variable, block->get_predecessor(i));
		instr->get(i).op = operand;

		LOG3("\tPredecessor " << *block->get_predecessor(i) << " has operand " << operand << nl);
	}
	LOG3("After adding operands to PHI " << instr << nl);
	return try_remove_trivial_phi(instr);
}

MachineOperand* SSAReconstructor::try_remove_trivial_phi(MachinePhiInst* instr)
{
	LOG3("Try to remove trivial PHI: " << *instr << nl);

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

	// Replace all uses of the PHI result with the new operand
	for (auto desc : uses[instr->get_result().op->get_id()]) {
		LOG(Cyan << "Replacing " << desc->op << " with " << same << " in "
		         << desc->get_MachineInstruction() << reset_color << nl);
		desc->op = same;
		uses[same->get_id()].push_back(desc);
	}

	// Update the current_def table, so future read_variable calls will have the new value
	for (auto& orig_to_new : current_def) {
		for (auto& block_to_def : orig_to_new.second) {
			if (block_to_def.second == instr->get_result().op) {
				LOG(Cyan << "Updating current_def of " << instr->get_result().op << " to " << same
				         << reset_color << nl);
				block_to_def.second = same;
			}
		}
	}

	auto block = instr->get_block();
	for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
		if (*i == instr) {
			LOG(Cyan << "Removing PHI instruction from " << *block << ": " << instr << reset_color
			         << nl);
			block->phi_erase(i);
			break;
		}
	}

	// Try to remove other PHIs if they have become trivial
	for (auto desc : uses[instr->get_result().op->get_id()]) {
		auto phi_instr = desc->get_MachineInstruction()->to_MachinePhiInst();
		if (phi_instr) {
			try_remove_trivial_phi(phi_instr);
		}
	}

	return same;
}

bool SSAReconstructor::is_sealed(MachineBasicBlock* block) const
{
	return sealed_blocks.count(block->get_id()) == 1;
}

bool SSAReconstructor::is_finished(MachineBasicBlock* block) const
{
	return finished_blocks.count(block->get_id()) == 1;
}

void SSAReconstructor::try_seal_block(MachineBasicBlock* block)
{
	if (is_sealed(block))
		return;

	for (auto i = block->pred_begin(), e = block->pred_end(); i != e; ++i) {
		if (!is_finished(*i)) {
			return;
		}
	}

	seal_block(block);
}

void SSAReconstructor::seal_block(MachineBasicBlock* block)
{
	LOG1("Sealing block " << *block << nl);
	for (const auto& variable_to_phi : incomplete_phis[block->get_id()]) {
		add_phi_operands(variable_to_phi.first, variable_to_phi.second);
	}
	sealed_blocks.insert(block->get_id());
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
