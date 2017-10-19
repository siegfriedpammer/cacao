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
#include "vm/jit/compiler2/MachineOperandFactory.hpp"
#include "vm/jit/compiler2/MachineRegister.hpp"
#include "vm/jit/compiler2/ReversePostOrderPass.hpp"
#include "vm/jit/compiler2/lsra/LogHelper.hpp"
#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/lsra/NewSpillPass.hpp"

#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"

#define DEBUG_NAME "compiler2/RegisterAssignment"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

static bool op_cmp(MachineOperand* lhs, MachineOperand* rhs)
{
	return lhs->aquivalence_less(*rhs);
}

} // end anonymous namespace

OStream& operator<<(OStream& OS, const RegisterAssignment::Variable& variable)
{
	OS << *variable.operand;
	return OS;
}

void RegisterAssignment::initialize_physical_registers(Backend* backend)
{
	initialize_physical_registers_for_class(backend, Type::ByteTypeID);
	initialize_physical_registers_for_class(backend, Type::CharTypeID);
	initialize_physical_registers_for_class(backend, Type::ShortTypeID);
	initialize_physical_registers_for_class(backend, Type::IntTypeID);
	initialize_physical_registers_for_class(backend, Type::LongTypeID);
	initialize_physical_registers_for_class(backend, Type::ReferenceTypeID);
	initialize_physical_registers_for_class(backend, Type::FloatTypeID);
	initialize_physical_registers_for_class(backend, Type::DoubleTypeID);
}

void RegisterAssignment::initialize_physical_registers_for_class(Backend* backend,
                                                                 Type::TypeID type)
{
	const RegisterClass* regclass;

	for (unsigned i = 0; i < backend->get_RegisterInfo()->class_count(); ++i) {
		const auto& rc = backend->get_RegisterInfo()->get_class(i);
		if (rc.handles_type(type)) {
			regclass = &rc;
			break;
		}
	}

	/// @todo only works on x86_64, this needs to be arch independent!!!
	RegisterSet registers;
	for (const auto reg : regclass->get_All()) {
		auto native_reg = x86_64::cast_to<x86_64::X86_64Register>(reg);
		registers.push_back(new NativeRegister(type, native_reg));
	}

	std::sort(registers.begin(), registers.end(), op_cmp);

	physical_registers.emplace(type, registers);
}

void RegisterAssignment::initialize_variables_for(MachineBasicBlock* block, MachineBasicBlock* idom)
{
	if (idom) {
		auto& idom_variables = allocated_variables_map.at(idom);
		allocated_variables_map.emplace(block, AllocatedVariableSet(idom_variables));
	}
	else {
		allocated_variables_map.emplace(block, AllocatedVariableSet());
	}
}

void RegisterAssignment::intersect_variables_with(MachineBasicBlock* block,
                                                  const OperandSet& live_in)
{
	auto& variables = allocated_variables_map.at(block);

	if (DEBUG_COND_N(3)) {
		LOG3(nl);
		LOG3_NAMED_CONTAINER("Initial allocated variables (" << *block << "): ", variables);
		LOG3_NAMED_CONTAINER("LiveIn                      (" << *block << "): ", live_in);
	}

	for (auto i = variables.begin(); i != variables.end();) {
		auto var = *i;
		if (!live_in.contains(var->operand)) {
			i = variables.erase(i);
		}
		else {
			++i;
		}
	}

	if (DEBUG_COND_N(3)) {
		LOG3_NAMED_CONTAINER("Allocated vars intersected  (" << *block << "): ", variables);
	}
}

void RegisterAssignment::add_allocated_variable(MachineBasicBlock* block, MachineOperand* operand)
{
	auto& variable = variable_for(operand);
	allocated_variables_map.at(block).push_back(&variable);
}

void RegisterAssignment::remove_allocated_variable(MachineBasicBlock* block,
                                                   MachineOperand* operand)
{
	auto& variable = variable_for(operand);
	auto& allocated_variables = allocated_variables_map.at(block);

	auto iter = std::find(allocated_variables.begin(), allocated_variables.end(), &variable);
	if (iter != allocated_variables.end()) {
		allocated_variables.erase(iter);
	}
}

