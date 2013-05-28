/* src/vm/jit/compiler2/ScheduleLatePass.cpp - ScheduleLatePass

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

#include "vm/jit/compiler2/ScheduleLatePass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/DominatorPass.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/ScheduleLate"

namespace cacao {
namespace jit {
namespace compiler2 {

void ScheduleLatePass::schedule_late(Instruction *I) {
	LOG("schedule_late: " << I << nl);
	for (Value::UserListTy::const_iterator i = I->user_begin(),
			e = I->user_end(); i != e; ++i) {
		Instruction *user = (*i);
		assert(user);
		if (!user->get_BeginInst()) {
			schedule_late(user);
		}
	}
	if (I->get_BeginInst())
		return;
	BeginInst* block = NULL;
	for (Value::UserListTy::const_iterator i = I->user_begin(),
			e = I->user_end(); i != e; ++i) {
		Instruction *user = (*i);
		assert(user);
		BeginInst* user_block = user->get_BeginInst();
		assert(user_block);
		PHIInst *phi = user->to_PHIInst();
		if (phi) {
			int index = phi->get_operand_index(I);
			assert(index >= 0);
			BeginInst *pred = phi->get_BeginInst()->get_predecessor(index);
			block = DT->find_nearest_common_dom(block,pred);
		} else {
			block = DT->find_nearest_common_dom(block,user->get_BeginInst());
		}
	}
	assert(block);
	I->set_BeginInst(block);
}

bool ScheduleLatePass::run(JITData &JD) {
	DT = get_Pass<DominatorPass>();
	M = JD.get_Method();
	for (Method::InstructionListTy::const_iterator i = M->begin(),
			e = M->end() ; i != e ; ++i) {
		if ((*i)->is_floating()) {
			(*i)->set_BeginInst(NULL);
		}
	}
	for (Method::InstructionListTy::const_iterator i = M->begin(),
			e = M->end() ; i != e ; ++i) {
		schedule_late(*i);
	}
	set_schedule(M);
	// clear schedule
	M->clear_schedule();
	return true;
}

PassUsage& ScheduleLatePass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(DominatorPass::ID);
	return PU;
}
// the address of this variable is used to identify the pass
char ScheduleLatePass::ID = 0;

// registrate Pass
static PassRegistery<ScheduleLatePass> X("ScheduleLatePass");

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
