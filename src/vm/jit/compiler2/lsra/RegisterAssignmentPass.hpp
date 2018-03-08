/* src/vm/jit/compiler2/lsra/RegisterAssignmentPass.hpp - RegisterAssignmentPass

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

#ifndef _JIT_COMPILER2_LSRA_REGISTERASSIGNMENT
#define _JIT_COMPILER2_LSRA_REGISTERASSIGNMENT

#include <list>
#include <map>
#include <vector>

#include "vm/jit/compiler2/MachineOperandSet.hpp"
#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/Type.hpp"

MM_MAKE_NAME(RegisterAssignmentPass)

namespace cacao {
namespace jit {
namespace compiler2 {

class Backend;
class MachineBasicBlock;
class MachineInstruction;
class MachineOperand;
class MachineOperandDesc;
class MachineOperandFactory;
class NewLivetimeAnalysisPass;
class RegisterAssignmentPass;
class ParallelCopy;

class RegisterAssignment {
public:
	explicit RegisterAssignment(RegisterAssignmentPass& pass);

	void initialize_physical_registers(Backend* backend);
	void initialize_variables_for(MachineBasicBlock* block, MachineBasicBlock* idom);
	void intersect_variables_with(MachineBasicBlock* block, const OperandSet& live_in);

	void add_allocated_variable(MachineBasicBlock* block, MachineOperand* operand);
	void remove_allocated_variable(MachineBasicBlock* block, MachineOperand* operand);

	struct ColorPair {
		MachineOperand* gcolor;
		MachineOperand* ccolor;
	};

	ColorPair choose_color(MachineBasicBlock* block,
	                       MachineInstruction* instruction,
	                       MachineOperandDesc& descriptor);
	ColorPair choose_color(MachineBasicBlock* block,
	                       MachineInstruction* instruction,
	                       MachineOperandDesc& descriptor,
	                       OperandSet& allowed_ccolors);

	void assign_operands_color(MachineInstruction* instruction);

	struct Variable {
		MachineOperand* operand;
		MachineOperand* ccolor = nullptr;
		MachineOperand* gcolor = nullptr;

		std::vector<MachineOperandDesc*> unassigned_uses;

		/// Colors are machine registers without type, during final assignment
		/// NativeRegisters are created and cached here
		MachineOperand* ccolor_native = nullptr;
		MachineOperand* gcolor_native = nullptr;
	};

	bool repair_argument(MachineBasicBlock* block,
	                     MachineInstruction* instruction,
	                     MachineOperandDesc& descriptor,
	                     ParallelCopy& parallel_copy);
	bool repair_argument(MachineBasicBlock* block,
	                     MachineInstruction* instruction,
	                     MachineOperandDesc& descriptor,
	                     ParallelCopy& parallel_copy,
	                     OperandSet& available,
	                     OperandSet& forbidden);

	bool repair_result(MachineBasicBlock* block,
	                   MachineInstruction* instruction,
	                   MachineOperandDesc& descriptor,
	                   ParallelCopy& parallel_copy,
	                   const OperandSet& live_out);

	MachineOperand* ccolor(MachineOperand* operand);
	Variable& variable_for(MachineOperand* operand);

	// Checks whether a physical register is free to be used
	bool ccolor_is_available(MachineBasicBlock* block, MachineOperand* operand);

	template <typename UnaryFunction>
	void for_each_allocated_variable(MachineBasicBlock* block, UnaryFunction function)
	{
		auto allocated_vars = allocated_variables_map.at(block);
		std::for_each(allocated_vars.begin(), allocated_vars.end(), function);
	}

private:
	RegisterAssignmentPass& rapass;

	std::map<Type::TypeID, OperandSet> physical_registers;
	std::map<std::size_t, Variable> variables;

	using AllocatedVariableSet = std::vector<Variable*>;
	std::map<MachineBasicBlock*, AllocatedVariableSet> allocated_variables_map;

	void initialize_physical_registers_for_class(Backend* backend, Type::TypeID type);
	const OperandSet& physical_registers_for(Type::TypeID type)
	{
		return physical_registers.at(type);
	}

	OperandSet ccolors(MachineBasicBlock* block);
	OperandSet gcolors(MachineBasicBlock* block);

	MachineOperand* pick(OperandSet& operands) const;

	// Returns all allowed colors for a given instruction and variable
	// If variable is not used/defined by instruction, all colors of the corresponding
	// register class are returned
	OperandSet allowed_colors(MachineInstruction* instruction, Variable* variable);

	OperandSet used_ccolors(MachineInstruction* instruction);
	OperandSet def_ccolors(MachineInstruction* instruction);

	Variable* allocated_variable_with_ccolor(MachineBasicBlock* block, MachineOperand* ccolor);
};

OStream& operator<<(OStream& OS, const RegisterAssignment::Variable& variable);

class ParallelCopy {
public:
	struct Copy {
		RegisterAssignment::Variable* variable;
		MachineOperand* source;
		MachineOperand* target;
	};

	void add(RegisterAssignment::Variable* variable, MachineOperand* target);

	auto begin() const { return operations.begin(); }
	auto end() const { return operations.end(); }

private:
	std::vector<Copy> operations;
};

class RegisterAssignmentPass : public Pass, public Artifact, public memory::ManagerMixin<RegisterAssignmentPass> {
public:
	RegisterAssignmentPass() : Pass(), assignment(*this) {}
	bool run(JITData& JD) override;
	bool verify() const override;
	PassUsage& get_PassUsage(PassUsage& PU) const override;

	RegisterAssignmentPass* provide_Artifact(ArtifactInfo::IDTy) override {
		return this;
	}

	/// Returns a OperandSet from the Native Operand Factory of all the machine registers
	/// assigned during this pass. This is currently used to get all callee-saved registers and
	/// save/restore them during method prolog/epilog
	const OperandSet& get_used_operands() const { return *used_operands; }

private:
	MachineOperandFactory* factory = nullptr;
	MachineOperandFactory* native_factory = nullptr;

	std::unique_ptr<OperandSet> used_operands;

	RegisterAssignment assignment;

	NewLivetimeAnalysisPass* LTA;
	friend class RegisterAssignment;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LSRA_REGISTERASSIGNMENT */

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
