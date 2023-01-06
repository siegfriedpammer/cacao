#ifndef _AUTOMATON_AUTOMATA_HPP_
#define _AUTOMATON_AUTOMATA_HPP_ 1

#include "vm/jit/linenumbertable.hpp"
#include "codegen.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/ir/instruction.hpp"
#include "vm/jit/builtin.hpp"
#include "vm/jit/show.hpp"
#include "vm/descriptor.hpp"
#include "vm/method.hpp"
#include "automata-common.hpp"
#include "automata-codegen.hpp"

void automaton_run(basicblock *bptr, jitdata *jd);

#ifdef AUTOMATON_KIND_IBURG 

extern char burm_arity[];
extern char *burm_opname[];

typedef struct burm_state {
	int op;
	void *left, *right;
	short cost[2];
	struct {
		unsigned burm_reg:3;
	} rule;
} *STATEPTR_TYPE;

extern "C" STATEPTR_TYPE burm_label(node root);
extern "C" void burm_reduce(node root, int goalnt);

#endif

#ifdef AUTOMATON_KIND_FSM

extern char *burm_opname[];
extern "C" int next(int a, int b);
extern "C" void execute_pre_action(int state, int symbol, struct jitdata *jd, struct instruction *iptr, struct instruction **tos);
extern "C" void execute_post_action(int state, int symbol, struct jitdata *jd, struct instruction *iptr, struct instruction **tos);

#endif

#endif