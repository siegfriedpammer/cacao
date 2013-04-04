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

#include <set>
#include <map>
#include <vector>

namespace cacao {
namespace jit {
namespace compiler2 {


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
class DominatorPass : public Pass {
private:
	typedef BeginInst NodeTy;
	typedef std::set<NodeTy *> NodeListTy;
	typedef std::map<NodeTy *,NodeListTy> NodeListMapTy;
	typedef std::vector<NodeTy *> NodeMapTy;
	typedef std::map<NodeTy *,int> IndexMapTy;
	typedef std::map<NodeTy *,NodeTy *> EdgeMapTy;

	EdgeMapTy parent;
	NodeListMapTy pred;
	IndexMapTy semi;
	NodeMapTy vertex;
	NodeListMapTy bucket;
	EdgeMapTy dom;
	int n;

	EdgeMapTy ancestor;
	EdgeMapTy label;

	NodeListTy& succ(NodeTy *v, NodeListTy &list);
	void DFS(NodeTy * v);

	void Link(NodeTy *v, NodeTy *w);
	NodeTy* Eval(NodeTy *v);
	void Compress(NodeTy *v);
public:
	DominatorPass(PassManager *PM) : Pass(PM) {}
	bool run(JITData &JD);
	const char* name() { return "DominatorPass"; };
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