RegisterAssignment::ColorPair RegisterAssignment::choose_color(MachineBasicBlock* block,
                                                               MachineInstruction* instruction,
                                                               MachineOperandDesc& descriptor)
{
	RegisterSet allowed_ccolors;

	auto allocated_ccolors = ccolors(block);
	auto colors = allowed_colors(instruction, &variable_for(descriptor.op));

	std::set_difference(colors.begin(), colors.end(), allocated_ccolors.begin(),
	                    allocated_ccolors.end(), std::back_inserter(allowed_ccolors), op_cmp);

	return choose_color(block, instruction, descriptor, allowed_ccolors);
}

RegisterAssignment::ColorPair RegisterAssignment::choose_color(MachineBasicBlock* block,
                                                               MachineInstruction* instruction,
                                                               MachineOperandDesc& descriptor,
                                                               const RegisterSet& allowed_ccolors)
{
	RegisterSet allowed_gcolors;

	auto& physical_registers = physical_registers_for(descriptor.op->get_type());
	auto allocated_gcolors = gcolors(block);
	std::set_difference(physical_registers.cbegin(), physical_registers.cend(),
	                    allocated_gcolors.begin(), allocated_gcolors.end(),
	                    std::back_inserter(allowed_gcolors), op_cmp);

	auto& variable = variable_for(descriptor.op);

	if (DEBUG_COND_N(3)) {
		LOG3("[" << descriptor.op << "]: Allowed Local Colors:  ");
		print_ptr_container(dbg(), allowed_ccolors.begin(), allowed_ccolors.end());
		LOG3("\n[" << descriptor.op << "]: Allowed Global Colors: ");
		print_ptr_container(dbg(), allowed_gcolors.begin(), allowed_gcolors.end());
		LOG3(nl);
	}

	// We have reached a definition, set both gcolor and ccolor
	if (variable.gcolor == nullptr && variable.ccolor == nullptr) {
		RegisterSet intersection;
		std::set_intersection(allowed_ccolors.begin(), allowed_ccolors.end(),
		                      allowed_gcolors.begin(), allowed_gcolors.end(),
		                      std::back_inserter(intersection), op_cmp);

		LOG3("[" << descriptor.op << "]: Intersection:          ");
		DEBUG3(print_ptr_container(dbg(), intersection.begin(), intersection.end()));
		LOG3(nl);

		auto color = pick(intersection);
		if (color) {
			return {color, color};
		}
		else {
			return {pick(allowed_gcolors), pick(allowed_ccolors)};
		}
	}

	// Returns the new ccolor (required for repairing)
	if (variable.gcolor != nullptr && variable.ccolor != nullptr) {
		if (std::find(allowed_ccolors.begin(), allowed_ccolors.end(), variable.gcolor) !=
		    allowed_ccolors.end()) {
			return {nullptr, variable.gcolor};
		}
		else {
			return {nullptr, pick(allowed_ccolors)};
		}
	}

	// Repairing result
	if (variable.gcolor != nullptr && variable.ccolor == nullptr) {
		if (std::find(allowed_ccolors.begin(), allowed_ccolors.end(), variable.gcolor) !=
		    allowed_ccolors.end()) {
			return {nullptr, variable.gcolor};
		}
		else {
			return {nullptr, pick(allowed_ccolors)};
		}
	}

	assert(false && "choose_color for repairing not implemented!");
}

std::vector<MachineOperand*> RegisterAssignment::ccolors(MachineBasicBlock* block)
{
	std::vector<MachineOperand*> result;

	auto& allocated_variables = allocated_variables_map.at(block);
	for (const auto& variable : allocated_variables) {
		if (variable->ccolor)
			result.push_back(variable->ccolor);
	}

	std::sort(result.begin(), result.end(), op_cmp);

	return result;
}

std::vector<MachineOperand*> RegisterAssignment::gcolors(MachineBasicBlock* block)
{
	std::vector<MachineOperand*> result;

	auto& allocated_variables = allocated_variables_map.at(block);
	for (const auto& variable : allocated_variables) {
		if (variable->gcolor)
			result.push_back(variable->gcolor);
	}

	std::sort(result.begin(), result.end(), op_cmp);

	return result;
}

