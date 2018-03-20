/* src/vm/jit/compiler2/PhiLiftingPass.cpp - PhiLiftingPass

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

#include "vm/jit/compiler2/PhiLiftingPass.hpp"

#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/ReversePostOrderPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/treescan/NewLivetimeAnalysisPass.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/PhiLiftingPass"

namespace cacao {
namespace jit {
namespace compiler2 {

void PhiLiftingPass::handle_phi(MachinePhiInst* instruction)
{
	LOG2(Cyan << "Handling PHI instruction: " << *instruction << reset_color << nl);

	auto MOF = backend->get_JITData()->get_MachineOperandFactory();
	auto block = instruction->get_block();

	// Copy parameters in predecessor basic block
	for (unsigned idx = 0, e = block->pred_size(); idx < e; ++idx) {
		auto operand = instruction->get(idx).op;
		auto copy_op = MOF->CreateVirtualRegister(operand->get_type());
		instruction->get(idx).op = copy_op;

		auto move_instr = backend->create_Move(operand, copy_op);
		auto predecessor = block->get_predecessor(idx);
		LOG2("\tInserting (" << *move_instr << ") in " << *predecessor << nl);
		
		insert_before(predecessor->mi_last_insertion_point(), move_instr);
	}

	// Copy result in current block
	auto operand = instruction->get_result().op;
	auto result = MOF->CreateVirtualRegister(operand->get_type());
	instruction->get_result().op = result;

	auto move_instr = backend->create_Move(result, operand);
	block->insert_after(block->begin(), move_instr);
	
	LOG2("\tInserting (" << *move_instr << ") in " << *block << nl);
}

bool PhiLiftingPass::run(JITData& JD)
{
	LOG1(BoldYellow << "\nRunning PhiLiftingPass" << reset_color << nl);
	backend = JD.get_Backend();

	auto RPO = get_Artifact<ReversePostOrderPass>();
	for (auto& block : *RPO) {
		for (auto i = block->phi_begin(), e = block->phi_end(); i != e; ++i) {
			handle_phi(*i);
		}
	}
	return true;
}

// pass usage
PassUsage& PhiLiftingPass::get_PassUsage(PassUsage& PU) const
{
	PU.requires<ReversePostOrderPass>();
	PU.before<NewLivetimeAnalysisPass>();
	PU.modifies<LIRInstructionScheduleArtifact>();
	return PU;
}

// register pass
static PassRegistry<PhiLiftingPass> X("PhiLiftingPass");

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
