/* src/vm/jit/compiler2/LoopPass.hpp - LoopPass

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

#ifndef _JIT_COMPILER2_LOOPPASS
#define _JIT_COMPILER2_LOOPPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/JITData.hpp"

#include <set>
#include <map>
#include <vector>

namespace cacao {
namespace jit {
namespace compiler2 {

class BeginInst;
class LoopSimplificationPass;

class Loop {
public:
	typedef std::set<Loop*> LoopListTy;
	typedef LoopListTy::iterator loop_iterator;
private:
	BeginInst *header;
	BeginInst *exit;
	Loop      *parent;
	LoopListTy inner_loops;
public:
	Loop(BeginInst *header, BeginInst *exit) : header(header), exit(exit), parent(NULL) {}
	BeginInst *get_header() const {
		return header;
	}
	BeginInst *get_exit() const {
		return exit;
	}
	Loop *get_parent() const {
		return parent;
	}
	loop_iterator loop_begin() {
		return inner_loops.begin();
	}
	loop_iterator loop_end() {
		return inner_loops.end();
	}
	void add_inner_loop(Loop* inner_loop) {
		if(!inner_loop->parent) {
			inner_loop->parent = this;
			inner_loops.insert(inner_loop);
		}
	}
	void set_parent(Loop* outer_loop) {
		if (outer_loop) {
			outer_loop->add_inner_loop(this);
		}
	}
	friend class LoopSimplificationPass;
};

OStream& operator<<(OStream &OS, const Loop &L);
OStream& operator<<(OStream &OS, const Loop *L);

class LoopTree {
public:
	// Note vector needed for std::sort!
	typedef std::vector<Loop*> LoopListTy;
	typedef Loop::LoopListTy::iterator loop_iterator;
	typedef Loop::LoopListTy::const_iterator const_loop_iterator;
	typedef std::pair<const_loop_iterator, const_loop_iterator> ConstLoopIteratorPair;
protected:
	bool reducible;
	LoopListTy loops;
	Loop::LoopListTy top_loops;
	std::map<BeginInst*,Loop*> loop_map;
	std::map<BeginInst*,Loop::LoopListTy> loop_header_map;
public:
	LoopTree() : reducible(true) {}

	bool is_reducible() const {
		return reducible;
	}
	Loop *add_loop(BeginInst* header, BeginInst *exit) {
		Loop *loop =new Loop(header,exit);
		loops.push_back(loop);
		return loop;
	}
	void add_top_loop(Loop* loop) {
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
	Loop* get_Loop(BeginInst *BI) const {
		std::map<BeginInst*,Loop*>::const_iterator it = loop_map.find(BI);
		if (it == loop_map.end()) {
			return NULL;
		}
		return it->second;
	}
	bool is_loop_header(BeginInst *BI) const {
		std::map<BeginInst*,Loop::LoopListTy>::const_iterator it = loop_header_map.find(BI);
		if (it == loop_header_map.end()) {
			return false;
		}
		return true;
	}
	ConstLoopIteratorPair get_Loops_from_header(BeginInst *BI) const {
		std::map<BeginInst*,Loop::LoopListTy>::const_iterator it = loop_header_map.find(BI);
		if (it == loop_header_map.end()) {
			// TODO there must be a better approach...
			static Loop::LoopListTy empty;
			return std::make_pair(empty.begin(),empty.end());
		}
		return std::make_pair(it->second.begin(),it->second.end());
	}
	/**
	 * Test if a loop is a strictly inner loop of another loop.
	 *
	 * Note that a loop is not an inner loop of itself!
	 */
	bool is_inner_loop(Loop *inner, Loop *outer) const {
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
	int loop_nest(Loop *loop) const {
		if(!loop)
			return -1;
		unsigned nest = 0;
		while((loop = loop->get_parent())) {
			++nest;
		}
		return nest;
	}
	virtual ~LoopTree() {
		for (LoopListTy::iterator i = loops.begin(), e = loops.end();
				i != e; ++i) {
			delete *i;
		}
	}

	friend class LoopSimplificationPass;
};

/**
 * Calculate the Loop Tree.
 *
 * The algorithm used here is based on the method proposed in
 * "Testing Flow Graph Reducibility" by Tarjan @cite Tarjan1974
 * with the modifications "SSA-Based Reduction of Operator Strengh" by
 * Vick @cite VickMScThesis. See also Click's Phd Thesis, Chapter 6
 * @cite ClickPHD.
 */
class LoopPass : public Pass , public LoopTree {
private:
	typedef BeginInst NodeTy;
	typedef std::set<const NodeTy *> NodeListTy;
	typedef std::map<const NodeTy *,NodeListTy> NodeListMapTy;
	typedef std::vector<const NodeTy *> NodeMapTy;
	typedef std::map<const NodeTy *,int> IndexMapTy;
	typedef std::map<const NodeTy *,const NodeTy *> EdgeMapTy;

	#if 0
	EdgeMapTy parent;
	NodeListMapTy pred;
	#endif
	NodeListMapTy bucket;
	IndexMapTy semi;
	NodeMapTy vertex;
	int n;

	EdgeMapTy ancestor;
	EdgeMapTy label;

	NodeListTy& succ(const NodeTy *v, NodeListTy &list);
	void DFS(const NodeTy * v);

	void Link(const NodeTy *v, const NodeTy *w);
	const NodeTy* Eval(const NodeTy *v);
	void Compress(const NodeTy *v);
public:
	static char ID;
	LoopPass() : Pass() {}
	bool run(JITData &JD);
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LOOPPASS */


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
