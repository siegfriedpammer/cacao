/* src/vm/jit/compiler2/ScheduleEarlyPass.cpp - ScheduleEarlyPass

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

#include "vm/jit/compiler2/ScheduleEarlyPass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/DominatorPass.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/ScheduleEarly"

namespace cacao {
namespace jit {
namespace compiler2 {

void ScheduleEarlyPass::schedule_early(Instruction *I) {
	LOG("schedule_early: " << I << nl);
	for (Instruction::OperandListTy::const_iterator i = I->op_begin(),
			e = I->op_end(); i != e; ++i) {
		Instruction *op = (*i)->to_Instruction();
		if (op && !op->get_BeginInst()) {
			schedule_early(op);
		}
	}
	if (I->get_BeginInst())
		return;
	BeginInst* block = NULL;
	Instruction::OperandListTy::const_iterator i = I->op_begin();
	Instruction::OperandListTy::const_iterator e = I->op_end();

	if (I->op_size() == 0) {
		I->set_BeginInst(M->get_init_bb());
		return;
	}
	for (;i != e; ++i) {
		Instruction *op = (*i)->to_Instruction();
		if (op) {
			block = op->get_BeginInst();
			++i;
			assert(block);
			break;
		}

	}
	for ( ; i != e; ++i) {
		Instruction *op = (*i)->to_Instruction();
		if (op) {
			BeginInst* i_block = op->get_BeginInst();
			assert(i_block);
			if ( DT->depth(block) < DT->depth(i_block) ) {
				block = i_block;
			}
		}
	}
	assert(block);
	I->set_BeginInst(block);
}

bool ScheduleEarlyPass::run(JITData &JD) {
	DT = get_Pass<DominatorPass>();
	M = JD.get_Method();
	for (Method::InstructionListTy::const_iterator i = M->begin(),
			e = M->end() ; i != e ; ++i) {
		schedule_early(*i);
	}
	return true;
}

PassUsage& ScheduleEarlyPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(DominatorPass::ID);
	return PU;
}
// the address of this variable is used to identify the pass
char ScheduleEarlyPass::ID = 0;

// registrate Pass
static PassRegistery<ScheduleEarlyPass> X("ScheduleEarlyPass");

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
