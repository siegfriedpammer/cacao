#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct automaton_context_t *automaton_context;
typedef struct burm_state *STATEPTR_TYPE;
struct instruction;
struct jitdata;

char burm_arity[];
char *burm_opname[];

typedef struct node_t {
	int		op;
	struct node_t   *kids[2];
	STATEPTR_TYPE	state;
    /* user defined data fields follow here */
	struct instruction *iptr;
	void *jd;
} *node;

#define NODEPTR_TYPE	node
#define OP_LABEL(p)	((p)->op)
#define LEFT_CHILD(p)	((p)->kids[0])
#define RIGHT_CHILD(p)	((p)->kids[1])
#define STATE_LABEL(p)	((p)->state)
#define PANIC		printf

void codegen_emit_instruction(NODEPTR_TYPE bnode);
void codegen_nop(NODEPTR_TYPE bnode);
void codegen_emit_builtin(NODEPTR_TYPE bnode);
void codegen_emit_breakpoint(NODEPTR_TYPE bnode);
void codegen_emit_checknull(NODEPTR_TYPE bnode);
void codegen_emit_iconst(NODEPTR_TYPE bnode);
void codegen_emit_lconst(NODEPTR_TYPE bnode);
void codegen_emit_undef(NODEPTR_TYPE bnode);
void codegen_emit_astore(NODEPTR_TYPE bnode);
void codegen_emit_branch(NODEPTR_TYPE bnode);
void codegen_emit_copy(NODEPTR_TYPE bnode);
void codegen_emit_epilog(NODEPTR_TYPE bnode);
void codegen_emit_getexception(NODEPTR_TYPE bnode);
void codegen_emit_getstatic(NODEPTR_TYPE bnode);
void codegen_emit_putstatic(NODEPTR_TYPE bnode);
void codegen_emit_ifnull(NODEPTR_TYPE bnode);
void codegen_emit_inline_body(NODEPTR_TYPE bnode);
void codegen_emit_inline_end(NODEPTR_TYPE bnode);
void codegen_emit_inline_start(NODEPTR_TYPE bnode);
void codegen_emit_invoke(NODEPTR_TYPE bnode);
void codegen_emit_jump(NODEPTR_TYPE bnode);
void codegen_emit_lookup(NODEPTR_TYPE bnode);
void codegen_emit_phi(NODEPTR_TYPE bnode);
void codegen_emit_unknown(NODEPTR_TYPE bnode);
void codegen_emit_return(NODEPTR_TYPE bnode);