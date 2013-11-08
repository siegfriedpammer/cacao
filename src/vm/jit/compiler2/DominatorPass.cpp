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
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/CFGMetaPass.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/DominatorPass"

namespace cacao {
namespace jit {
namespace compiler2 {

///////////////////////////////////////////////////////////////////////////////
// DominatorTree
///////////////////////////////////////////////////////////////////////////////
bool DominatorTree::dominates(NodeTy *a, NodeTy *b) const {
	for ( ; b != NULL ; b = get_idominator(b) ) {
		if (a == b)
		  return true;
	}
	return false;
}

DominatorTree::NodeTy* DominatorTree::find_nearest_common_dom(NodeTy *a, NodeTy *b) const {
	// trivial cases
	if (!a)
		return b;
	if (!b)
		return a;
	if (dominates(a,b))
	  return a;
	if (dominates(b,a))
	  return b;
	// collect a's dominators
	std::set<NodeTy*> dom_a;
	while ( (a = get_idominator(a)) ) {
		dom_a.insert(a);
	}

	// search nearest common dominator
	for(std::set<NodeTy*>::const_iterator e = dom_a.end(); b != NULL ; b = get_idominator(b) ) {
		if (dom_a.find(b) != e) {
			return b;
		}
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// DominatorPass
///////////////////////////////////////////////////////////////////////////////

DominatorPass::NodeListTy& DominatorPass::succ(NodeTy *v, NodeListTy& list) {
	EndInst *ve = v->get_EndInst();
	assert(ve);
	for (EndInst::SuccessorListTy::const_iterator i = ve->succ_begin(),
			e = ve->succ_end(); i != e; ++i) {
		list.insert(i->get());
	}
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

// simple versions
void DominatorPass::Link(NodeTy *v, NodeTy *w) {
	ancestor[w] = v;
}

DominatorPass::NodeTy* DominatorPass::Eval(NodeTy *v) {
	if (ancestor[v] == 0) {
		return v;
	} else {
		return label[v];
	}
}

void DominatorPass::Compress(NodeTy *v) {
	// this preocedure assumes ancestor[v] != 0
	assert(ancestor[v]);
	if (ancestor[ancestor[v]] != 0) {
		Compress(ancestor[v]);
		if (semi[label[ancestor[v]]] < semi[label[v]]) {
			label[v] = label[ancestor[v]];
		}
		ancestor[v] = ancestor[ancestor[v]];
	}
}

bool DominatorPass::run(JITData &JD) {
	Method *M = JD.get_Method();
	// initialize
	n = 0;
	vertex.resize(M->bb_size() + 1); // +1 because we start at 1 not at 0
	for(Method::BBListTy::const_iterator i = M->bb_begin(), e = M->bb_end() ;
			i != e; ++i) {
		NodeTy *v = *i;
		// pred[v] = NodeListTy(); // maps are auto initialized at first access
		semi[v] = 0;
		// init label and ancestor array
		ancestor[v] = 0;
		label[v] = v;
	}
	DFS(M->get_init_bb());

	LOG("DFS:" << nl);
	for (int i = 1 ; i <= n; ++i) {
		LOG("index" << setw(3) << i << " " << (long)vertex[i] << nl);
	}

	// from n to 2
	for (int i = n; i >= 2; --i) {
		NodeTy *w = vertex[i];
		// step 2
		NodeListTy &pred_w = pred[w];
		for (NodeListTy::const_iterator it = pred_w.begin(), et = pred_w.end();
				it != et; ++it) {
			NodeTy *v = *it;
			NodeTy *u = Eval(v);
			if (semi[u] < semi[w]) {
				semi[w] = semi[u];
			}
		}
		bucket[vertex[semi[w]]].insert(w);
		Link(parent[w],w);
		// step 3
		NodeListTy &bucket_p_w = bucket[parent[w]];
		for (NodeListTy::const_iterator it = bucket_p_w.begin(), et = bucket_p_w.end();
				it != et; ++it) {
			NodeTy *v = *it;
			bucket_p_w.erase(v);
			NodeTy *u = Eval(v);
			dom[v] = semi[u] < semi[v] ? u : parent[w] ;
		}

	}

	// step 4
	for (int i = 2; i <= n ; ++i) {
		NodeTy *w = vertex[i];
		if (dom[w] != vertex[semi[w]]) {
			dom[w] = dom[dom[w]];
		}
	}
	dom[M->get_init_bb()];

	LOG("Dominators:" << nl);
	for (int i = 1 ; i <= n; ++i) {
		NodeTy *v = vertex[i];
		NodeTy *w = dom[v];
		LOG("index" << setw(3) << i << " dom(" << (long)v <<") =" << (long)w << nl);
	}

	return true;
}

// pass usage
PassUsage& DominatorPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<CFGMetaPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char DominatorPass::ID = 0;

// register pass
static PassRegistry<DominatorPass> X("DominatorPass");

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

