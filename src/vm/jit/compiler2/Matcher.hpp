/* src/vm/jit/compiler2/Matcher.hpp - Matcher class

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
#ifndef _JIT_COMPILER2_MATCHER
#define _JIT_COMPILER2_MATCHER

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "Instruction.hpp"
#include "vm/jit/compiler2/GlobalSchedule.hpp"
#include "vm/jit/compiler2/MatcherDefs.hpp"
#include "vm/jit/compiler2/alloc/list.hpp"
#include "vm/jit/compiler2/alloc/set.hpp"
#include "vm/jit/compiler2/alloc/unordered_map.hpp"
#include "Target.hpp"

#include "future/memory.hpp"

// #include "vm/jit/compiler2/MatcherDefs.hpp"

namespace cacao {
namespace jit {
namespace compiler2{

// typedefs and macros needed by generated matcher
typedef Instruction* NODEPTR_TYPE;
typedef struct burm_state STATE_TYPE;

// INCLUDE GENERATED DEFINES
#define GENDEFS
#include "Grammar.inc"

class Matcher {

public:

	// typedefs to avoid naming dependency to generated matcher
	typedef alloc::unordered_map<Instruction*, shared_ptr<STATE_TYPE> >::type InstToStateLabelTy;
	typedef alloc::list<Instruction*>::type InstListTy;
	typedef alloc::set<Instruction*>::type InstSetTy;
	typedef alloc::unordered_map<int, shared_ptr<Instruction> >::type ProxyMapByInstTy;
	typedef alloc::set<Instruction::InstID>::type ExcludeSetTy;
	typedef alloc::unordered_map<Instruction*, shared_ptr<InstSetTy> >::type DependencyMapTy;
	typedef alloc::unordered_map<Instruction*, shared_ptr<ProxyMapByInstTy> >::type ProxyMapTy;

	Matcher(GlobalSchedule* sched, BeginInst* BI, LoweringVisitor *LV) : 
		sched(sched), basicBlock(BI), LV(LV) { }

	void run();

private: 
	GlobalSchedule *sched;
	BeginInst* basicBlock;
	LoweringVisitor *LV;
	DependencyMapTy roots;
	DependencyMapTy revdeps;

	InstToStateLabelTy state_labels;
	ProxyMapTy instrProxies;
	InstSetTy phiInst;
	InstSetTy inst_in_bb;

	static const ExcludeSetTy excluded_nodes;

// INCLUDE GENERATED MATCHER ALGORITHM 
#define GENMETHODDEFS
#include "Grammar.inc"

	// matching
	void findRoots();
	bool checkIsNodeExcluded(Instruction* inst);
	Instruction* getOperand(Instruction* op, unsigned pos);
	Instruction* createProxy(Instruction* op, unsigned pos, Instruction* operand, bool dependency);

	// debug output
	void dumpCover(Instruction* p, int goalnt, int indent);
	void printDependencies(Instruction* inst);

	// scheduling and lowering
	void scheduleTrees();
	void lowerTree(Instruction* p, int goalnt);
	void lowerRule(Instruction* inst, RuleId id);

};

}
}
}

#endif

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
