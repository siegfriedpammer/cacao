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
#include "vm/jit/compiler2/SSADeconstructionPass.hpp"
#include "vm/jit/compiler2/lsra/LogHelper.hpp"
#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/lsra/NewSpillPass.hpp"

#define DEBUG_NAME "compiler2/RegisterAssignment"

namespace cacao {
namespace jit {
namespace compiler2 {

OStream& operator<<(OStream& OS, const RegisterAssignment::Variable& variable)
{
	OS << *variable.operand;
	return OS;
}

RegisterAssignment::RegisterAssignment(RegisterAssignmentPass& pass) : rapass(pass)
{
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
	for (unsigned i = 0; i < backend->get_RegisterInfo()->class_count(); ++i) {
		const auto& rc = backend->get_RegisterInfo()->get_class(i);
		if (rc.handles_type(type)) {
			physical_registers.emplace(type, rc.get_All());
			return;
		}
	}
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
		LOG3_NAMED_PTR_CONTAINER("Initial allocated variables (" << *block << "): ", variables);
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
		LOG3_NAMED_PTR_CONTAINER("Allocated vars intersected  (" << *block << "): ", variables);
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
	auto colors = allowed_colors(instruction, &variable_for(descriptor.op));
	auto allowed_ccolors = colors - ccolors(block);

	return choose_color(block, instruction, descriptor, allowed_ccolors);
}

RegisterAssignment::ColorPair RegisterAssignment::choose_color(MachineBasicBlock* block,
                                                               MachineInstruction* instruction,
                                                               MachineOperandDesc& descriptor,
                                                               OperandSet& allowed_ccolors)
{
	const auto& physical_registers = physical_registers_for(descriptor.op->get_type());
	auto allowed_gcolors = physical_registers - gcolors(block);

	auto& variable = variable_for(descriptor.op);

	if (DEBUG_COND_N(3)) {
		LOG3_NAMED_CONTAINER("[" << descriptor.op << "]: Allowed Local Colors:  ", allowed_ccolors);
		LOG3_NAMED_CONTAINER("[" << descriptor.op << "]: Allowed Global Colors: ", allowed_gcolors);
	}

	// We have reached a definition, set both gcolor and ccolor
	if (variable.gcolor == nullptr && variable.ccolor == nullptr) {
		auto intersection = allowed_ccolors & allowed_gcolors;
		LOG3_NAMED_CONTAINER("[" << descriptor.op << "]: Intersection:          ", intersection);

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
		if (allowed_ccolors.contains(variable.gcolor)) {
			return {nullptr, variable.gcolor};
		}
		else {
			return {nullptr, pick(allowed_ccolors)};
		}
	}

	// Repairing result
	if (variable.gcolor != nullptr && variable.ccolor == nullptr) {
		if (allowed_ccolors.contains(variable.gcolor)) {
			return {nullptr, variable.gcolor};
		}
		else {
			return {nullptr, pick(allowed_ccolors)};
		}
	}

	assert(false && "choose_color for repairing not implemented!");
}

OperandSet RegisterAssignment::ccolors(MachineBasicBlock* block)
{
	auto result = rapass.native_factory->EmptySet();

	auto& allocated_variables = allocated_variables_map.at(block);
	for (const auto& variable : allocated_variables) {
		if (variable->ccolor)
			result.add(variable->ccolor);
	}

	return result;
}

OperandSet RegisterAssignment::gcolors(MachineBasicBlock* block)
{
	auto result = rapass.native_factory->EmptySet();

	auto& allocated_variables = allocated_variables_map.at(block);
	for (const auto& variable : allocated_variables) {
		if (variable->gcolor)
			result.add(variable->gcolor);
	}

	return result;
}

MachineOperand* RegisterAssignment::pick(OperandSet& operands) const
{
	if (operands.empty())
		return nullptr;

	return &*operands.begin();
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

/// Since we use the machine registers as colors directly, we need to create
/// native instances that also contain the correct type
void RegisterAssignment::assign_operands_color(MachineInstruction* instruction)
{
	auto uses_lambda = [&](auto& descriptor) {
		if (!reg_alloc_considers_operand(*descriptor.op))
			return;

		auto& variable = this->variable_for(descriptor.op);
		if (variable.ccolor_native != nullptr) {
			descriptor.op = variable.ccolor_native;
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
		auto type = variable.operand->get_type();
		variable.ccolor_native =
		    this->rapass.factory->CreateNativeRegister<NativeRegister>(type, variable.ccolor);
		variable.gcolor_native =
		    this->rapass.factory->CreateNativeRegister<NativeRegister>(type, variable.gcolor);

		descriptor.op = variable.ccolor_native;
		rapass.used_operands->add(variable.ccolor);

		for (auto& desc : variable.unassigned_uses) {
			desc->op = variable.gcolor_native;
			rapass.used_operands->add(variable.gcolor);
		}
	};
	std::for_each(instruction->results_begin(), instruction->results_end(), result_lambda);
}

MachineOperand* RegisterAssignment::ccolor(MachineOperand* operand)
{
	return variable_for(operand).ccolor;
}

OperandSet RegisterAssignment::allowed_colors(MachineInstruction* instruction, Variable* variable)
{
	OperandSet constrained = rapass.native_factory->EmptySet();

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

			constrained.add(required_operand);
		}
	};

	std::for_each(instruction->begin(), instruction->end(), descriptor_lambda);
	std::for_each(instruction->dummy_begin(), instruction->dummy_end(), descriptor_lambda);
	std::for_each(instruction->results_begin(), instruction->results_end(), descriptor_lambda);

	if (!constrained.empty())
		return constrained;

	return physical_registers_for(variable->operand->get_type());
}

OperandSet RegisterAssignment::used_ccolors(MachineInstruction* instruction)
{
	auto used_colors = rapass.native_factory->EmptySet();
	auto uses_lambda = [&](const auto& descriptor) {
		auto& variable = this->variable_for(descriptor.op);
		if (variable.ccolor != nullptr) {
			used_colors.add(variable.ccolor);
		}
	};

	std::for_each(instruction->begin(), instruction->end(), uses_lambda);
	std::for_each(instruction->dummy_begin(), instruction->dummy_end(), uses_lambda);

	return used_colors;
}

OperandSet RegisterAssignment::def_ccolors(MachineInstruction* instruction)
{
	auto def_colors = rapass.native_factory->EmptySet();
	auto def_lambda = [&](const auto& descriptor) {
		auto& variable = this->variable_for(descriptor.op);
		if (variable.ccolor != nullptr) {
			def_colors.add(variable.ccolor);
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
	const auto& physical_registers = physical_registers_for(descriptor.op->get_type());
	auto available = physical_registers - ccolors(block);
	auto forbidden = rapass.native_factory->EmptySet();

	return repair_argument(block, instruction, descriptor, parallel_copy, available, forbidden);
}

bool RegisterAssignment::repair_argument(MachineBasicBlock* block,
                                         MachineInstruction* instruction,
                                         MachineOperandDesc& descriptor,
                                         ParallelCopy& parallel_copy,
                                         OperandSet& available,
                                         OperandSet& forbidden)
{
	LOG1("[" << descriptor.op << "]: Reparing argument " << descriptor.op << nl);

	MachineOperand* ccolor = nullptr;

	auto allowed_cols = allowed_colors(instruction, &variable_for(descriptor.op));
	auto allowed = allowed_cols - forbidden;

	if (DEBUG_COND_N(3)) {
		LOG3_NAMED_CONTAINER("[" << descriptor.op << "]: Allowed colors:   ", allowed);
		LOG3_NAMED_CONTAINER("[" << descriptor.op << "]: Available colors: ", available);
		LOG3(nl);
	}

	while (ccolor == nullptr && !allowed.empty()) {
		// Regs not used in instruction => not constrained. So start trying regs that are not used
		// in this instruction first
		auto used_colors = used_ccolors(instruction);

		if (DEBUG_COND_N(3)) {
			LOG3_NAMED_CONTAINER("[" << descriptor.op << "]: Used ccolors:        ", used_colors);
			LOG3_NAMED_PTR_CONTAINER("[" << descriptor.op << "]: Allocated variables: ",
			                         allocated_variables_map.at(block));
		}

		auto not_used_colors = allowed - used_colors;
		auto& tmp_set = not_used_colors.empty() ? allowed : not_used_colors;
		auto color_pair = choose_color(block, instruction, descriptor, tmp_set);

		ccolor = color_pair.ccolor;
		LOG3("[" << descriptor.op << "]: Choosen ccolor: " << ccolor << nl);

		auto pawn = allocated_variable_with_ccolor(block, ccolor);
		LOG3("[" << descriptor.op << "]: Found pawn " << pawn->operand << " [" << pawn->gcolor
		         << ", " << pawn->ccolor << "]\n");

		auto available_with_current = available;
		available_with_current.add(variable_for(descriptor.op).ccolor);

		auto contrained = allowed_colors(instruction, pawn);
		auto pawn_allowed = (contrained & available_with_current) - forbidden;
		LOG3_NAMED_CONTAINER("[" << descriptor.op << "]: Allowed pawn colors: ", pawn_allowed);

		bool success = false;
		if (!pawn_allowed.empty()) {
			MachineOperandDesc descriptor(nullptr, pawn->operand);
			auto pawn_color_pair = choose_color(block, instruction, descriptor, pawn_allowed);
			parallel_copy.add(pawn, pawn_color_pair.ccolor);
			success = true;
		}
		else {
			auto tmp_available = available;
			auto tmp_forbidden = forbidden;
			tmp_available.add(variable_for(descriptor.op).ccolor);
			tmp_forbidden.add(pawn->ccolor);
			MachineOperandDesc tmp_descriptor(nullptr, pawn->operand);

			success = repair_argument(block, instruction, tmp_descriptor, parallel_copy,
			                          tmp_available, tmp_forbidden);
		}

		if (!success) {
			allowed.remove(ccolor);
			ccolor = nullptr;
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

	auto allowed_cols = allowed_colors(instruction, &variable_for(descriptor.op));
	auto use_def_cols = used_ccolors(instruction) | def_ccolors(instruction);
	auto allowed = allowed_cols - use_def_cols;

	LOG3_NAMED_CONTAINER("[" << descriptor.op << "]: Allowed colors: ", allowed);

	while (!allowed.empty() && ccolor == nullptr) {
		auto color_pair = choose_color(block, instruction, descriptor, allowed);
		ccolor = color_pair.ccolor;

		LOG3("[" << descriptor.op << "]: Choosen ccolor: " << ccolor << nl);

		auto pawn = allocated_variable_with_ccolor(block, ccolor);
		LOG3("[" << descriptor.op << "]: Found pawn " << pawn->operand << " [" << pawn->gcolor
		         << ", " << pawn->ccolor << "]\n");

		auto pawn_allowed = allowed_colors(instruction, pawn) - use_def_cols;

		// Temporarly, also remove all currently used colors, only allow free colors to be used
		// for the pawn
		pawn_allowed -= ccolors(block);

		LOG3_NAMED_CONTAINER("[" << descriptor.op << "]: Allowed pawn colors: ", pawn_allowed);

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

				auto& variable = this->variable_for(descriptor.op);
				auto constrained_pawn = this->allowed_colors(instruction, pawn);
				auto constrained_arg = this->allowed_colors(instruction, &variable);
				bool success = this->ccolor_is_available(block, variable.ccolor) &&
				               constrained_pawn.contains(variable.ccolor) &&
				               constrained_arg.contains(pawn->ccolor);

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
			allowed.remove(ccolor);
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
	auto MDT = get_Artifact<MachineDominatorPass>();
	auto RPO = get_Artifact<ReversePostOrderPass>();
	LTA = get_Artifact<NewLivetimeAnalysisPass>();

	factory = JD.get_MachineOperandFactory();
	native_factory = JD.get_Backend()->get_NativeFactory();
	used_operands = std::make_unique<OperandSet>(native_factory->EmptySet());

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
			if (parallel_copy.begin() != parallel_copy.end()) {
				ParallelCopyImpl pcopy(JD);
				LOG1("Calculating parallel copy:\n");
				for (const auto& copy : parallel_copy) {
					auto source = factory->CreateNativeRegister<NativeRegister>(
						copy.variable->operand->get_type(), copy.source);
					auto target = factory->CreateNativeRegister<NativeRegister>(
						copy.variable->operand->get_type(), copy.target);

					LOG1("Request move " << source << " -> " << target << nl);
					pcopy.add(source, target);

					used_operands->add(copy.target);
					copy.variable->ccolor_native = target;
				}

				pcopy.calculate();
				auto& operations = pcopy.get_operations();
				auto iter = std::find(block->begin(), block->end(), instruction);
				assert(iter != block->end());
				block->insert_before(iter, operations.begin(), operations.end());

				if (DEBUG_COND_N(1)) {
					for (auto operation : operations) {
						LOG1("Inserting operation before this instruction: " << *operation << nl);
					}
				}
			}
			assignment.assign_operands_color(instruction);
		}

		// Fix global colors, but only for blocks that dont end with a "ret"
		if (!block->back()->is_end()) {
			ParallelCopy parallel_copy;
			assignment.for_each_allocated_variable(
			    block, [&](const auto variable) { parallel_copy.add(variable, variable->gcolor); });

			ParallelCopyImpl pcopy(JD);
			for (const auto& copy : parallel_copy) {
				auto source = factory->CreateNativeRegister<NativeRegister>(copy.variable->operand->get_type(), copy.source);
				auto target = factory->CreateNativeRegister<NativeRegister>(copy.variable->operand->get_type(), copy.target);
				
				pcopy.add(source, target);

				copy.variable->ccolor_native = target;
			}

			pcopy.calculate();
			auto& operations = pcopy.get_operations();
			// TODO: We might need a universal "insert-point" at the end of basic blocks
			block->insert_before(--block->end(), operations.begin(), operations.end());

			if (DEBUG_COND_N(1)) {
				LOG1("Fixing global colors:\n");
				for (auto operation : operations) {
					LOG1("Inserting operation at end of block (" << *block << "): " << *operation << nl);
				}
			}
		}
	}

	return true;
}

/// Verify that machine registers have been replaced by NativeRegistes with assigned type
bool RegisterAssignmentPass::verify() const
{
	auto assert_op = [](const auto& descriptor) {
		auto op = descriptor.op;
		if (!op->to_Register())
			return;

		if (!op->to_Register()->to_MachineRegister()->to_NativeRegister() &&
		    op->get_type() != Type::VoidTypeID) {
			ABORT_MSG("Non NativeRegister", op << " in " << descriptor.get_MachineInstruction());
		}
	};

	auto assert_instruction = [&](const auto instr) {
		std::for_each(instr->begin(), instr->end(), assert_op);
		std::for_each(instr->dummy_begin(), instr->dummy_end(), assert_op);
		std::for_each(instr->results_begin(), instr->results_end(), assert_op);
	};

	auto RPO = get_Artifact<ReversePostOrderPass>();
	for (auto block : *RPO) {
		std::for_each(block->begin(), block->end(), assert_instruction);
		std::for_each(block->phi_begin(), block->phi_end(), assert_instruction);
	}

	return true;
}

PassUsage& RegisterAssignmentPass::get_PassUsage(PassUsage& PU) const
{
	PU.provides<RegisterAssignmentPass>();

	PU.requires<MachineDominatorPass>();
	PU.requires<MachineLoopPass>();
	PU.requires<NewLivetimeAnalysisPass>();
	PU.requires<ReversePostOrderPass>();
	PU.modifies<LIRInstructionScheduleArtifact>();

	PU.after<NewSpillPass>();
	return PU;
}

// register pass
static PassRegistry<RegisterAssignmentPass> X("RegisterAssignmentPass");
static ArtifactRegistry<RegisterAssignmentPass> Y("RegisterAssignmentPass");

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
