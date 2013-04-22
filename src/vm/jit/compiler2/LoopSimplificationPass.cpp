/* src/vm/jit/compiler2/LoopSimplificationPass.cpp - LoopSimplificationPass

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

#include "vm/jit/compiler2/LoopSimplificationPass.hpp"
#include "vm/jit/compiler2/LoopPass.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/LoopSimplificationPass"

namespace cacao {
namespace jit {
namespace compiler2 {

///////////////////////////////////////////////////////////////////////////////
// LoopSimplificationPass
///////////////////////////////////////////////////////////////////////////////

void LoopSimplificationPass::check_loop(Loop *loop) const {
	// of the we have a shared loop header
	assert(LT);
	BeginInst* header = loop->get_header();
	if (LT->get_Loop(header) != loop) {
		LOG("Loop with shared header:" << header << nl);
		/*
		 * We have a BeginInst which is the header of more than one loop.
		 * We create a new header block for the _outer_ loop (stored in the
		 * variable loop). All edges from outside the loop and the backedge
		 * of the current loop will be redirected to the new header block.
		 */
		std::set<BeginInst*> outer_edges;

		for (BeginInst::PredecessorListTy::const_iterator i = header->pred_begin(),
				e = header->pred_end(); i != e; ++i) {
			BeginInst *pred = *i;
			assert(pred);
			Loop *pred_loop = LT->get_Loop(pred);

			if (!LT->is_inner_loop(pred_loop,loop)) {
				// edge from outside -> replace target
				// NOTE: We can not handle this edges now because that would mess up our
				// iterator. Therefor we store them in this list.
				outer_edges.insert(pred);
			}
		}
		// create new loop header
		BeginInst* new_header = new BeginInst();
		// create jump to the old header
		EndInst* new_exit = new GOTOInst(new_header,header);
		for (std::set<BeginInst*>::iterator i = outer_edges.begin(), e = outer_edges.end() ;
				i != e ; ++i) {
			EndInst *end = (*i)->get_EndInst();
			assert(end && "Can not replace a successor of a BeginInst without an EndInst!");
			end->replace_succ(header, new_header);
		}

		// update loop information
		// Loop
		loop->header = new_header;
		// LoopTree
		LT->loop_map[new_header] = loop;


		// install new instructions
		M->add_bb(new_header);
		M->add_Instruction(new_exit);

		// XXX we have to do something with the phi instructions
	}
	for (Loop::loop_iterator i = loop->loop_begin(), e = loop->loop_end();
			i != e; ++i) {
		check_loop(*i);
	}
}

bool LoopSimplificationPass::run(JITData &JD) {

	M = JD.get_Method();
	assert(M);
	LT = get_Pass<LoopPass>();
	assert(LT);

	for (Loop::loop_iterator i = LT->loop_begin(), e = LT->loop_end();
			i != e; ++i) {
		check_loop(*i);
	}

	#ifndef NDEBUG
	LOG("Verify LoopSimplificationPass result" << nl);
	// verify the result
	std::list<Loop*> loops(LT->loop_begin(), LT->loop_end());
	while(!loops.empty()) {
		Loop *loop = loops.front();
		loops.pop_front();
		assert(LT->get_Loop(loop->get_header()) == loop);
		for (Loop::loop_iterator i = loop->loop_begin(), e = loop->loop_end();
				i != e; ++i) {
			loops.push_back(*i);
		}
	}
	#endif

	return true;
}

PassUsage& LoopSimplificationPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(LoopPass::ID);
	PU.add_modifies(LoopPass::ID);
	return PU;
}
// the address of this variable is used to identify the pass
char LoopSimplificationPass::ID = 0;

// register pass
static PassRegistery<LoopSimplificationPass> X("LoopSimplificationPass");

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

