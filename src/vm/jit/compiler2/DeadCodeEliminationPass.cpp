/* src/vm/jit/compiler2/DeadCodeEliminationPass.cpp - DeadCodeEliminationPass

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

#include "vm/jit/compiler2/DeadCodeEliminationPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "toolbox/logging.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/SSAPrinterPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/ScheduleEarlyPass.hpp"
#include "vm/jit/compiler2/InstructionMetaPass.hpp"

#define DEBUG_NAME "compiler2/DeadCodeEliminationPass"

STAT_DECLARE_GROUP(compiler2_stat)
STAT_REGISTER_SUBGROUP(compiler2_deadcodeeliminationpass_stat,
	"deadcodeeliminationpass","deadcodeeliminationpass",compiler2_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_dead_nodes,0,"# dead nodes",
	"number of removed dead nodes",compiler2_deadcodeeliminationpass_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_remaining_nodes,0,"# remaining nodes",
	"number of nodes not removed",compiler2_deadcodeeliminationpass_stat)

namespace cacao {
namespace jit {
namespace compiler2 {

// TODO: maybe we should make this a flag in the Instruction class
bool is_control_flow_inst(Instruction *inst) {
	return inst->get_opcode() == Instruction::BeginInstID
			|| inst->to_EndInst();
}

bool DeadCodeEliminationPass::run(JITData &JD) {
	Method *M = JD.get_Method();

	// this work list is used by the algorithm to store the instructions which
	// have to be reconsidered. at the beginning it therefore contains all
	// instructions.
	Method::InstructionListTy workList(M->begin(), M->end());

	// will be used to look up whether an instruction is currently contained in the
	// worklist to avoid inserting an instruction which is already in the list.
	InstBoolMapTy inWorkList;
	for (Method::const_iterator i = workList.begin(), e = workList.end();
			i != e; i++) {
		inWorkList[*i] = true;
	}

	// will be used to mark instructions as dead.
	InstBoolMapTy dead;

	// used to track for each instruction the number of its users which are
	// already dead
	InstIntMapTy deadUsers;

	Method::InstructionListTy liveInstructions;
	Method::InstructionListTy deadInstructions;

	while (!workList.empty()) {
		Instruction *I = workList.front();
		workList.pop_front();
		inWorkList[I] = false;

		// here we inspect whether the node that has just been picked from
		// the work list is dead
		if (I->user_size() == deadUsers[I] && !I->has_side_effects()
				&& !is_control_flow_inst(I) && !I->to_SourceStateInst()
				&& !I->to_ReplacementEntryInst()
				&& !I->to_AssumptionInst()
				&& I->rdep_size() == 0) {
			dead[I] = true;
			InstBoolMapTy visited;

			// insert the dead instructions in the order they should be deleted
			deadInstructions.push_back(I);

			for (Instruction::OperandListTy::const_iterator
					i = I->op_begin(), e = I->op_end(); i != e; i++) {
				Instruction *Op = (*i)->to_Instruction();
				
				if (Op && !visited[Op]) {
					visited[Op] = true;
					deadUsers[Op]++;

					if (!inWorkList[Op]) {
						workList.push_back(Op);
						inWorkList[Op] = true;
					}
				}
			}
		}
	}
	
	// collect the living instructions (i.e., the instructions which are not dead)
	for (Method::const_iterator i = M->begin(), e = M->end(); i != e; i++) {
		Instruction *I = *i;
		if (dead[I]) {
			LOG("Dead: " << I << nl);
		} else {
			liveInstructions.push_back(I);
			LOG("Live: " << I << nl);
		}
	}

	STATISTICS(num_dead_nodes = deadInstructions.size());
	STATISTICS(num_remaining_nodes = liveInstructions.size());

	M->replace_instruction_list(liveInstructions.begin(), liveInstructions.end());

	// delete the dead instructions
	while (!deadInstructions.empty()) {
		Instruction *I = deadInstructions.front();
		deadInstructions.pop_front();
		delete I;
	}

	return true;
}

// pass usage
PassUsage& DeadCodeEliminationPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<InstructionMetaPass>();
	PU.add_schedule_before<ScheduleEarlyPass>();
	PU.add_schedule_before<SSAPrinterPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char DeadCodeEliminationPass::ID = 0;

// register pass
static PassRegistry<DeadCodeEliminationPass> X("DeadCodeEliminationPass");

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
