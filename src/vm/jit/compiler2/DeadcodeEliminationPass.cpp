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
#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/deadcodeeliminationpass"

namespace cacao {
namespace jit {
namespace compiler2 {

bool DeadcodeEliminationPass::run(JITData &JD) {
	Method *M = JD.get_Method();

	Method::InstructionListTy workList(M->begin(), M->end());

	// will be used to look up whether an instruction is currently contained in the
	// worklist to avoid inserting an instruction which is already in the list.
	// TODO: verify if we ca be sure that ids of instructions are consecutive and
	// starting from zero
	std::vector<bool> inWorkList(workList.size(), true);

	// will be used to mark instructions as dead.
	// TODO: verify if we ca be sure that ids of instructions are consecutive and
	// starting from zero
	std::vector<bool> dead(workList.size(), false);

	while (!workList.empty()) {
		Instruction *I = workList.front();
		workList.pop_front();
		inWorkList[I->get_id()] = false;

		if (I->user_size() == 0) {
			if (I->get_opcode() == Instruction::BeginInstID) {
				// TODO: remove BeginInst only if its corresponding
				// "end" instruction is dead
			} else if (I->get_opcode() != Instruction::RETURNInstID) {
				// TODO: consider here all instructions that could possibly
				// influence the method output

				dead[I->get_id()] = true;
				// TODO: remove instruction from the user list of its operands and place
				// each operand on the workList
			}
		}
	}

	return true;
}

// pass usage
PassUsage& DeadcodeEliminationPass::get_PassUsage(PassUsage &PU) const {
	//PU.add_requires(YyyPass::ID);
    //PU.add_modifies(YyyPass::ID);
    //PU.add_destroys(YyyPass::ID);
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
