#include "vm/jit/codegen-common.hpp"
#include "vm/jit/ir/instruction.hpp"

#ifndef _AUTOMATON_CODEGEN_HPP_
#define _AUTOMATON_CODEGEN_HPP_

struct burm_state {
	int op;
	void *left, *right;
	short cost[2];
	struct {
		unsigned burm_reg:3;
	} rule;
};

typedef struct automaton_context_t *automaton_context;

typedef struct node_t {
	int		op;
	struct node_t   *kids[2];
	void	*state;
    /* user defined data fields follow here */
	struct instruction *iptr;
	jitdata *jd;
    int offset;
    bool has_side_effects;
} *node;

extern "C" {
    void codegen_emit_instruction(node bnode);
    void codegen_emit_throw(node bnode);
    void codegen_emit_arraylength(node bnode);
    void codegen_nop(node bnode);
    void codegen_emit_builtin(node bnode);
    void codegen_emit_breakpoint(node bnode);
    void codegen_emit_checknull(node bnode);
    void codegen_emit_iconst(node bnode);
    void codegen_emit_lconst(node bnode);
    void codegen_emit_undef(node bnode);
    void codegen_emit_astore(node bnode);
    void codegen_emit_branch(node bnode);
    void codegen_emit_copy(node bnode);
    void codegen_emit_getexception(node bnode);
    void codegen_emit_getstatic(node bnode);
    void codegen_emit_putstatic(node bnode);
    void codegen_emit_ifnull(node bnode);
    void codegen_emit_inline_body(node bnode);
    void codegen_emit_inline_end(node bnode);
    void codegen_emit_inline_start(node bnode);
    void codegen_emit_invoke(node bnode);
    void codegen_emit_jump(node bnode);
    void codegen_emit_lookup(node bnode);
    void codegen_emit_phi(node bnode);
    void codegen_emit_return(node bnode);
    void codegen_emit_result(node bnode);
    void codegen_emit_arraylength(node bnode);
    void codegen_emit_const_return(node bnode);
}

#endif