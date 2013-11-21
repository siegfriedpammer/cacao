/* src/vm/jit/compiler2/DeadcodeEliminationPass.cpp - DeadcodeEliminationPass

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

#include "vm/jit/compiler2/DeadcodeEliminationPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/Method.hpp"
#include "toolbox/logging.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/SSAPrinterPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"

#define DEBUG_NAME "compiler2/deadcodeeliminationpass"

namespace cacao {
namespace jit {
namespace compiler2 {

// helper function which determines if an instruction with the
// given opcode could possibly affect the output of the method.
// this information is essential to decide if an instruction withou
// any users is really dead and thus should be deleted, or if it
// may have influence on the method outputs and thus is not dead
// even if it has to users (e.g. the return instruction).
// TODO: maybe we should move this to the Instruction class
bool affectsMethodOutput(const Instruction::InstID opcode) {
	return opcode == Instruction::RETURNInstID ||
		   opcode == Instruction::INVOKESTATICInstID;
}

bool DeadcodeEliminationPass::run(JITData &JD) {
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

		// the first condition to be met for an instruction to be considered
		// 'dead' is that all its users are 'dead'.
		if (I->user_size() == deadUsers[I]) {
			if (I->get_opcode() == Instruction::BeginInstID) {
				// TODO: remove BeginInst only if its corresponding
				// "end" instruction is dead
			} else if (!affectsMethodOutput(I->get_opcode())) {
				// TODO: consider here all instructions that could possibly
				// influence the method output
				dead[I] = true;

				// insert the dead instructions in the order they should be deleted
				deadInstructions.push_back(I);

				for (Instruction::OperandListTy::const_iterator
						i = I->op_begin(), e = I->op_end(); i != e; i++) {
					Instruction *Op = (*i)->to_Instruction();
					
					if (Op) {
						deadUsers[Op]++;

						if (!inWorkList[Op]) {
							workList.push_back(Op);
							inWorkList[Op] = true;
						}
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
PassUsage& DeadcodeEliminationPass::get_PassUsage(PassUsage &PU) const {
	return PU;
}

// the address of this variable is used to identify the pass
char DeadcodeEliminationPass::ID = 0;

// register pass
static PassRegistery<DeadcodeEliminationPass> X("DeadcodeEliminationPass");

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