RegisterAssignment::Variable& RegisterAssignment::variable_for(MachineOperand* operand)
{
	auto iter = variables.find(operand->get_id());
	if (iter == variables.end()) {
		Variable variable{operand, nullptr, nullptr};
		iter = variables.emplace(operand->get_id(), variable).first;
	}
	return iter->second;
}

void RegisterAssignment::assign_operands_color(MachineInstruction* instruction)
{
	auto uses_lambda = [&](auto& descriptor) {
		if (!reg_alloc_considers_operand(*descriptor.op))
			return;

		auto& variable = this->variable_for(descriptor.op);
		if (variable.ccolor != nullptr) {
			descriptor.op = variable.ccolor;
		}
		else {
			variable.unassigned_uses.push_back(&descriptor);
		}
	};

	std::for_each(instruction->begin(), instruction->end(), uses_lambda);
	std::for_each(instruction->dummy_begin(), instruction->dummy_end(), uses_lambda);

	auto result_lambda = [&](auto& descriptor) {
		if (!reg_alloc_considers_operand(*descriptor.op))
			return;

		auto& variable = this->variable_for(descriptor.op);
		descriptor.op = variable.ccolor;

		for (auto& desc : variable.unassigned_uses) {
			desc->op = variable.gcolor;
		}
	};
	std::for_each(instruction->results_begin(), instruction->results_end(), result_lambda);
}

MachineOperand* RegisterAssignment::ccolor(MachineOperand* operand)
{
	return variable_for(operand).ccolor;
}

std::vector<MachineOperand*> RegisterAssignment::allowed_colors(MachineInstruction* instruction,
                                                                Variable* variable)
{
	RegisterSet constrained;

	auto descriptor_lambda = [&](const auto& descriptor) {
		if (!descriptor.op->aquivalent(*variable->operand))
			return;

		auto requirement = descriptor.get_MachineRegisterRequirement();
		if (requirement) {
			// If the required operand is virtual (like for 2-Address Mode requirements), look up
			// the currently assigned physical register
			auto required_operand = requirement->get_required();
			required_operand =
			    required_operand->is_virtual() ? this->ccolor(required_operand) : required_operand;
			assert(required_operand && "Required operand must not be null (its required...)");

			constrained.push_back(required_operand);
		}
	};

	std::for_each(instruction->begin(), instruction->end(), descriptor_lambda);
	std::for_each(instruction->dummy_begin(), instruction->dummy_end(), descriptor_lambda);
	std::for_each(instruction->results_begin(), instruction->results_end(), descriptor_lambda);

	if (!constrained.empty())
		return constrained;

	return std::vector<MachineOperand*>(physical_registers_for(variable->operand->get_type()));
}

std::vector<MachineOperand*> RegisterAssignment::used_ccolors(MachineInstruction* instruction)
{
	std::vector<MachineOperand*> used_colors;
	auto uses_lambda = [&](const auto& descriptor) {
		auto& variable = this->variable_for(descriptor.op);
		if (variable.ccolor != nullptr) {
			used_colors.push_back(variable.ccolor);
		}
	};

	std::for_each(instruction->begin(), instruction->end(), uses_lambda);
	std::for_each(instruction->dummy_begin(), instruction->dummy_end(), uses_lambda);

	return used_colors;
}

std::vector<MachineOperand*> RegisterAssignment::def_ccolors(MachineInstruction* instruction)
{
	std::vector<MachineOperand*> def_colors;
	auto def_lambda = [&](const auto& descriptor) {
		auto& variable = this->variable_for(descriptor.op);
		if (variable.ccolor != nullptr) {
			def_colors.push_back(variable.ccolor);
		}
	};

	std::for_each(instruction->results_begin(), instruction->results_end(), def_lambda);

	return def_colors;
}

RegisterAssignment::Variable*
RegisterAssignment::allocated_variable_with_ccolor(MachineBasicBlock* block, MachineOperand* ccolor)
{
	auto& allocated_variables = allocated_variables_map.at(block);
	for (auto& variable : allocated_variables) {
		if (variable->ccolor->aquivalent(*ccolor)) {
			return variable;
		}
	}

	return nullptr;
}

