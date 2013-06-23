/* src/vm/jit/compiler2/ScheduleClickPass.cpp - ScheduleClickPass

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

#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "vm/jit/compiler2/ScheduleEarlyPass.hpp"
#include "vm/jit/compiler2/ScheduleLatePass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/DominatorPass.hpp"
#include "vm/jit/compiler2/LoopPass.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/ScheduleClick"

namespace cacao {
namespace jit {
namespace compiler2 {

void ScheduleClickPass::schedule(Instruction *I) {
	// do not schedule non-floating instructions
	if (!I->is_floating())
	  return;
	/**
	 * We want to schedule a block as late as possible, but
	 * outside of loop bodies (except for constants)
	 * In most cases constants can be encoded in the instructions.
	 * @todo register pressure
	 * @todo evaluate this!
	 */
	BeginInst* latest = late->get(I);
	BeginInst* best = latest;
	if (I->get_opcode() != Instruction::CONSTInstID) {
		BeginInst* earliest = DT->get_idominator(early->get(I));

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
	}
	LOG("scheduled to " << best << nl);
	// set the basic block
	I->set_BeginInst(best);
}

bool ScheduleClickPass::run(JITData &JD) {
	M = JD.get_Method();
	DT = get_Pass<DominatorPass>();
	LT = get_Pass<LoopPass>();
	early = get_Pass<ScheduleEarlyPass>();
	late = get_Pass<ScheduleLatePass>();
	for (Method::InstructionListTy::const_iterator i = M->begin(),
			e = M->end() ; i != e ; ++i) {
		Instruction *I = *i;
		LOG("schedule_click: " << I << nl);
		BeginInst* latest = late->get(I);
		BeginInst* earliest = early->get(I);
		// walk up the dominator tree
		for (; earliest != latest; latest = DT->get_idominator(latest)) {
			LOG(latest << " LoopLevel: " << LT->loop_nest(LT->get_Loop(latest)) << nl);
		}
		LOG(latest << " LoopLevel: " << LT->loop_nest(LT->get_Loop(latest)) << nl);

		schedule(I);
	}
	set_schedule(M);
	// clear schedule
	M->clear_schedule();
	return true;
}

PassUsage& ScheduleClickPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(DominatorPass::ID);
	PU.add_requires(ScheduleEarlyPass::ID);
	PU.add_requires(ScheduleLatePass::ID);
	PU.add_requires(LoopPass::ID);
	return PU;
}
// the address of this variable is used to identify the pass
char ScheduleClickPass::ID = 0;

// registrate Pass
static PassRegistery<ScheduleClickPass> X("ScheduleClickPass");

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
