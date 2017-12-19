/* src/vm/jit/compiler2/DominatorPassBase.hpp - DominatorPassBase

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

#ifndef _JIT_COMPILER2_DOMINATORPASSBASE
#define _JIT_COMPILER2_DOMINATORPASSBASE

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"

#include "vm/jit/compiler2/alloc/set.hpp"
#include "vm/jit/compiler2/alloc/map.hpp"
#include "vm/jit/compiler2/alloc/vector.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

template<typename _T>
class DominatorTreeBase {
public:
	typedef _T NodeTy;
	typedef typename alloc::map<NodeTy*, NodeTy*>::type EdgeMapTy;
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
		typename EdgeMapTy::const_iterator i = dom.find(a);
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
template<typename _T, typename SuccIter>
class DominatorPassBase : public Pass, public memory::ManagerMixin<DominatorPassBase<_T, SuccIter> >, public DominatorTreeBase<_T> {
private:
	typedef _T NodeTy;
	typedef typename alloc::set<NodeTy *>::type NodeListTy;
	typedef typename alloc::map<NodeTy *,NodeListTy>::type NodeListMapTy;
	typedef typename alloc::vector<NodeTy *>::type NodeMapTy;
	typedef typename alloc::map<NodeTy *,int>::type IndexMapTy;
	typedef typename alloc::map<NodeTy *,NodeTy *>::type EdgeMapTy;

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

	NodeTy* get_init_node(JITData &JD);
	std::size_t get_nodes_size(JITData &JD);
	
	auto node_begin(JITData &JD);
	auto node_end(JITData &JD);

	SuccIter succ_begin(NodeTy* v);
	SuccIter succ_end(NodeTy* v);

	// Needed since BeginInst.succ_begin() iterator returns BeginInstRef& and we need to convert it
	template<typename Ref>
	NodeTy* get(Ref v);
	//NodeTy* get(Ref v) {
	//	return v.get();
	//}	
	
	/**
	 * Returns the children in the DOMINATOR TREE for a given node
	 * Do NOT confuse this with successor in the CFG
	 */
	NodeListTy get_children(NodeTy* node) const {
		NodeListTy children;
		for (const auto& pair : DominatorTreeBase<_T>::dom) {
			if (pair.second && *node == *pair.second) {
				children.insert(pair.first);
			}
		}
		return children;
	}

public:
	typedef typename alloc::vector<NodeTy*>::type DominanceFrontierTy;

	DominatorPassBase() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PU) const;
};

#define DEBUG_NAME "compiler2/DominatorPass"


///////////////////////////////////////////////////////////////////////////////
// DominatorTree
///////////////////////////////////////////////////////////////////////////////
template<typename T>
bool DominatorTreeBase<T>::dominates(NodeTy *a, NodeTy *b) const {
	for ( ; b != NULL ; b = get_idominator(b) ) {
		if (a == b)
		  return true;
	}
	return false;
}

template<typename T>
typename DominatorTreeBase<T>::NodeTy* DominatorTreeBase<T>::find_nearest_common_dom(NodeTy *a, NodeTy *b) const {
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
	typename alloc::set<NodeTy*>::type dom_a;
	while ( (a = get_idominator(a)) ) {
		dom_a.insert(a);
	}

	// search nearest common dominator
	for(auto e = dom_a.end(); b != NULL ; b = get_idominator(b) ) {
		if (dom_a.find(b) != e) {
			return b;
		}
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// DominatorPass
///////////////////////////////////////////////////////////////////////////////
template<typename T, typename I>
typename DominatorPassBase<T,I>::NodeListTy& DominatorPassBase<T,I>::succ(NodeTy *v, NodeListTy& list) {
	for (auto i = succ_begin(v), e = succ_end(v); i != e; ++i) {
		list.insert(get(*i));
	}
	return list;
}

template<typename T, typename I>
void DominatorPassBase<T,I>::DFS(NodeTy * v) {
	assert(v);
	semi[v] = ++n;
	vertex[n] = v;
	// TODO this can be done better
	NodeListTy succ_v;
	succ_v = succ(v, succ_v);
	LOG("number of succ for " << (long)v << " (" << n << ") " << " = " << succ_v.size() << nl);
	for(auto i = succ_v.begin() , e = succ_v.end();
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
template<typename T, typename I>
void DominatorPassBase<T,I>::Link(NodeTy *v, NodeTy *w) {
	ancestor[w] = v;
}

template<typename T, typename I>
typename DominatorPassBase<T,I>::NodeTy* DominatorPassBase<T,I>::Eval(NodeTy *v) {
	if (ancestor[v] == 0) {
		return v;
	} else {
		return label[v];
	}
}

template<typename T, typename I>
void DominatorPassBase<T,I>::Compress(NodeTy *v) {
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

template<typename T, typename I>
bool DominatorPassBase<T,I>::run(JITData &JD) {
	// initialize
	n = 0;
	vertex.resize(get_nodes_size(JD) + 1); // +1 because we start at 1 not at 0
	for(auto i = node_begin(JD), e = node_end(JD); i != e; ++i) {
		NodeTy *v = *i;
		// pred[v] = NodeListTy(); // maps are auto initialized at first access
		semi[v] = 0;
		// init label and ancestor array
		ancestor[v] = 0;
		label[v] = v;
	}
	DFS(get_init_node(JD));

	LOG("DFS:" << nl);
	for (int i = 1 ; i <= n; ++i) {
		LOG("index" << setw(3) << i << " " << (long)vertex[i] << nl);
	}

	// from n to 2
	for (int i = n; i >= 2; --i) {
		NodeTy *w = vertex[i];
		// step 2
		NodeListTy &pred_w = pred[w];
		for (auto it = pred_w.begin(), et = pred_w.end();
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
		for (auto it = bucket_p_w.begin(), et = bucket_p_w.end();
				it != et; ) {
			auto ittmp = it;
			it++;
			NodeTy *v = *ittmp;
			bucket_p_w.erase(ittmp);
			NodeTy *u = Eval(v);
			DominatorTreeBase<T>::dom[v] = semi[u] < semi[v] ? u : parent[w] ;
		}

	}

	// step 4
	for (int i = 2; i <= n ; ++i) {
		NodeTy *w = vertex[i];
		if (DominatorTreeBase<T>::dom[w] != vertex[semi[w]]) {
			DominatorTreeBase<T>::dom[w] = DominatorTreeBase<T>::dom[DominatorTreeBase<T>::dom[w]];
		}
	}
	DominatorTreeBase<T>::dom[get_init_node(JD)];

	LOG("Dominators:" << nl);
	#if defined(ENABLE_LOGGING)
	if (DEBUG_COND_N(0)) {
		for (int i = 1 ; i <= n; ++i) {
			NodeTy *v = vertex[i];
			NodeTy *w = DominatorTreeBase<T>::dom[v];
			LOG("index" << setw(3) << i << " dom(" << (long)v <<") =" << (long)w << nl);
		}
	}
	#endif

	return true;
}

#undef DEBUG_NAME


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_DOMINATORPASSBASE */


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
