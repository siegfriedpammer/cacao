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
#include "vm/jit/compiler2/ScheduleLatePass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/Instruction.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/ScheduleClick"

namespace cacao {
namespace jit {
namespace compiler2 {

bool ScheduleClickPass::run(JITData &JD) {
	// XXX Currently the logic is implemented in ScheduleLatePass
	// this pass only copies the result. The reason is that
	// final scheduling must be performed interleaved with schedule
	// late. This might change if we run passes on instruction rather
	// then on global scope. Therefor this is kept as a placeholder.
	M = JD.get_Method();
	late = get_Pass<ScheduleLatePass>();
	for (Method::InstructionListTy::const_iterator i = M->begin(),
			e = M->end() ; i != e ; ++i) {
		Instruction *I = *i;
		LOG1("schedule_click: " << I << nl);
		if (I->is_floating()) {
			I->set_BeginInst(late->get(I));
		}
	}
	set_schedule(M);
	// clear schedule
	M->clear_schedule();
	return true;
}

bool ScheduleClickPass::verify() const {
	for (Method::const_iterator i = M->begin(), e = M->end(); i!=e; ++i) {
		Instruction *I = *i;
		if (!get(I)) {
			ERROR_MSG("Instruction not Scheduled!","Instruction: " << I);
			return false;
		}
	}
	return true;
}
PassUsage& ScheduleClickPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<ScheduleLatePass>();
	return PU;
}
// the address of this variable is used to identify the pass
char ScheduleClickPass::ID = 0;

// registrate Pass
static PassRegistry<ScheduleClickPass> X("ScheduleClickPass");

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