bool RegisterAssignment::repair_argument(MachineBasicBlock* block,
                                         MachineInstruction* instruction,
                                         MachineOperandDesc& descriptor,
                                         ParallelCopy& parallel_copy)
{
	RegisterSet available;

	auto& physical_registers = physical_registers_for(descriptor.op->get_type());
	auto allocated_ccolors = ccolors(block);
	std::set_difference(physical_registers.cbegin(), physical_registers.cend(),
	                    allocated_ccolors.begin(), allocated_ccolors.end(),
	                    std::back_inserter(available), op_cmp);
	return repair_argument(block, instruction, descriptor, parallel_copy, available, {});
}

bool RegisterAssignment::repair_argument(MachineBasicBlock* block,
                                         MachineInstruction* instruction,
                                         MachineOperandDesc& descriptor,
                                         ParallelCopy& parallel_copy,
                                         const RegisterSet& available,
                                         const RegisterSet& forbidden)
{
	LOG1("[" << descriptor.op << "]: Reparing argument " << descriptor.op << nl);

	MachineOperand* ccolor = nullptr;
	RegisterSet allowed;

	auto allowed_cols = allowed_colors(instruction, &variable_for(descriptor.op));
	std::set_difference(allowed_cols.begin(), allowed_cols.end(), forbidden.begin(),
	                    forbidden.end(), std::back_inserter(allowed), op_cmp);

	if (DEBUG_COND_N(3)) {
		LOG3("[" << descriptor.op << "]: Allowed colors:   ");
		print_ptr_container(dbg(), allowed.begin(), allowed.end());
		LOG3(nl);
		LOG3("[" << descriptor.op << "]: Available colors: ");
		print_ptr_container(dbg(), available.begin(), available.end());
		LOG3("\n\n");
	}

	while (ccolor == nullptr && !allowed.empty()) {
		// Regs not used in instruction => not constrained. So start trying regs that are not used
		// in this instruction first
		std::vector<MachineOperand*> not_used_colors;
		auto used_colors = used_ccolors(instruction);

		if (DEBUG_COND_N(3)) {
			LOG3("[" << descriptor.op << "]: Used ccolors:        ");
			print_ptr_container(dbg(), used_colors.begin(), used_colors.end());
			LOG3(nl);

			LOG3("[" << descriptor.op << "]: Allocated variables: ");
			print_ptr_container(dbg(), allocated_variables_map.at(block).begin(),
			                    allocated_variables_map.at(block).end());
			LOG3(nl);
		}

		std::set_difference(allowed.begin(), allowed.end(), used_colors.begin(), used_colors.end(),
		                    std::back_inserter(not_used_colors), op_cmp);

		auto& tmp_set = not_used_colors.empty() ? allowed : not_used_colors;
		auto color_pair = choose_color(block, instruction, descriptor, tmp_set);
		ccolor = color_pair.ccolor;
		LOG3("[" << descriptor.op << "]: Choosen ccolor: " << ccolor << nl);

		auto pawn = allocated_variable_with_ccolor(block, ccolor);
		LOG3("[" << descriptor.op << "]: Found pawn " << pawn->operand << " [" << pawn->gcolor
		         << ", " << pawn->ccolor << "]\n");

		RegisterSet pawn_allowed;
		RegisterSet available_with_current(available);
		available_with_current.push_back(variable_for(descriptor.op).ccolor);
		std::sort(available_with_current.begin(), available_with_current.end(), op_cmp);

		RegisterSet intersection;
		std::set_intersection(allowed_cols.begin(), allowed_cols.end(),
		                      available_with_current.begin(), available_with_current.end(),
		                      std::back_inserter(intersection), op_cmp);
		std::set_difference(available_with_current.begin(), available_with_current.end(),
		                    forbidden.begin(), forbidden.end(), std::back_inserter(pawn_allowed),
		                    op_cmp);

		LOG3("[" << descriptor.op << "]: Allowed pawn colors: ");
		DEBUG3(print_ptr_container(dbg(), pawn_allowed.begin(), pawn_allowed.end()));
		LOG3(nl);

		bool success = false;
		if (!pawn_allowed.empty()) {
			MachineOperandDesc descriptor(nullptr, pawn->operand);
			auto pawn_color_pair = choose_color(block, instruction, descriptor, pawn_allowed);
			parallel_copy.add(pawn, pawn_color_pair.ccolor);
			success = true;
		}
		else {
			assert(false && "Recursive call not implemented yet!");
		}

		if (!success) {
			assert(false && "Multiple iterations not implemented!");
		}
	}

	if (ccolor != nullptr) {
		parallel_copy.add(&variable_for(descriptor.op), ccolor);
		return true;
	}

	return false;
}

