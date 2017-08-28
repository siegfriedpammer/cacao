/* src/vm/jit/compiler2/LoopBase.hpp - LoopBase

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

#ifndef _JIT_COMPILER2_LOOPBASE
#define _JIT_COMPILER2_LOOPBASE

#include <cstddef> // for NULL
#include "vm/jit/compiler2/alloc/unordered_set.hpp"

#include "vm/jit/compiler2/alloc/set.hpp"
#include "vm/jit/compiler2/alloc/map.hpp"
#include "vm/jit/compiler2/alloc/vector.hpp"

namespace cacao {

// forward declaration
class OStream;

namespace jit {
namespace compiler2 {

template <class _T>
class LoopBase {
public:
	typedef _T NodeType;
	typedef typename alloc::unordered_set<LoopBase*>::type LoopSetTy;
	typedef typename LoopSetTy::iterator loop_iterator;
	typedef typename alloc::vector<NodeType*>::type NodeTypeListTy;
	typedef typename NodeTypeListTy::iterator child_iterator;
private:
	NodeType *header;
	NodeType *exit;
	LoopBase      *parent;
	LoopSetTy inner_loops;
	NodeTypeListTy children;
	unsigned register_pressure; //< Maximum register pressure inside this loop (only used by MachineLoops)
public:
	LoopBase(NodeType *header, NodeType *exit) : header(header), exit(exit), parent(NULL) {}
	NodeType *get_header() const {
		return header;
	}
	NodeType *get_exit() const {
		return exit;
	}
	LoopBase *get_parent() const {
		return parent;
	}
	loop_iterator loop_begin() {
		return inner_loops.begin();
	}
	loop_iterator loop_end() {
		return inner_loops.end();
	}
	void add_inner_loop(LoopBase* inner_loop) {
		if(!inner_loop->parent) {
			inner_loop->parent = this;
			inner_loops.insert(inner_loop);
		}
	}
	void set_parent(LoopBase* outer_loop) {
		if (outer_loop) {
			outer_loop->add_inner_loop(this);
		}
	}
	void add_child(NodeType* child) {
		children.push_back(child);
	}
	child_iterator child_begin() { return children.begin(); }
	child_iterator child_end() { return children.end(); }
	const unsigned get_register_pressure() const {
		return register_pressure;
	}
	void set_register_pressure(unsigned pressure) {
		register_pressure = pressure;
	}
};

template <class _T>
class LoopTreeBase {
public:
	typedef _T NodeType;
	typedef LoopBase<NodeType> LoopType;
	// Note vector needed for std::sort!
	typedef typename LoopType::LoopSetTy LoopSetTy;
	typedef typename LoopSetTy::iterator loop_iterator;
	typedef typename LoopSetTy::const_iterator const_loop_iterator;
	typedef std::pair<const_loop_iterator, const_loop_iterator> ConstLoopIteratorPair;

	typedef typename alloc::vector<LoopType*>::type LoopListTy;
	typedef typename LoopListTy::iterator iterator;
	typedef typename LoopListTy::reverse_iterator reverse_iterator;
protected:
	bool reducible;
	LoopListTy loops;
	LoopSetTy top_loops;
	typename alloc::map<NodeType*,LoopType*>::type loop_map;
	typename alloc::map<NodeType*,LoopSetTy>::type loop_header_map;

	void set_loop(NodeType* node, LoopType* loop) {
		loop_map[node] = loop;
	}
	void insert_loop_header(NodeType* node, LoopType* loop) {
		loop_header_map[node].insert(loop);
	}
	iterator begin() {
		return loops.begin();
	}
	iterator end() {
		return loops.end();
	}
	reverse_iterator rbegin() {
		return loops.rbegin();
	}
	reverse_iterator rend() {
		return loops.rend();
	}
public:
	LoopTreeBase() : reducible(true) {}

	bool is_reducible() const {
		return reducible;
	}
	LoopType *add_loop(NodeType* header, NodeType *exit) {
		LoopType *loop =new LoopType(header,exit);
		loops.push_back(loop);
		return loop;
	}
	void add_top_loop(LoopType* loop) {
		if(loop) {
			top_loops.insert(loop);
		}
	}
	loop_iterator loop_begin() {
		return top_loops.begin();
	}
	loop_iterator loop_end() {
		return top_loops.end();
	}
	/**
	 * Get the inner most loop which contains BI or NULL if not contained in any loop
	 */
	LoopType* get_Loop(NodeType *BI) const {
		typename alloc::map<NodeType*,LoopType*>::type::const_iterator it = loop_map.find(BI);
		if (it == loop_map.end()) {
			return NULL;
		}
		return it->second;
	}
	bool is_loop_header(NodeType *BI) const {
		typename alloc::map<NodeType*,LoopSetTy>::type::const_iterator it = loop_header_map.find(BI);
		if (it == loop_header_map.end()) {
			return false;
		}
		return true;
	}
	ConstLoopIteratorPair get_Loops_from_header(NodeType *BI) const {
		typename alloc::map<NodeType*,LoopSetTy>::type::const_iterator it = loop_header_map.find(BI);
		if (it == loop_header_map.end()) {
			// TODO there must be a better approach...
			static LoopSetTy empty;
			return std::make_pair(empty.begin(),empty.end());
		}
		return std::make_pair(it->second.begin(),it->second.end());
	}
	bool is_backedge(NodeType *src, NodeType *header) const {
		for (ConstLoopIteratorPair it = get_Loops_from_header(header);
				it.first != it.second; ++it.first) {
			LoopType *loop = *it.first;
			if(loop->get_exit() == src) {
				return true;
			}
		}

		return false;
	}
	/**
	 * Test if a loop is a strictly inner loop of another loop.
	 *
	 * Note that a loop is not an inner loop of itself!
	 */
	bool is_inner_loop(LoopType *inner, LoopType *outer) const {
		if (!inner || inner == outer) {
			return false;
		}
		if (!outer) {
			// every loop is a inner loop of no loop
			return true;
		}
		while((inner = inner->get_parent())) {
			if (inner == outer) {
				return true;
			}
		}
		return false;
	}
	/**
	 * TODO: cache?
	 */
	int loop_nest(LoopType *loop) const {
		if(!loop)
			return -1;
		unsigned nest = 0;
		while((loop = loop->get_parent())) {
			++nest;
		}
		return nest;
	}
	virtual ~LoopTreeBase() {
		for (typename LoopListTy::iterator i = loops.begin(), e = loops.end();
				i != e; ++i) {
			delete *i;
		}
	}

};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LOOPBASE */


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
