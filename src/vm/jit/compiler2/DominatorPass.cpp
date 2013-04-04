/* src/vm/jit/compiler2/DominatorPass.cpp - DominatorPass

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

#include "vm/jit/compiler2/DominatorPass.hpp"
#include "vm/jit/compiler2/Instructions.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/DominatorPass"

namespace cacao {
namespace jit {
namespace compiler2 {

DominatorPass::NodeListTy& DominatorPass::succ(NodeTy *v, DominatorPass::NodeListTy& list) {
	EndInst *ve = v->get_EndInst();
	assert(ve);
	list.insert(ve->succ_begin(),ve->succ_end());
	return list;
}

void DominatorPass::DFS(NodeTy * v) {
	assert(v);
	semi[v] = ++n;
	vertex[n] = v;
	// TODO this can be done better
	NodeListTy succ_v;
	succ_v = succ(v, succ_v);
	LOG("number of succ for " << (long)v << " (" << n << ") " << " = " << succ_v.size() << nl);
	for(NodeListTy::const_iterator i = succ_v.begin() , e = succ_v.end();
			i != e; ++i) {
		NodeTy *w = *i;
		if (semi[w] == 0) {
			parent[w] = v;
			DFS(w);
		}
		pred[w].insert(v);
	}
}

bool DominatorPass::run(JITData &JD) {
	Method *M = JD.get_Method();
	// initialize
	n = 0;
	vertex.resize(M->bb_size() + 1); // +1 because we start at 1 not at 0
	for(Method::BBListTy::const_iterator i = M->bb_begin(), e = M->bb_end() ;
			i != e; ++i) {
		// pred[*i] = NodeListTy(); // maps are auto initialized at first access
		semi[*i] = 0;
	}
	DFS(M->get_init_bb());

	for (int i = 1 ; i < vertex.size(); ++i) {
		LOG("index" << setw(3) << i << " BeginInst" << (long)vertex[i] << nl);
	}

	return true;
}

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

