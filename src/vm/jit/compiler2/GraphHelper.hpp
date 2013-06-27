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
 * The only information required from the graph is the successor relation successor()
 * and number of vertices  num_nodes.
 */
template <class _NodeTy>
class DFSTraversal {
private:
	typedef typename std::vector<_NodeTy *> NodeMapTy;
	typedef typename std::map<_NodeTy *,int> IndexMapTy;
	typedef typename std::vector<int> ParentMapTy;
	typedef typename std::vector<int> DecendantMapTy;

	typedef typename std::set<_NodeTy *> NodeListTy;
	typedef typename std::set<int> SuccessorListTy;
	typedef typename std::vector<SuccessorListTy> SuccessorListMapTy;

	NodeMapTy vertex;
	IndexMapTy index;
	ParentMapTy parent;
	SuccessorListMapTy succ;
	DecendantMapTy number_decendants;

	int n;
	int _size;

	NodeListTy& successor(_NodeTy *v, NodeListTy &list);
	int num_nodes(_NodeTy *v) const;

	int dfs(_NodeTy * v);

public:
	class iterator;
	typedef std::set<iterator> iterator_list;

	explicit DFSTraversal(_NodeTy *entry) {
		n = -1;
		_size = num_nodes(entry);
		LOG("num nodes" << _size << nl);
		vertex.resize(_size, NULL);
		// index.resize(_size, -1); auto initialzied
		parent.resize(_size, -1);
		succ.resize(_size);
		number_decendants.clear();
		number_decendants.resize(_size,0);
		LOG("vertex size " << vertex.size() << nl);

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

	_NodeTy* operator[](unsigned i) const {
		return vertex[i];
	}

	int operator[](_NodeTy *v) const {
		typename IndexMapTy::const_iterator i = index.find(v);
		if (i == index.end())
			return -1;
		return i->second;
	}

	int num_decendants(int v) const {
		assert(v < _size);
		return number_decendants[v];
	}

	#if 0
	bool is_ancestor(const _NodeTy *v, const _NodeTy *w) const {
		return is_ancestor(index[v],index[w]);
	}
	#endif
	inline bool is_ancestor(int v, int w) const {
		assert(w < _size);
		for(;w >= 0; w = parent[w]) {
			if (v == w)
				return true;

		}
		return false;
	}
	#if 0
	bool is_decendant(const _NodeTy *w, const _NodeTy *v) const {
		return is_ancestor(index[v],index[w]);
	}
	#endif
	bool is_decendant(int w, int v) const {
		return is_ancestor(v,w);
	}

	class iterator {
		private:
			int index;
			DFSTraversal *parent;
		public:
			explicit iterator (DFSTraversal *parent) : index(0), parent(parent)  {}
			explicit iterator (DFSTraversal *parent, int index) : index(index), parent(parent){}

			_NodeTy* operator*() const {
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
			iterator& operator--() {
				--index;
				if (index < 0)
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

			iterator_list& get_successors(iterator_list &list) const {
				for(SuccessorListTy::iterator i = parent->succ[index].begin(),
						e = parent->succ[index].end(); i != e ; ++i) {
					list.insert(iterator(parent,*i));
				}
				return list;
			}
	};

	typedef iterator reverse_iterator;

	iterator begin() {
		return iterator(this);
	}

	iterator end() {
		return iterator(this,-1);
	}

	reverse_iterator rbegin() {
		return reverse_iterator(this,size()-1);
	}

	reverse_iterator rend() {
		return reverse_iterator(this,-1);
	}

	friend class DFSTraversal::iterator;
};

template <class _NodeTy>
int DFSTraversal<_NodeTy>::dfs(_NodeTy * v)
{
	assert(v);
	int my_n = ++n;
	LOG("my_n for " << (long)v << " (" << my_n << ") " << nl);
	index[v] = my_n;
	assert(vertex.size() > my_n);
	vertex[my_n] = v;
	// TODO this can be done better
	NodeListTy succ_v;
	succ_v = successor(v, succ_v);
	LOG("number of succ for " << (long)v << " (" << my_n << ") " << " = " << succ_v.size() << nl);
	for(typename std::set<_NodeTy *>::const_iterator i = succ_v.begin() , e = succ_v.end();
			i != e; ++i) {
		_NodeTy *w = *i;
		SuccessorListTy &succ_list = succ[my_n];
		if (index.find(w) == index.end()) {
			int w_n = dfs(w);
			parent[w_n] = my_n;
			succ_list.insert(w_n);
		} else {
			succ_list.insert(index[w]);
		}
		//pred[w].insert(v);
	}
	number_decendants[my_n] = n - my_n;
	return my_n;
}

// specialization for BeginInst
template <>
DFSTraversal<BeginInst>::NodeListTy& DFSTraversal<BeginInst>::successor(BeginInst *v, NodeListTy& list);

template <>
int DFSTraversal<BeginInst>::num_nodes(BeginInst *v) const;

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
