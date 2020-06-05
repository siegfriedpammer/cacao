/* src/vm/jit/compiler2/ReversePostOrderPass.cpp - ReversePostOrderPass

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

#include "vm/jit/compiler2/ReversePostOrderPass.hpp"

#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineLoopPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/ReversePostOrderPass"

namespace cacao {
namespace jit {
namespace compiler2 {

bool ReversePostOrderPass::run(JITData& JD)
{
	auto MIS = get_Artifact<LIRControlFlowGraphArtifact>()->MIS;

	// Reset all basic blocks
	for (auto block : *MIS) {
		block->mark_unprocessed();
	}
	build_reverse_post_order(MIS->front());

	return true;
}

void ReversePostOrderPass::build_reverse_post_order(MachineBasicBlock* block)
{
	auto loop_tree = get_Artifact<MachineLoopPass>();
	for (auto i = block->back()->successor_begin(), e = block->back()->successor_end(); i != e;
	     ++i) {
		auto succ = *i;
		// Only visit the successor if it is unproccessed and not a loop-back edge
		if (!succ->is_processed() && !loop_tree->is_backedge(block, succ)) {
			build_reverse_post_order(succ);
		}
	}

	reverse_post_order.push_front(block);
	block->mark_processed();
}

// pass usage
PassUsage& ReversePostOrderPass::get_PassUsage(PassUsage& PU) const
{
	PU.provides<ReversePostOrderPass>();
	PU.requires<LIRControlFlowGraphArtifact>();
	PU.requires<MachineLoopPass>();
	return PU;
}

// register pass
static PassRegistry<ReversePostOrderPass> X("ReversePostOrderPass");
static ArtifactRegistry<ReversePostOrderPass> Y("ReversePostOrderPass");

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