bool RegisterAssignment::repair_result(MachineBasicBlock* block,
                                       MachineInstruction* instruction,
                                       MachineOperandDesc& descriptor,
                                       ParallelCopy& parallel_copy,
                                       const OperandSet& live_out)
{
	LOG1("[" << descriptor.op << "]: Repairing result\n");

	MachineOperand* ccolor = nullptr;
	RegisterSet allowed;

	auto allowed_cols = allowed_colors(instruction, &variable_for(descriptor.op));
	auto use_def_cols = used_ccolors(instruction);
	auto def_cols = def_ccolors(instruction);
	std::copy(def_cols.begin(), def_cols.end(), std::back_inserter(use_def_cols));
	std::sort(use_def_cols.begin(), use_def_cols.end(), op_cmp);

	std::set_difference(allowed_cols.begin(), allowed_cols.end(), use_def_cols.begin(),
	                    use_def_cols.end(), std::back_inserter(allowed), op_cmp);

	if (DEBUG_COND_N(3)) {
		LOG3_NAMED_PTR_CONTAINER("[" << descriptor.op << "]: Allowed colors: ", allowed);
	}

	while (!allowed.empty() && ccolor == nullptr) {
		auto color_pair = choose_color(block, instruction, descriptor, allowed);
		ccolor = color_pair.ccolor;

		LOG3("[" << descriptor.op << "]: Choosen ccolor: " << ccolor << nl);

		auto pawn = allocated_variable_with_ccolor(block, ccolor);
		LOG3("[" << descriptor.op << "]: Found pawn " << pawn->operand << " [" << pawn->gcolor
		         << ", " << pawn->ccolor << "]\n");

		RegisterSet tmp;
		RegisterSet pawn_allowed;

		auto constrained = allowed_colors(instruction, pawn);
		std::set_difference(constrained.begin(), constrained.end(), use_def_cols.begin(),
		                    use_def_cols.end(), std::back_inserter(tmp), op_cmp);
		// Remove the choosen color from the allowed ones
		// auto iter = std::find_if(pawn_allowed.begin(), pawn_allowed.end(),
		//                         [&](const auto pawn) { return pawn->aquivalent(*ccolor); });
		// if (iter != pawn_allowed.end()) {
		//	pawn_allowed.erase(iter);
		//}

		// Temporarly, also remove all currently used colors, only allow free colors to be used
		// for the pawn

		auto used_ccolors = ccolors(block);
		std::set_difference(tmp.begin(), tmp.end(), used_ccolors.begin(), used_ccolors.end(),
		                    std::back_inserter(pawn_allowed), op_cmp);

		LOG3("[" << descriptor.op << "]: Allowed pawn colors: ");
		DEBUG3(print_ptr_container(dbg(), pawn_allowed.begin(), pawn_allowed.end()));
		LOG3(nl);

		MachineOperand* pawn_color = nullptr;
		if (!pawn_allowed.empty()) {
			// There is an available spot for pawn
			MachineOperandDesc desc(nullptr, pawn->operand);
			auto pawn_color_pair = choose_color(block, instruction, desc, pawn_allowed);
			pawn_color = pawn_color_pair.ccolor;
		}
		else {
			// TODO: Implement this more efficiently
			bool found_color = false;

			// pawn's color could be free (for our operand) by swapping pawn with a last use
			auto uses_lambda = [&](const auto& descriptor) {
				if (found_color || !reg_alloc_considers_operand(*descriptor.op))
					return;
				if (live_out.contains(descriptor.op))
					return;

				// TODO: Should be somewhere else and better documented (its a simple "contains")
				auto is_element = [](const RegisterSet& set, MachineOperand* op) {
					auto iter = std::find_if(set.begin(), set.end(), [&](const auto operand) {
						return op->aquivalent(*operand);
					});
					return iter != set.end();
				};

				auto& variable = this->variable_for(descriptor.op);
				auto constrained_pawn = this->allowed_colors(instruction, pawn);
				auto constrained_arg = this->allowed_colors(instruction, &variable);
				bool success = this->ccolor_is_available(block, variable.ccolor) &&
				               is_element(constrained_pawn, variable.ccolor) &&
				               is_element(constrained_arg, pawn->ccolor);

				if (success) {
					found_color = true;
					pawn_color = variable.ccolor;
					parallel_copy.add(&variable, pawn->ccolor);
				}
			};

			std::for_each(instruction->begin(), instruction->end(), uses_lambda);
			std::for_each(instruction->dummy_begin(), instruction->dummy_end(), uses_lambda);
		}

		if (pawn_color != nullptr) {
			parallel_copy.add(pawn, pawn_color);
		}
		else {
			auto iter = std::find_if(allowed.begin(), allowed.end(), [&](const auto operand) {
				return operand->aquivalent(*ccolor);
			});
			if (iter != allowed.end()) {
				allowed.erase(iter);
			}
			ccolor = nullptr;
		}
	}

	if (ccolor != nullptr) {
		auto& variable = variable_for(descriptor.op);
		variable.ccolor = ccolor;
		return true;
	}

	return false;
}

