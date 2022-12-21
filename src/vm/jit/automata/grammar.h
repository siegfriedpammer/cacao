#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

//#define AUTOMATON_KIND_IBURG 1
#define AUTOMATON_KIND_FSM 2

#define FSM_MAX_STACK 4

struct instruction;
struct jitdata;

#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_



#ifdef AUTOMATON_KIND_FSM

typedef struct fsm_node_t {
	int		op;
    /* user defined data fields follow here */
	struct instruction *iptr;
	struct instruction *prev;
	struct jitdata *jd;
    int offset;
} *fsm_node;


#define NODEPTR_TYPE	fsm_node
extern char *burm_opname[];
extern char burm_arity[];
extern int state_stackdepth[];
extern int stackdepth_default_state[];

#define PREV_LEFT(p)	(prev)
#define PREV_RIGHT(p)	(prev)
#define CURRENT_INST(p) (current)
#define PREV_INST(p)    (prev)

#elif AUTOMATON_KIND_IBURG

extern char burm_arity[];
extern char *burm_opname[];

typedef struct burm_state *STATEPTR_TYPE;

typedef struct node_t {
	int		op;
	struct node_t   *kids[2];
	STATEPTR_TYPE state;
    /* user defined data fields follow here */
	struct instruction *iptr;
	struct jitdata *jd;
    int offset;
    bool has_side_effects;
} *node;

#define NODEPTR_TYPE	node

#define OP_LABEL(p)	((p)->op)
#define LEFT_CHILD(p)	((p)->kids[0])
#define RIGHT_CHILD(p)	((p)->kids[1])
#define CURRENT_INST(p) ((p)->iptr)
#define JITDATA(p) ((p)->jd)
#define PREV_INST_LEFT(p)	((p)->kids[0]->iptr)
#define PREV_INST_RIGHT(p)	((p)->kids[1]->iptr)
#define STATE_LABEL(p)	((p)->state)
#define PANIC		printf

#endif

void codegen_emit_simple_instruction(struct jitdata *jd, struct instruction *iptr);
void codegen_nop(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_builtin(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_breakpoint(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_checknull(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_iconst(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_lconst(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_iadd(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_iaddconst(struct jitdata *jd, struct instruction *iptr, struct instruction *constant);
void codegen_emit_iaddconst_right(struct jitdata *jd, struct instruction *iptr, struct instruction *constant);
void codegen_emit_undef(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_astore(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_branch(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_copy(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_epilog(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_getexception(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_getstatic(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_putstatic(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_ifnull(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_inline_body(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_inline_end(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_inline_start(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_invoke(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_jump(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_lookup(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_phi(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_unknown(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_return(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_result(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_throw(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_arraylength(struct jitdata *jd, struct instruction *iptr);
void codegen_emit_combined_instruction(struct jitdata *jd, struct instruction *iptr);
void codegen_push_to_reg(struct jitdata *jd, struct instruction *iptr);
void remember_instruction(struct jitdata *jd, struct instruction *iptr);

int get_opc(struct instruction *iptr);

#endif