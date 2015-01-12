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
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <unordered_map>
#include <list>
#include <set>
#include <memory>


#include "Instruction.hpp"

using namespace std;

namespace cacao {
namespace jit {
namespace compiler2{

// typedefs and macros needed by generated matcher
typedef Instruction* NODEPTR_TYPE;
typedef struct burm_state * STATE_TYPE;

#define OP_LABEL(p) ((p)->get_opcode())

// TODO maybe check for bounds?
#define LEFT_CHILD(p) (getOperand(p, 0))
#define RIGHT_CHILD(p) (getOperand(p, 1))
#define STATE_LABEL(p) (state_labels[p])

#define ABORT_MSG(a,b) printf(a)

// INCLUDE GENERATED DEFINES
#define GLOBALDEFS
#include "Grammar.inc"

// INCLUDE GENERATED DEFINES
#define GENDEFS
#include "Grammar.inc"

class Matcher {

public:

	// typedefs to avoid naming dependency to generated matcher
	typedef struct burm_state * StateTy;

	Matcher(std::list<Instruction*> *instructions) : 
		instructions(instructions) {

		basicBlock = instructions->front()->get_BeginInst();
	}

	void run();

private: 
	
	std::list<Instruction*> *instructions;
	std::unordered_map<Instruction*, std::set<Instruction*>* > roots;
	std::unordered_map<Instruction*, std::set<Instruction*>* > revdeps;

	std::unordered_map<Instruction*,StateTy> state_labels;
	std::unordered_map<Instruction*,std::unordered_map<int, Instruction*>* > instrProxies;
	Instruction* basicBlock;

	static const std::set<Instruction::InstID> excluded_nodes;

	void burm_trace(Instruction* p, int eruleno, int cost, int bestcost);

	bool checkIsNodeExcluded(Instruction* inst);

	Instruction* findRoot(Instruction* inst);


// INCLUDE GENERATED MATCHER ALGORITHM 
#define GENMETHODDEFS
#include "Grammar.inc"

	Instruction* getOperand(Instruction* op, int pos);

	void dumpCover(Instruction* p, int goalnt, int indent);

	void lowerTree(Instruction* p, int goalnt);

	void scheduleTrees();

	void findRoots();

	void printDependencies(Instruction* inst);

	// TODO move lowering to LV
	void lowerRule(Instruction* inst, RuleId id);

};

}
}
}

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