bool RegisterAssignment::ccolor_is_available(MachineBasicBlock* block, MachineOperand* operand)
{
	return allocated_variable_with_ccolor(block, operand) == nullptr;
}

void ParallelCopy::add(RegisterAssignment::Variable* variable, MachineOperand* target)
{
	auto iter = std::find_if(operations.begin(), operations.end(), [&](const auto& copy) {
		return copy.variable->operand->aquivalent(*variable->operand);
	});

	if (iter == operations.end()) {
		operations.push_back({variable, variable->ccolor, target});
		LOG2("Adding parallel copy for " << variable->operand << ": " << variable->ccolor << " -> "
		                                 << target << nl);
	}
	else {
		iter->target = target;
		LOG2("Updating parallel copy for " << variable->operand << ": " << iter->source << " -> "
		                                   << iter->target << nl);
	}
	variable->ccolor = target;
}

bool RegisterAssignmentPass::run(JITData& JD)
{
	LOG(nl << BoldYellow << "Running RegisterAssignmentPass" << reset_color << nl);
	auto MDT = get_Pass<MachineDominatorPass>();
	auto RPO = get_Pass<ReversePostOrderPass>();
	LTA = get_Pass<NewLivetimeAnalysisPass>();

	// Initialize available registers for assignment
	assignment.initialize_physical_registers(JD.get_Backend());

	for (auto& block : *RPO) {
		auto idom = MDT->get_idominator(block);

		LOG1(Yellow << "\nProcessing block " << *block);
		if (idom)
			LOG1(" (immediate dominator = " << *idom << ")");
		LOG1(reset_color << nl);

		assignment.initialize_variables_for(block, idom);

		// Remove all allocated variables that are not in the live in set of this block
		auto& block_live_in = LTA->get_live_in(block);
		assignment.intersect_variables_with(block, block_live_in);

		// Reverse instruction iteration to calculate live out sets for each instruction
		std::list<std::pair<MachineInstruction*, OperandSet>> live_outs;
		OperandSet live_out = LTA->get_live_out(block);
		for (auto i = block->rbegin(), e = --block->rend(); i != e; ++i) {
			auto instruction = *i;
			live_outs.emplace_front(instruction, live_out);

			live_out = LTA->liveness_transfer(live_out, block->convert(i));
		}

		// Add PHIs at the front
		for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
			auto instruction = *i;
			live_outs.emplace_front(instruction, live_out);
		}

		// Forward iteration over instructions + live_out data
		for (const auto& pair : live_outs) {
			auto instruction = pair.first;
			const auto& live_out = pair.second;

			if (!reg_alloc_considers_instruction(instruction)) {
				// MDeopts do not need any consideration, but the used vregs still need their
				// physical registers assigned
				assignment.assign_operands_color(instruction);
				continue;
			}

			if (DEBUG_COND_N(2)) {
				LOG2(Magenta << "\nProcessing instruction: " << *instruction << reset_color << nl);
				LOG2_NAMED_CONTAINER("                        LiveOut: ", live_out);
			}

			ParallelCopy parallel_copy;

			if (!instruction->to_MachinePhiInst()) {
				std::vector<MachineOperand*> dead_operands;

				auto uses_lambda = [&](auto& op_desc) {
					auto operand = op_desc.op;
					if (!reg_alloc_considers_operand(*operand))
						return;

					// Check if there are any requirements, and if so, if the operand already
					// is in the correct register
					auto assigned_color = assignment.ccolor(operand);
					auto requirement = op_desc.get_MachineRegisterRequirement();
					if (requirement) {
						auto required_op = requirement->get_required();
						LOG3("[" << operand << "]: Operator required to be in " << required_op
						         << " and is currently in " << assigned_color << nl);

						if (!required_op->aquivalent(*assigned_color)) {
							// If the required register is available, we move it there
							if (assignment.ccolor_is_available(block, required_op)) {
								LOG3("[" << operand << "]: " << required_op
								         << " is available, adding to parallel copy.\n");
								parallel_copy.add(&assignment.variable_for(operand), required_op);
							}
							else {
								auto success = assignment.repair_argument(block, instruction,
								                                          op_desc, parallel_copy);
								assert(success && "Graph-coloring as fallback not implemented!");
							}
						}
					}

					if (live_out.contains(operand))
						return;

					dead_operands.push_back(operand);
				};

				std::for_each(instruction->begin(), instruction->end(), uses_lambda);
				std::for_each(instruction->dummy_begin(), instruction->dummy_end(), uses_lambda);

				// Release dead variables
				for (auto operand : dead_operands) {
					LOG1("[" << operand << "]: Making " << assignment.ccolor(operand)
					         << " available.\n");
					assignment.remove_allocated_variable(block, operand);
				}
			}

			for (auto i = instruction->results_begin(), e = instruction->results_end(); i != e;
			     ++i) {
				auto& result_descriptor = *i;
				auto result_operand = result_descriptor.op;

				if (!reg_alloc_considers_operand(*result_operand)) {
					continue;
				}

				// Assign definition
				auto color_pair = assignment.choose_color(block, instruction, result_descriptor);
				auto& variable = assignment.variable_for(result_operand);
				variable.gcolor = color_pair.gcolor;
				variable.ccolor = color_pair.ccolor;

				LOG1("Assigned " << variable.operand << " = [" << variable.gcolor << ", "
				                 << variable.ccolor << "]\n");

				if (color_pair.ccolor == nullptr) {
					bool success = assignment.repair_result(block, instruction, result_descriptor,
					                                        parallel_copy, live_out);
					assert(success && "Fallback Graph-coloring for definitions not implemented!");
				}
				assignment.add_allocated_variable(block, result_operand);
			}

			for (auto i = instruction->results_begin(), e = instruction->results_end(); i != e;
			     ++i) {
				// Release dead definitions
				auto result_operand = i->op;
				if (reg_alloc_considers_operand(*result_operand) &&
				    !live_out.contains(result_operand)) {
					LOG1("Releasing " << *result_operand << " since it is dead after this.\n");
					assignment.remove_allocated_variable(block, result_operand);
				}
			}

			// Insert the parallel copies before this instrution
			for (const auto& copy : parallel_copy) {
				auto move = JD.get_Backend()->create_Move(copy.source, copy.target);
				auto iter = std::find(block->begin(), block->end(), instruction);
				assert(iter != block->end());
				block->insert_before(iter, move);

				LOG1("Add Move: " << move << nl);
			}
			assignment.assign_operands_color(instruction);
		}

		// Fix global colors, but only for blocks that dont end with a "ret"
		if (!block->back()->is_end()) {
			ParallelCopy parallel_copy;
			assignment.for_each_allocated_variable(
			    block, [&](const auto variable) { parallel_copy.add(variable, variable->gcolor); });

			for (const auto& copy : parallel_copy) {
				assert(false && "Fixing global colors not implemented!");
			}
		}
	}

	return true;
}

PassUsage& RegisterAssignmentPass::get_PassUsage(PassUsage& PU) const
{
	PU.add_requires<MachineDominatorPass>();
	PU.add_requires<MachineLoopPass>();
	PU.add_requires<NewLivetimeAnalysisPass>();
	PU.add_requires<ReversePostOrderPass>();
	PU.add_requires<NewSpillPass>();

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
