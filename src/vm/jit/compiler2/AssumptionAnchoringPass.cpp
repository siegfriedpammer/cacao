/* src/vm/jit/compiler2/AssumptionAnchoringPass.cpp - AssumptionAnchoringPass

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

#include "vm/jit/compiler2/AssumptionAnchoringPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/InstructionMetaPass.hpp"
#include "vm/jit/compiler2/DominatorPass.hpp"
#include "vm/jit/compiler2/ScheduleEarlyPass.hpp"
#include "vm/jit/compiler2/SSAPrinterPass.hpp"
#include "vm/jit/compiler2/SourceStateAttachmentPass.hpp"
#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/AssumptionAnchoringPass"

namespace cacao {
namespace jit {
namespace compiler2 {

/**
 * Finds the nearest dominator of @p begin that is the start of the branch
 * that contains @p begin.
 */
BeginInst *AssumptionAnchoringPass::find_nearest_branch_begin(BeginInst *begin) {
	BeginInst *current_begin = begin;
	BeginInst *idom = dominator_tree->get_idominator(current_begin);
	while (current_begin != method->get_init_bb()
			&& idom->get_EndInst()->succ_size() <= 1) {
		current_begin = idom;
		idom = dominator_tree->get_idominator(current_begin);
	}
	return current_begin;
}

/**
 * Makes sure that @p deopt is anchored to a proper BeginInst.
 */
void AssumptionAnchoringPass::anchor(AssumptionInst *assumption) {
	BeginInst *anchor;

	if (/* Some guarded instruction is floating */ true) {
		// Anchor the AssumptionInst to the first BeginInst of the method.
		anchor = method->get_init_bb();
	} else {
		// Make sure that the AssumptionInst does not leave the branch of the
		// instructions that it guards. Therefore the AssumptionInst has to be
		// anchored to the BeginInst that starts the current branch.

		// TODO
	}

	assert(anchor);
	assumption->append_dep(anchor);
	LOG("anchored " << assumption << " to " << anchor << nl);
}

bool AssumptionAnchoringPass::run(JITData &JD) {
	method = JD.get_Method();
	dominator_tree = get_Pass<DominatorPass>();

	for (Method::const_iterator i = method->begin(), e = method->end(); i != e;
			++i) {
		Instruction *I = *i;
		if (I->to_AssumptionInst()) {
			AssumptionInst *deopt = I->to_AssumptionInst();
			anchor(deopt);
		}
	}
	return true;
}

// pass usage
PassUsage& AssumptionAnchoringPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<InstructionMetaPass>();
	PU.add_requires<DominatorPass>();
	PU.add_schedule_before<ScheduleEarlyPass>();
	PU.add_schedule_before<SourceStateAttachmentPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char AssumptionAnchoringPass::ID = 0;

// register pass
#if defined(ENABLE_REPLACEMENT)
//static PassRegistry<AssumptionAnchoringPass> X("AssumptionAnchoringPass");
#endif

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
