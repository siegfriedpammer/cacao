/* src/vm/jit/compiler2/DominatorPass.hpp - DominatorPass

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

#ifndef _JIT_COMPILER2_DOMINATORPASS
#define _JIT_COMPILER2_DOMINATORPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/JITData.hpp"

#include "vm/jit/compiler2/alloc/set.hpp"
#include "vm/jit/compiler2/alloc/map.hpp"
#include "vm/jit/compiler2/alloc/vector.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class DominatorTree {
public:
	typedef BeginInst NodeTy;
	typedef alloc::map<NodeTy*, NodeTy*>::type EdgeMapTy;
protected:
	EdgeMapTy dom;
public:
	/**
	 * True if a dominates b.
	 */
	bool dominates(NodeTy *a, NodeTy *b) const;
	/**
	 * True if b is dominated by b.
	 */
	inline bool is_dominated_by(NodeTy *b, NodeTy *a) const {
		return dominates(a,b);
	}

	/**
	 * Get the immediate dominator.
	 *
	 * @return the immediate dominator or NULL if it is the starting node of
	 *         the dominator tree (and if a is not in the tree).
	 */
	inline NodeTy* get_idominator(NodeTy *a) const {
		EdgeMapTy::const_iterator i = dom.find(a);
		if (i == dom.end())
			return NULL;
		return i->second;
	}

	/**
	 * Find the nearest common dominator.
	 */
	NodeTy* find_nearest_common_dom(NodeTy *a, NodeTy *b) const;

	/**
	 * Depth of a tree node.
	 *
	 * The depth is defined as the length of the path from the node to the
	 * root. The depth of the root is 0.
	 */
	int depth(NodeTy *node) const {
		int d = 0;
		while ( (node = get_idominator(node)) != NULL) {
			++d;
		}
		return d;
	}
};

/**
 * Calculate the Dominator Tree.
 *
 * This Pass implements the algorithm proposed by Lengauer and Tarjan.
 * The variable and function are named accoring to the paper. Currently the
 * 'simple' version is implemented. The 'sophisticated' version is left
 * for future work.
 *
 * A Fast Algorithm for Finding Dominators in a Flowgraph, by Lengauer
 * and Tarjan, 1979 @cite Lengauer1979.
 */
class DominatorPass : public Pass , public DominatorTree {
private:
	typedef BeginInst NodeTy;
	typedef alloc::set<NodeTy *>::type NodeListTy;
	typedef alloc::map<NodeTy *,NodeListTy>::type NodeListMapTy;
	typedef alloc::vector<NodeTy *>::type NodeMapTy;
	typedef alloc::map<NodeTy *,int>::type IndexMapTy;
	typedef alloc::map<NodeTy *,NodeTy *>::type EdgeMapTy;

	EdgeMapTy parent;
	NodeListMapTy pred;
	IndexMapTy semi;
	NodeMapTy vertex;
	NodeListMapTy bucket;
	int n;

	EdgeMapTy ancestor;
	EdgeMapTy label;

	NodeListTy& succ(NodeTy *v, NodeListTy &list);
	void DFS(NodeTy * v);

	void Link(NodeTy *v, NodeTy *w);
	NodeTy* Eval(NodeTy *v);
	void Compress(NodeTy *v);
public:
	static char ID;
	DominatorPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PU) const;
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_DOMINATORPASS */


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
