/* src/vm/jit/compiler2/NullCheckEliminationPass.cpp - NullCheckEliminationPass

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

#include <algorithm>
#include <cassert>

#include "vm/jit/compiler2/NullCheckEliminationPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/InstructionMetaPass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/SourceStateAttachmentPass.hpp"
#include "vm/jit/compiler2/alloc/queue.hpp"
#include "vm/jit/compiler2/alloc/unordered_map.hpp"
#include "vm/jit/compiler2/alloc/unordered_set.hpp"
#include "vm/jit/compiler2/alloc/vector.hpp"

#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/NullCheckEliminationPass"

namespace cacao {
namespace jit {
namespace compiler2 {

bool NullCheckEliminationPass::is_non_null(Value *objectref) {
	Instruction *I = objectref->to_Instruction();

	if (I->to_LOADInst()) {
		LOADInst *load = I->to_LOADInst();
		// A LOADInst is known to be non-null if it is a reference
		// to the this-pointer within an instance-method.
		return !M->is_static() && load->get_index();
	}

	// Instructions that create new objects are trivially non-null.
	return I->to_NEWInst()
		|| I->to_NEWARRAYInst()
		|| I->to_ANEWARRAYInst()
		|| I->to_MULTIANEWARRAYInst();
}

bool NullCheckEliminationPass::run(JITData &JD) {
	M = JD.get_Method();
	ListSchedulingPass *IS = get_Pass<ListSchedulingPass>();

	alloc::unordered_set<Instruction*>::type safe_dereferences;
	alloc::unordered_map<BeginInst*, alloc::vector<Value*>::type>::type non_null_references_per_block;
	alloc::queue<BeginInst*>::type worklist;

	while (!worklist.empty()) {
		BeginInst *begin = worklist.front();
		worklist.pop();

		// Solve data-flow equation at block entry, ignoring loop back-edges.
		auto &non_null_references = non_null_references_per_block[begin];
		alloc::vector<Value*>::type intersection;
		for (auto pred_it = begin->pred_begin(); pred_it != begin->pred_end();
				pred_it++) {
			BeginInst *pred = *pred_it;
			auto &pred_non_null_references = non_null_references_per_block[pred];

			if (pred_it == begin->pred_begin()) {
				std::copy(pred_non_null_references.begin(), pred_non_null_references.end(),
						std::back_inserter(non_null_references));
			} else if (pred->get_id() < begin->get_id()) {
				std::set_intersection(non_null_references.begin(), non_null_references.end(),
						pred_non_null_references.begin(), pred_non_null_references.end(),
						std::back_inserter(intersection));
				non_null_references.clear();
				std::copy(intersection.begin(), intersection.end(),
						std::back_inserter(non_null_references));
				intersection.clear();
			}
		}

		// Compute data-flow information for each instruction within the current block.
		for (auto inst_it = IS->inst_begin(begin); inst_it != IS->inst_end(begin); inst_it++) {
			Instruction *I = *inst_it;

			if (I->is_dereference()) {
				Value *objectref = I->get_operand(0);
				if (std::find(non_null_references.begin(), non_null_references.end(), objectref) != non_null_references.end()) {
					safe_dereferences.insert(I);
				} else {
					non_null_references.push_back(objectref);
				}
			} else if (is_non_null(I)) {
				non_null_references.push_back(I);
			}
		}

		std::sort(non_null_references.begin(), non_null_references.end());

		// Process successors of the current block.
		EndInst *end = begin->get_EndInst();
		for (auto succ_it = end->succ_begin(); succ_it != end->succ_end(); succ_it++) {
			BeginInst *succ = (*succ_it).get();
			if (succ->get_id() > begin->get_id()) {
				worklist.push(succ);
			}
		}
	}

	// Collect redundant null-checks.
	alloc::vector<CHECKNULLInst*>::type redundant_null_checks;
	for (auto inst_it = M->begin(); inst_it != M->end(); inst_it++) {
		CHECKNULLInst *null_check = (*inst_it)->to_CHECKNULLInst();
		if (null_check) {
			Value *dereference = *(null_check->rdep_begin());
			if (null_check->rdep_size() == 0
					|| safe_dereferences.count(dereference->to_Instruction()) > 0) {
				redundant_null_checks.push_back(null_check);
			}
		}
	}

	// Remove redundant null-checks.
	while (!redundant_null_checks.empty()) {
		CHECKNULLInst *null_check = redundant_null_checks.back();
		redundant_null_checks.pop_back();
		LOG("Remove null-check " << null_check << nl);
		for (auto rdep_it = null_check->rdep_begin(); rdep_it != null_check->rdep_end(); rdep_it++) {
			Instruction *rdep = *rdep_it;
			rdep->remove_dep(null_check);
		}
	}

	return true;
}

// pass usage
PassUsage& NullCheckEliminationPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<ScheduleClickPass>();
	PU.add_requires<ListSchedulingPass>();
	PU.add_schedule_before<SourceStateAttachmentPass>();
	return PU;
}

// register pass
static PassRegistry<NullCheckEliminationPass> X("NullCheckEliminationPass");

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
