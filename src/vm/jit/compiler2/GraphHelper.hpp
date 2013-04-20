/* src/vm/jit/compiler2/GraphHelper.hpp - GraphHelper

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

#ifndef _JIT_COMPILER2_GRAPHHELPER
#define _JIT_COMPILER2_GRAPHHELPER


#include <set>
#include <map>
#include <vector>

#include "vm/jit/compiler2/Instructions.hpp"
#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/GraphHelper"

namespace cacao {
namespace jit {
namespace compiler2 {


/**
 * This helper class creates traversal for a graph.
 *
 * The only information required from the graph is the successor relation succ().
 */
template <class _NodeTy>
class DFSTraversal {
private:
	typedef typename std::vector<const _NodeTy *> NodeMapTy;
	typedef typename std::map<const _NodeTy *,int> IndexMapTy;
	typedef typename std::vector<int> ParentMapTy;

	typedef typename std::set<const _NodeTy *> NodeListTy;
	typedef typename std::map<const _NodeTy *,NodeListTy> NodeListMapTy;
	typedef typename std::map<const _NodeTy *,const _NodeTy *> EdgeMapTy;


	typedef typename std::set<const _NodeTy *>::const_iterator const_node_list_iterator;

	NodeMapTy vertex;
	IndexMapTy index;
	ParentMapTy parent;

	int n;
	int _size;

	#if 0
	EdgeMapTy parent;
	NodeListMapTy pred;
	NodeListMapTy bucket;
	IndexMapTy semi;
	NodeMapTy vertex;

	EdgeMapTy ancestor;
	EdgeMapTy label;
	#endif

	NodeListTy& succ(const _NodeTy *v, NodeListTy &list);
	int num_nodes(const _NodeTy *v) const;

	int dfs(const _NodeTy * v);

public:
	explicit DFSTraversal(const _NodeTy *entry) {
		n = -1;
		_size = num_nodes(entry);
		LOG("num nodes" << _size << nl);
		vertex.resize(_size, NULL);
		// index.resize(_size, -1); auto initialzied
		parent.clear();
		parent.resize(_size, -1);
		LOG("vertex size " << vertex.size() << nl);

		#if 0
		for(int i = 0, e = size; i != e ; ++i) {
			// pred[v] = NodeListTy(); // maps are auto initialized at first access
			semi[v] = -1;
			// init label and ancestor array
			ancestor[v] = -1;
			//label[v] = v;
		}
		#endif
		dfs(entry);
#ifdef ENABLE_LOGGING
		LOG("index"<<nl);
		for(int i = 0, e = _size; i != e ; ++i) {
			LOG(" node " << (long) vertex[i] << " parent " << parent[i] << nl);
		}

		LOG("iterator"<<nl);
		for(iterator i = begin(), e = end(); i != e; ++i) {
			LOG(" node " << (long) *i << " parent " << i.get_parent().get_index() << nl);
		}
#endif
	}

	int size() const {
		return _size;
	}

	const _NodeTy* operator[](unsigned i) {
		return vertex[i];
	}

	int operator[](const _NodeTy *v) {
		return index[v];
	}

	class iterator {
		private:
			int index;
			DFSTraversal *parent;
		public:
			explicit iterator (DFSTraversal *parent) : index(0), parent(parent)  {}
			explicit iterator (DFSTraversal *parent, int index) : index(index), parent(parent){}

			const _NodeTy* operator*() const {
				if(index >= 0 && index < parent->size())
					return parent->vertex[index];
				return NULL;
			};
			iterator& operator++() {
				++index;
				if (parent->size() <= index)
					index = -1;
				return *this;
			}

			/*
			 * If other.parent is not equal to this->parent the
			 * behaviour is undefined. Not checking parent for
			 * performance reasons.
			 */
			bool operator==(const iterator &other) const {
				assert(parent == other.parent);
				return index == other.index;
			}
			bool operator!=(const iterator &other) const {
				assert(parent == other.parent);
				return index != other.index;
			}
			bool operator<(const iterator &other) const {
				assert(parent == other.parent);
				return index < other.index;
			}
			bool operator>(const iterator &other) const {
				assert(parent == other.parent);
				return index > other.index;
			}
			bool operator<=(const iterator &other) const {
				assert(parent == other.parent);
				return index <= other.index;
			}
			bool operator>=(const iterator &other) const {
				assert(parent == other.parent);
				return index >= other.index;
			}

			int get_index() const {
				return index;
			}

			iterator get_parent() const {
				return iterator(parent,parent->parent[index]);
			}
	};

	iterator begin() {
		return iterator(this);
	}

	iterator end() {
		return iterator(this,-1);
	}


	friend class DFSTraversal::iterator;
};

template <class _NodeTy>
int DFSTraversal<_NodeTy>::dfs(const _NodeTy * v)
{
	assert(v);
	int my_n = ++n;
	LOG("my_n for " << (long)v << " (" << my_n << ") " << nl);
	index[v] = my_n;
	assert(vertex.size() > my_n);
	vertex[my_n] = v;
	// TODO this can be done better
	NodeListTy succ_v;
	succ_v = succ(v, succ_v);
	LOG("number of succ for " << (long)v << " (" << my_n << ") " << " = " << succ_v.size() << nl);
	for(const_node_list_iterator i = succ_v.begin() , e = succ_v.end();
			i != e; ++i) {
		const _NodeTy *w = *i;
		if (index.find(w) == index.end()) {
			int w_n = dfs(w);
			parent[w_n] = my_n;
		}
		//pred[w].insert(v);
	}
	return my_n;
}

// specialization for BeginInst

template <>
DFSTraversal<BeginInst>::NodeListTy& DFSTraversal<BeginInst>::succ(const BeginInst *v, NodeListTy& list) {
	EndInst *ve = v->get_EndInst();
	assert(ve);
	list.insert(ve->succ_begin(),ve->succ_end());
	return list;
}

template <>
int DFSTraversal<BeginInst>::num_nodes(const BeginInst *v) const {
	assert(v->get_method());
	return v->get_method()->bb_size();
}

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#undef DEBUG_NAME

#endif /* _JIT_COMPILER2_GRAPHHELPER */


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
