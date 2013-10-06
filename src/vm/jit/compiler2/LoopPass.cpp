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

#include "toolbox/UnionFind.hpp"

#include <list>
#include <set>

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/LoopPass"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

template<class NodeType>
class LoopComparator {
private:
	DFSTraversal<NodeType> &dfs;
public:
	LoopComparator(DFSTraversal<NodeType> &dfs) : dfs(dfs) {}
	bool operator() (const Loop *a, const Loop* b) {
		NodeType *a_i = a->get_header();
		NodeType *b_i = b->get_header();
		if (dfs[a_i] == dfs[b_i]) {
			a_i = a->get_exit();
			b_i = b->get_exit();
		}
		return dfs[a_i] < dfs[b_i];
	}
};

} // end namespace anonymous


template<>
bool LoopPass::run(JITData &JD) {
	Method *M = JD.get_Method();
	DFSTraversal<NodeType> dfs(M->get_init_bb());
	int size = dfs.size();

	std::vector<std::set<int> > backedges(size);
	std::vector<std::set<int> > forwardedges(size);
	std::vector<std::set<int> > crossedges(size);
	std::vector<std::set<int> > treeedges(size);
	std::vector<int> highpt(size,-1);

	int end = -1;

	UnionFindImpl<int> unionfind(end);

	// store the loop for each bb
	std::vector<Loop*> index_to_loop(size);


	// initialization
	for (DFSTraversal<NodeType>::iterator i = dfs.begin(), e = dfs.end();
			i != e ; ++i) {
		int v = i.get_index();

		unionfind.make_set(v);
		// get successors
		DFSTraversal<NodeType>::iterator_list successors;
		i.get_successors(successors);

		for (DFSTraversal<NodeType>::iterator_list::iterator ii = successors.begin(),
				ee = successors.end(); ii != ee ; ++ii) {
			int w = (*ii).get_index();
			// edge v -> w

			if ((*ii).get_parent() != i) {
				if (dfs.is_ancestor(v,w)) {
					// forward edge
					forwardedges[w].insert(v);
				} else if (dfs.is_decendant(v,w)) {
					// backward edge
					backedges[w].insert(v);
				} else {
					// cross edge
					crossedges[w].insert(v);
				}
			} else {
				// tree edge
				treeedges[w].insert(v);
			}
		}
	}


	#if 0
	for (int w = 0; w < size ; ++w ) {
		std::set<int> set = treeedges[w];
		// print treeedge
		for(std::set<int>::iterator i = set.begin(), e = set.end(); i != e ; ++i) {
			int v = *i;
			//LOG2("treeedge:    " << setw(3) << v << " -> " << setw(3) << w << nl);
		}
		// print backedge
		set = backedges[w];
		for(std::set<int>::iterator i = set.begin(), e = set.end(); i != e ; ++i) {
			int v = *i;
			//LOG2("backedge:    " << setw(3) << v << " -> " << setw(3) << w << nl);
		}
		// print forwardedge
		set = forwardedges[w];
		for(std::set<int>::iterator i = set.begin(), e = set.end(); i != e ; ++i) {
			int v = *i;
			//LOG2("forwardedge: " << setw(3) << v << " -> " << setw(3) << w << nl);
		}
		// print crossedge
		set = crossedges[w];
		for(std::set<int>::iterator i = set.begin(), e = set.end(); i != e ; ++i) {
			int v = *i;
			//LOG2("crossedge:   " << setw(3) << v << " -> " << setw(3) << w << nl);
		}
	}
	#endif

	std::set<int> P;
	std::set<int> Q;
	for (DFSTraversal<NodeType>::reverse_iterator i = dfs.rbegin(), e = dfs.rend();
			i != e ; --i) {
		int w = i.get_index();
		//LOG2("w=" << w << nl);

		P.clear();
		Q.clear();

		// get incoming backedges
		std::set<int> &backedges_w = backedges[w];
		for(std::set<int>::iterator i = backedges_w.begin(), e = backedges_w.end();
				i != e; ++i) {
			int v = *i;
			int v_r = unionfind.find(v);
			assert(v_r != end && "Element not found!");
			P.insert(v_r);
			Q.insert(v_r);

			// create a new loop
			Loop *loop = add_loop(dfs[w],dfs[v]);

			// set loop
			if(!index_to_loop[v]) {
				index_to_loop[v] = loop;
			}
			if (index_to_loop[w]) {
				Loop *inner_loop = index_to_loop[w];
				if(inner_loop != loop) {
					loop->add_inner_loop(inner_loop);
				}
			}

			//LOG2("Q=P= ");
			//DEBUG2(print_container(dbg(), Q.begin(),Q.end()));
			//LOG2(nl);

			// Q contains the sources of the backedges to w.
			// It must be order from smallest to largest preorder source to
			// guarantee strict inner to outer ordering.
			while (!Q.empty()) {
				int x;
				// simulate pop_front
				{ // new scope
					std::set<int>::iterator x_it = Q.begin();
					x = *x_it;
					Q.erase(x_it);
				}
				//LOG2("x=" << x << nl);

				// is loop header?
				if (backedges[x].size() != 0) {
					Loop *inner_loop = index_to_loop[x];
					assert(inner_loop);
					if(inner_loop != loop) {
						loop->add_inner_loop(inner_loop);
					}
				}

				std::set<int> incoming;
				// collect incoming edges
				{ // new scope
					std::set<int> ref1 = forwardedges[x];
					incoming.insert(ref1.begin(), ref1.end());
					std::set<int> ref2 = treeedges[x];
					incoming.insert(ref2.begin(), ref2.end());
					std::set<int> ref3 = crossedges[x];
					incoming.insert(ref3.begin(), ref3.end());
				}
				for(std::set<int>::iterator i = incoming.begin(), e = incoming.end();
						i != e; ++i) {
					int y = *i;
					//LOG2("y=" << y << nl);
					int y_r = unionfind.find(y);
					assert(y_r != end);
					//LOG2("y_r=" << y_r << nl);

					//LOG("ND(w)=" << dfs.num_decendants(w) << nl);
					// test for reducibility
					// TODO <= vs <
					// if ( (w > y_r) || ( (w + dfs.num_decendants(w)) <= y_r) ) {
					if ( (w > y_r) || ( (w + dfs.num_decendants(w)) < y_r) ) {
						assert(0 && "Graph is irreduciable. Can not yet deal with it.");
					}

					// add y_r if appropriate
					if ( P.find(y_r) == P.end() and y_r != w) {
						P.insert(y_r);
						Q.insert(y_r);
					}

					// set highpt
					if (highpt[y_r] == -1) {
						highpt[y_r] = w;
					}

					// set loop
					if (!index_to_loop[y_r]) {
						index_to_loop[y_r] = loop;
					}
				}
			}
		}

		// now P = P(w) in the current graph
		for(std::set<int>::iterator i = P.begin(), e = P.end();
				i != e; ++i) {
			int x = *i;
			unionfind.set_union(x,w);
		}
		// duplicate edges?

	}
	for (int w = 0; w < size ; ++w ) {
		//LOG2("highpt[" << w << "]=" << highpt[w] << nl);
	}
	for (int w = 0; w < size ; ++w ) {
		Loop *loop = index_to_loop[w];
		if (!loop) {
			//LOG2("loop(" << dfs[w] << ") not in a loop" << nl);
			continue;
		}
		loop_map[dfs[w]] = loop;
		//LOG2("loop(header = " << dfs[w] << ") = " << loop << nl);
	}

	// sort from outermost to innermost loop
	LoopComparator<NodeType> compare_loop(dfs);
	std::sort(loops.begin(), loops.end(), compare_loop);

	// add NodeType to loops
	for (LoopListTy::reverse_iterator i = loops.rbegin(), e = loops.rend();
			i != e; ++i) {
		Loop *loop = *i;
		//LOG("Loop: " << loop << nl);
		Loop *parent = loop->get_parent();
		if (parent) {
			//LOG("parent: " << parent << nl);
		} else {
			add_top_loop(loop);
			//LOG("parent: " << "toplevel" << nl);
		}
		// add to loop headers
		loop_header_map[loop->get_header()].insert(loop);
	}
	return true;
}

// the address of this variable is used to identify the pass
template<>
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

