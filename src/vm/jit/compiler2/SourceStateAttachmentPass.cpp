/* src/vm/jit/compiler2/SourceStateAttachmentPass.cpp - SourceStateAttachmentPass

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

#include "vm/jit/compiler2/SourceStateAttachmentPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/SSAConstructionPass.hpp"
#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "vm/jit/compiler2/GlobalSchedule.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/alloc/queue.hpp"
#include "vm/jit/compiler2/alloc/unordered_set.hpp"

#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/SourceStateAttachment"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

SourceStateInst *get_associated_source_state(Instruction *I) {
	for (auto i = I->rdep_begin(); i != I->rdep_end(); i++) {
		SourceStateInst *source_state = (*i)->to_SourceStateInst();
		if (source_state != NULL) {
			return source_state;
		}
	}
	return NULL;
}

} // end anonymous namespace

SourceStateInst *SourceStateAttachmentPass::process_block(BeginInst *begin,
		SourceStateInst *latest_source_state) {

	assert(begin);

	// This is a CFG merge, therefore it has its own SourceStateInst.
	if (begin->pred_size() > 1) {
		latest_source_state = get_associated_source_state(begin);
	}

	// TODO: java/util/WeakHashMap.expungeStaleEntries()V
	//       triggers this alert here. Investigate and fix bug, for now we just stop
	//       the optimizing compiler run
	// assert(latest_source_state);
	if (!latest_source_state) {
		LOG("SourceStateAttachementPass: no latest source state found for " << begin << "!");
		throw std::runtime_error("SourceStateAttachementPass: no latest source state found!");
	}

	for (auto i = IS->inst_begin(begin); i != IS->inst_end(begin); i++) {
		Instruction *I = *i;

		// Attach the SourceStateInst in case the current Instruction needs one.
		if (I->to_SourceStateAwareInst() && I->to_SourceStateAwareInst()->needs_source_state()) {
			SourceStateAwareInst *sink = I->to_SourceStateAwareInst();
			LOG("Attach " << latest_source_state << " to " << sink->to_Instruction() << nl);
			sink->set_source_state(latest_source_state);
		}

		// Keep track of the latest applicable SourceStateInst.
		if (I->has_side_effects()) {
			latest_source_state = get_associated_source_state(I);
			LOG2("Found source state " << latest_source_state << " for " << I << nl);
			assert(latest_source_state);
		}
	}

	return latest_source_state;
}

bool SourceStateAttachmentPass::run(JITData &JD) {
	M = JD.get_Method();
	IS = get_Artifact<ListSchedulingPass>();

	// The blocks that have not yet been processed.
	alloc::queue<BeginInst*>::type blocks_to_process;

	// For each block in blocks_to_process this holds the corresponding
	// SourceStateInst that is applicable at the entry of the block.
	alloc::queue<SourceStateInst*>::type source_state_at_block_entry;

	// The blocks that have already been processed.
	alloc::unordered_set<BeginInst*>::type processed_blocks;

	BeginInst *init_bb = M->get_init_bb();
	SourceStateInst *init_source_state = get_associated_source_state(init_bb);

	blocks_to_process.push(init_bb);
	source_state_at_block_entry.push(init_source_state);

	while (!blocks_to_process.empty()) {
		BeginInst *begin = blocks_to_process.front();
		blocks_to_process.pop();
		SourceStateInst *latest_source_state = source_state_at_block_entry.front();
		source_state_at_block_entry.pop();

		// Ignore blocks that have already been processed.
		if (processed_blocks.count(begin) > 0) {
			continue;
		}

		latest_source_state = process_block(begin, latest_source_state);
		processed_blocks.insert(begin);

		EndInst *end = begin->get_EndInst();
		for (auto i = end->succ_begin(); i != end->succ_end(); i++) {
			BeginInst *successor = (*i).get();
			blocks_to_process.push(successor);
			source_state_at_block_entry.push(latest_source_state);
		}
	}

	return true;
}

// pass usage
PassUsage& SourceStateAttachmentPass::get_PassUsage(PassUsage &PU) const {
	PU.requires<HIRInstructionsArtifact>();
	PU.requires<ListSchedulingPass>();
	PU.before<MachineInstructionSchedulingPass>();
	PU.modifies<ListSchedulingPass>();
	return PU;
}

// register pass
static PassRegistry<SourceStateAttachmentPass> X("SourceStateAttachmentPass");

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
