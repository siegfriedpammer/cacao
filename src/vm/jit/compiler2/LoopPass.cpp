/* src/vm/jit/compiler2/LoopPass.cpp - LoopPass

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

#include "vm/jit/compiler2/LoopPass.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/GraphHelper.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/DominatorPass.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/LoopPass"

namespace cacao {
namespace jit {
namespace compiler2 {

///////////////////////////////////////////////////////////////////////////////
// LoopTree
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// LoopPass
///////////////////////////////////////////////////////////////////////////////

bool LoopPass::run(JITData &JD) {
	Method *M = JD.get_Method();
	DFSTraversal<BeginInst> dfs(M->get_init_bb());
	// get dominator tree
	DominatorTree *DT = get_Pass<DominatorPass>();
	assert(DT);

	for (DFSTraversal<BeginInst>::iterator i = dfs.begin(), e = dfs.end();
			i != e ; ++i) {
		DFSTraversal<BeginInst>::iterator_list successors;
		i.get_successors(successors);
		LOG("BI:" << i.get_index() << ". num succssors: " << successors.size() << nl);
		for (DFSTraversal<BeginInst>::iterator_list::iterator ii = successors.begin(),
				ee = successors.end(); ii != ee ; ++ii) {
			if (i > *ii) {
				LOG("edge " << (long) *i << " (" << i.get_index() << ") -> "
					<< (long) *(*ii) << " (" << (*ii).get_index() << ") is a backedge" << nl);
				LOG("node " << (long) *(*ii) << " (" << (*ii).get_index() << ") is a loop header" << nl);
				LOG("node " << (long) *i << " (" << i.get_index() << ") is a loop exit " << nl);
			}
		}
	}
	return true;
}

PassUsage& LoopPass::get_PassUsage(PassUsage &PA) const {
	PA.add_required(DominatorPass::ID);
	return PA;
}

// the address of this variable is used to identify the pass
char LoopPass::ID = 0;

// register pass
static PassRegistery<LoopPass> X("LoopPass");

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

