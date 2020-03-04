/* src/vm/jit/compiler2/InliningPass.cpp - InliningPass

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
#include "toolbox/logging.hpp"
#include "vm/jit/compiler2/BasicBlockCoalescingPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/DominatorPass.hpp"
#include "vm/jit/compiler2/HIRManipulations.hpp"
#include "vm/jit/compiler2/InliningPass.hpp"
#include "vm/jit/compiler2/JITData.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/BasicBlockCoalescingPass"

namespace cacao {
namespace jit {
namespace compiler2 {

void coalesce(BeginInst* first, BeginInst* second)
{
	LOG("coalesce " << first << " " << second << nl);
	auto method = first->get_Method();

	auto new_end_inst = second->get_EndInst();
	first->set_EndInst(new_end_inst);
	second->set_EndInst(NULL);

	for (auto it = method->begin(); it != method->end(); it++) {
		auto inst = *it;
		if (inst->get_BeginInst() == second && inst != second) {
			// TODO inlining: schedule after last inst
			HIRManipulations::move_instruction_to_bb(inst, first, first);
		}
	}

	LOG("merging " << first << " with second: " << second <<nl);
	for (auto it = new_end_inst->succ_begin(); it != new_end_inst->succ_end(); it++) {
		auto succ = (*it).get();
		LOG("Predecessor of " << succ <<nl);
		for (auto itt = succ->pred_begin(); itt != succ->pred_end(); itt++){
			LOG(" " << *itt<<nl);
		}
		succ->remove_predecessor(second);
        succ->append_predecessor(first);
		LOG("Predecessor of " << succ <<nl);
		for (auto itt = succ->pred_begin(); itt != succ->pred_end(); itt++){
			LOG(" " << *itt<<nl);
		}
	}
	LOG("merged " << first << " with second: " << second <<nl);
}

void coalesce_if_possible(BeginInst* begin_inst)
{
	LOG("coalesce_if_possible " << begin_inst << nl);

	auto end_inst = begin_inst->get_EndInst();
	if (end_inst->succ_size() == 0) {
		LOG("Basic block " << begin_inst << " has no successors. Nothing to merge." << nl);
	}
	else if (end_inst->succ_size() > 1) {
		LOG("Basic block " << begin_inst << " has multiple successors. Nothing to merge." << nl);
	}
	else if (end_inst->succ_begin()->get()->pred_size() > 1) {
		auto first_suc = end_inst->succ_begin()->get();
		LOG("Basic block " << first_suc << " has multiple predecessors. Nothing to merge." << nl);
	}
	else {
		BeginInst* first_suc = end_inst->succ_begin()->get();
		LOG("Basic blocks " << begin_inst << " and " << first_suc << " eligible for merge." << nl);
        auto old_end_inst = begin_inst->get_EndInst();
		coalesce(begin_inst, first_suc);
		HIRManipulations::remove_instruction(first_suc);
		HIRManipulations::remove_instruction(old_end_inst);
        first_suc = NULL;
        old_end_inst = NULL;
		coalesce_if_possible(begin_inst);
		return;
	}

	LOG("invoking for successors" << nl);
	for (auto it = end_inst->succ_begin(); it != end_inst->succ_end(); it++) {
		coalesce_if_possible(it->get());
	}
}

bool BasicBlockCoalescingPass::run(JITData& JD)
{
	coalesce_if_possible(JD.get_Method()->get_init_bb());
	return true;
}

// pass usage
PassUsage& BasicBlockCoalescingPass::get_PassUsage(PassUsage& PU) const
{
	PU.after<InliningPass>();
	PU.before<BasicBlockSchedulingPass>();
	PU.before<DominatorPass>();
	return PU;
}

// register pass
static PassRegistry<BasicBlockCoalescingPass> X("BasicBlockCoalescingPass");

Option<bool> BasicBlockCoalescingPass::enabled("BasicBlockCoalescingPass",
                                               "compiler2: enable BasicBlockCoalescingPass",
                                               false,
                                               ::cacao::option::xx_root());

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
