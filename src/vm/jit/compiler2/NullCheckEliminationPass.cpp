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

	alloc::unordered_map<BeginInst*, alloc::vector<int>::type>::type non_null_references_per_block;
	alloc::queue<BeginInst*>::type worklist;
	alloc::unordered_set<BeginInst*>::type visited_blocks;

	worklist.push(M->get_init_bb());

	while (!worklist.empty()) {
		BeginInst *begin = worklist.front();
		worklist.pop();

		if (visited_blocks.count(begin) > 0) {
			continue;
		}

		visited_blocks.insert(begin);

		// Solve data-flow equation at block entry.
		// TODO Consider loop-back-edges.
		auto &non_null_references = non_null_references_per_block[begin];
		alloc::vector<int>::type intersection;
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
			DereferenceInst *dereference = I->to_DereferenceInst();

			if (dereference) {
				Instruction *objectref = dereference->get_objectref();
				assert(objectref);
				assert(objectref->get_type() == Type::ReferenceTypeID);

				if (std::find(non_null_references.begin(), non_null_references.end(), objectref->get_id()) != non_null_references.end()) {
					LOG("Detected redundant null-check on " << objectref  << " by " << I << nl);
					dereference->set_needs_null_check(false);
				} else {
					LOG("Detected non-redundant null-check on " << objectref << " by " << I << nl);
					non_null_references.push_back(objectref->get_id());
				}
			} else if (is_non_null(I)) {
				non_null_references.push_back(I->get_id());
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
