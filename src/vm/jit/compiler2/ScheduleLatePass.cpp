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
#include "vm/jit/compiler2/ScheduleEarlyPass.hpp"
#include "vm/jit/compiler2/LoopPass.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/ScheduleLate"

namespace cacao {
namespace jit {
namespace compiler2 {

void ScheduleLatePass::schedule_late(Instruction *I) {
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
	LOG1("schedule_late: " << I << nl);
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
	/**
	 * We want to schedule a block as late as possible, but
	 * outside of loop bodies (except for constants)
	 * In most cases constants can be encoded in the instructions.
	 * @todo register pressure
	 * @todo evaluate this!
	 */
	BeginInst* latest = block;
	BeginInst* best = latest;
	//if (I->get_opcode() != Instruction::CONSTInstID) {
	BeginInst* earliest = DT->get_idominator(early->get(I));
	LOG1("Sched.Late: " << latest << nl);
	LOG1("Sched.Early: " << early->get(I) << nl);

	while (latest != earliest) {
		Loop* loop_latest = LT->get_Loop(latest);
		Loop* loop_best = LT->get_Loop(best);
		// if the best is in an inner loop
		LOG2( "Loop best: " << best << " " << LT->loop_nest(loop_best)
		  << " Loop latest: " << latest << " " << LT->loop_nest(loop_latest) << nl);
		if ( LT->is_inner_loop(loop_best, loop_latest) ) {
			best = latest;
		}
		latest = DT->get_idominator(latest);
	}
	//}
	LOG("scheduled to " << best << nl);
	// set the basic block
	I->set_BeginInst(best);
}

bool ScheduleLatePass::run(JITData &JD) {
	DT = get_Pass<DominatorPass>();
	LT = get_Pass<LoopPass>();
	early = get_Pass<ScheduleEarlyPass>();
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
	PU.add_requires<DominatorPass>();
	PU.add_requires<ScheduleEarlyPass>();
	PU.add_requires<LoopPass>();
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
