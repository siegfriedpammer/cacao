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
#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/SourceStateAttachmentPass.hpp"
#include "vm/jit/compiler2/alloc/queue.hpp"

#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/NullCheckEliminationPass"

namespace cacao {
namespace jit {
namespace compiler2 {

bool NullCheckEliminationPass::is_trivially_non_null(Value *objectref) {
	Instruction *I = objectref->to_Instruction();

	if (I->to_LOADInst()) {
		LOADInst *load = I->to_LOADInst();
		// A LOADInst is known to be non-null if it is a reference
		// to the this-pointer within an instance-method.
		return !M->is_static() && load->get_index() == 0;
	}

	// Instructions that create new objects are trivially non-null.
	return I->to_NEWInst()
		|| I->to_NEWARRAYInst()
		|| I->to_ANEWARRAYInst()
		|| I->to_MULTIANEWARRAYInst();
}

void NullCheckEliminationPass::map_referenes_to_bitpositions() {
	int num_of_bits = 0;
	for (auto it = M->begin(); it != M->end(); it++) {
		Instruction *I = *it;
		if (I->get_type() == Type::ReferenceTypeID) {
			bitpositions[I] = num_of_bits++;
			LOG2("Map " << I << " to bitposition " << bitpositions[I] << nl);
		}
	}
}

void NullCheckEliminationPass::prepare_bitvectors() {
	// Prepare the initial bitevector for each block, 
	for (auto bb_it = M->bb_begin(); bb_it != M->bb_end(); bb_it++) {
		BeginInst *begin = *bb_it;
		non_null_references_at_entry[begin].resize(bitpositions.size());
		non_null_references_at_entry[begin].set();
		non_null_references_at_exit[begin].resize(bitpositions.size());
		non_null_references_at_exit[begin].set();
	}

	// At method entry we assume that each object reference may be null.
	non_null_references_at_entry[M->get_init_bb()].reset();
}

void NullCheckEliminationPass::perform_null_check_elimination() {
	ListSchedulingPass *schedule = get_Artifact<ListSchedulingPass>();

	alloc::queue<BeginInst*>::type worklist;
	alloc::unordered_map<BeginInst*, bool>::type in_worklist(M->bb_size());

	worklist.push(M->get_init_bb());
	in_worklist[M->get_init_bb()] = true;

	while (!worklist.empty()) {
		BeginInst *begin = worklist.front();
		worklist.pop();
		in_worklist[begin] = false;

		// The local analysis state.
		auto &non_null_references = non_null_references_at_exit[begin];

		// Initialize the local state to be equal to the entry state.
		non_null_references = non_null_references_at_entry[begin];

		LOG2("Process " << begin << nl);

		// Compute data-flow information for each instruction within the current block.
		for (auto inst_it = schedule->inst_begin(begin);
				inst_it != schedule->inst_end(begin); inst_it++) {
			Instruction *I = *inst_it;

			if (I->get_type() == Type::ReferenceTypeID) {
				if (is_trivially_non_null(I)) {
					LOG2(I << " is trivially non-null" << nl);
					non_null_references[bitpositions[I]] = true;
				} else if (I->to_PHIInst()) {
					bool meet = true;
					for (auto op_it = I->op_begin(); op_it != I->op_end(); op_it++) {
						Instruction *op = (*op_it)->to_Instruction();
						meet &= non_null_references[bitpositions[op]];
					}
					non_null_references[bitpositions[I]] = meet;
				}
			}

			if (I->to_DereferenceInst() != nullptr) {
				DereferenceInst *dereference = I->to_DereferenceInst();
				Instruction *objectref = dereference->get_objectref();

				assert(objectref);
				assert(objectref->get_type() == Type::ReferenceTypeID);

				if (non_null_references[bitpositions[objectref]]) {
					// The objectref that is dereferenced by the current
					// instruction is known to be non-null, therefore no
					// null-check has to be performed at this point.
					dereference->set_needs_null_check(false);
					LOG2("Remove null-check on " << objectref << " by " << I << nl);
				} else {
					// The objectref that is dereferenced by the current
					// instruction may be null, therefore a null-check has to
					// be performed at this point.
					dereference->set_needs_null_check(true);
					non_null_references[bitpositions[objectref]] = true;
					LOG2("Introduce null-check on " << objectref << " by " << I << nl);
				}
			}
		}

		// Process successors of the current block.
		EndInst *end = begin->get_EndInst();
		for (auto succ_it = end->succ_begin(); succ_it != end->succ_end(); succ_it++) {
			BeginInst *succ = (*succ_it).get();

			auto num_of_set_bits = non_null_references_at_entry[succ].count();

			// Compute the analysis state for the entry of the successor block.
			non_null_references_at_entry[succ] &= non_null_references;

			auto new_num_of_set_bits = non_null_references_at_entry[succ].count();

			// In case the analysis state at the successor changed, the
			// successor has to be reanalyzed as well.
			bool reanalyze_successor = num_of_set_bits != new_num_of_set_bits;
			if (reanalyze_successor && !in_worklist[succ]) {
				worklist.push(succ);
				in_worklist[succ] = true;
			}
		}
	}
}

#ifdef ENABLE_LOGGING
void NullCheckEliminationPass::print_final_results() {
	for (auto inst_it = M->begin(); inst_it != M->end(); inst_it++) {
		Instruction *I = *inst_it;
		DereferenceInst *dereference = I->to_DereferenceInst();

		if (dereference) {
			Instruction *objectref = dereference->get_objectref();
			if (!dereference->get_needs_null_check()) {
				LOG("Null-check on " << objectref << " by " << I << " is redundant" << nl);
			} else {
				LOG("Null-check on " << objectref << " by " << I << " is not redundant" << nl);
			}
		}
	}
}
#endif

bool NullCheckEliminationPass::run(JITData &JD) {
	M = JD.get_Method();

	map_referenes_to_bitpositions();
	prepare_bitvectors();
	perform_null_check_elimination();
#ifdef ENABLE_LOGGING
	print_final_results();
#endif

	return true;
}

// pass usage
PassUsage& NullCheckEliminationPass::get_PassUsage(PassUsage &PU) const {
	PU.requires<ScheduleClickPass>();
	PU.requires<ListSchedulingPass>();
	PU.before<SourceStateAttachmentPass>();
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
