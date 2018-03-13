/* src/vm/jit/compiler2/lsra/SSAReconstructor.hpp

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

#ifndef _JIT_COMPILER2_LSRA_SSARECONSTRUCTOR
#define _JIT_COMPILER2_LSRA_SSARECONSTRUCTOR

#include <map>
#include <unordered_map>
#include <vector>

#include "vm/jit/compiler2/lsra/NewLivetimeAnalysisPass.hpp"

MM_MAKE_NAME(SSAReconstructor)

namespace cacao {
namespace jit {
namespace compiler2 {

class MachineBasicBlock;
class MachineInstruction;
class MachineOperand;
class MachineOperandDesc;
class MachinePhiInst;

class NewSpillPass;

class SSAReconstructor : public memory::ManagerMixin<SSAReconstructor> {
public:
	explicit SSAReconstructor(NewSpillPass* pass);

	void add_new_definitions(const Occurrence& original_def,
	                         const std::vector<Occurrence>& new_definitions);

	void reconstruct();

private:
	NewSpillPass* sp;

	// These 2 sets are used for checking during
	// LIR traversal, if we need to consider the operands in each instruction
	std::vector<std::size_t> new_definitions_set;
	std::vector<std::size_t> original_definitions_set;

	/// \todo Current data structures used as a proof of concept, may need something better here
	using BlockToOperand = std::unordered_map<std::size_t, MachineOperand*>;
	std::unordered_map<std::size_t, BlockToOperand> current_def;

	using VariableToPhi = std::unordered_map<MachineOperand*, MachinePhiInst*>;
	std::unordered_map<std::size_t, VariableToPhi> incomplete_phis;

	using NewDefinitionTy = std::pair<Occurrence, std::vector<Occurrence>>;
	std::vector<NewDefinitionTy> definitions;

	// Links new defintions to the original one
	std::unordered_map<std::size_t, MachineOperand*> reverse_def;

	// Links newly introduced PHIs to their uses, so trivial PHIs can be removed
	// more easily later on
	std::unordered_map<std::size_t, std::vector<MachineOperandDesc*>> uses;

	void write_variable(MachineOperand*,MachineBasicBlock*, MachineOperand*);
	MachineOperand* read_variable(MachineOperand*,MachineBasicBlock*);
	MachineOperand* read_variable_recursive(MachineOperand*,MachineBasicBlock*);
	MachineOperand* add_phi_operands(MachineOperand*,MachinePhiInst*);
	MachineOperand* try_remove_trivial_phi(MachinePhiInst*);

	void handle_uses(MachineInstruction*);
	void handle_definitions(MachineInstruction*);
	void prepare_operand_sets();

	std::unordered_set<std::size_t> sealed_blocks;
	std::unordered_set<std::size_t> finished_blocks;

	bool is_sealed(MachineBasicBlock*) const;
	bool is_finished(MachineBasicBlock*) const;

	void try_seal_block(MachineBasicBlock*);
	void seal_block(MachineBasicBlock*);
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LSRA_SSARECONSTRUCTOR */

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
