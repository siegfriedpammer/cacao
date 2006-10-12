#include "config.h"
#include "vm/types.h"
#include "vm/global.h"

#include <assert.h>

#include <vm/jit/show.h>
#include <typecheck-common.h>

/****************************************************************************/
/* DEBUG HELPERS                                                            */
/****************************************************************************/

#ifdef TYPECHECK_VERBOSE_OPT
bool opt_typecheckverbose = false;
#endif

#if defined(TYPECHECK_VERBOSE) || defined(TYPECHECK_VERBOSE_IMPORTANT)

void typecheck_print_var(FILE *file, jitdata *jd, s4 index)
{
	varinfo *var;

	assert(index >= 0 && index < jd->varcount);
	var = VAR(index);
	typeinfo_print_type(file, var->type, &(var->typeinfo));
}

void typecheck_print_vararray(FILE *file, jitdata *jd, s4 *vars, int len)
{
	s4 i;

	for (i=0; i<len; ++i) {
		if (i)
			fputc(' ', file);
		typecheck_print_var(file, jd, *vars++);
	}
}

#endif /* defined(TYPECHECK_VERBOSE) || defined(TYPECHECK_VERBOSE_IMPORTANT) */


/****************************************************************************/
/* STATISTICS                                                               */
/****************************************************************************/

#if defined(TYPECHECK_STATISTICS)
static int stat_typechecked = 0;
static int stat_methods_with_handlers = 0;
static int stat_methods_maythrow = 0;
static int stat_iterations[STAT_ITERATIONS+1] = { 0 };
static int stat_reached = 0;
static int stat_copied = 0;
static int stat_merged = 0;
static int stat_merging_changed = 0;
static int stat_blocks[STAT_BLOCKS+1] = { 0 };
static int stat_locals[STAT_LOCALS+1] = { 0 };
static int stat_ins = 0;
static int stat_ins_maythrow = 0;
static int stat_ins_stack = 0;
static int stat_ins_field = 0;
static int stat_ins_field_unresolved = 0;
static int stat_ins_field_uninitialized = 0;
static int stat_ins_invoke = 0;
static int stat_ins_invoke_unresolved = 0;
static int stat_ins_primload = 0;
static int stat_ins_aload = 0;
static int stat_ins_builtin = 0;
static int stat_ins_builtin_gen = 0;
static int stat_ins_branch = 0;
static int stat_ins_switch = 0;
static int stat_ins_primitive_return = 0;
static int stat_ins_areturn = 0;
static int stat_ins_areturn_unresolved = 0;
static int stat_ins_athrow = 0;
static int stat_ins_athrow_unresolved = 0;
static int stat_ins_unchecked = 0;
static int stat_handlers_reached = 0;
static int stat_savedstack = 0;

static void print_freq(FILE *file,int *array,int limit)
{
	int i;
	for (i=0; i<limit; ++i)
		fprintf(file,"      %3d: %8d\n",i,array[i]);
	fprintf(file,"    >=%3d: %8d\n",limit,array[limit]);
}

void typecheck_print_statistics(FILE *file) {
	fprintf(file,"typechecked methods: %8d\n",stat_typechecked);
	fprintf(file,"    with handler(s): %8d\n",stat_methods_with_handlers);
	fprintf(file,"    with throw(s)  : %8d\n",stat_methods_maythrow);
	fprintf(file,"reached blocks     : %8d\n",stat_reached);
	fprintf(file,"copied states      : %8d\n",stat_copied);
	fprintf(file,"merged states      : %8d\n",stat_merged);
	fprintf(file,"merging changed    : %8d\n",stat_merging_changed);
	fprintf(file,"handlers reached   : %8d\n",stat_handlers_reached);
	fprintf(file,"saved stack (times): %8d\n",stat_savedstack);
	fprintf(file,"instructions       : %8d\n",stat_ins);
	fprintf(file,"    stack          : %8d\n",stat_ins_stack);
	fprintf(file,"    field access   : %8d\n",stat_ins_field);
	fprintf(file,"      (unresolved) : %8d\n",stat_ins_field_unresolved);
	fprintf(file,"      (uninit.)    : %8d\n",stat_ins_field_uninitialized);
	fprintf(file,"    invocations    : %8d\n",stat_ins_invoke);
	fprintf(file,"      (unresolved) : %8d\n",stat_ins_invoke_unresolved);
	fprintf(file,"    load primitive : (currently not counted) %8d\n",stat_ins_primload);
	fprintf(file,"    load address   : %8d\n",stat_ins_aload);
	fprintf(file,"    builtins       : %8d\n",stat_ins_builtin);
	fprintf(file,"        generic    : %8d\n",stat_ins_builtin_gen);
	fprintf(file,"    branches       : %8d\n",stat_ins_branch);
	fprintf(file,"    switches       : %8d\n",stat_ins_switch);
	fprintf(file,"    prim. return   : %8d\n",stat_ins_primitive_return);
	fprintf(file,"    areturn        : %8d\n",stat_ins_areturn);
	fprintf(file,"      (unresolved) : %8d\n",stat_ins_areturn_unresolved);
	fprintf(file,"    athrow         : %8d\n",stat_ins_athrow);
	fprintf(file,"      (unresolved) : %8d\n",stat_ins_athrow_unresolved);
	fprintf(file,"    unchecked      : %8d\n",stat_ins_unchecked);
	fprintf(file,"    maythrow       : %8d\n",stat_ins_maythrow);
	fprintf(file,"iterations used:\n");
	print_freq(file,stat_iterations,STAT_ITERATIONS);
	fprintf(file,"basic blocks per method / 10:\n");
	print_freq(file,stat_blocks,STAT_BLOCKS);
	fprintf(file,"locals:\n");
	print_freq(file,stat_locals,STAT_LOCALS);
}
#endif /* defined(TYPECHECK_STATISTICS) */


/* typecheck_init_flags ********************************************************
 
   Initialize the basic block flags for the following CFG traversal.
  
   IN:
       state............the current state of the verifier
       minflags.........minimum flags value of blocks that should be
                        considered

*******************************************************************************/

void typecheck_init_flags(verifier_state *state, s4 minflags)
{
	s4 i;
	basicblock *block;

    /* set all BBFINISHED blocks to BBTYPECHECK_UNDEF. */
	
    i = state->basicblockcount;
    for (block = state->basicblocks; block; block = block->next) {
		
#ifdef TYPECHECK_DEBUG
		/* check for invalid flags */
        if (block->flags != BBFINISHED && block->flags != BBDELETED && block->flags != BBUNDEF)
        {
            LOGSTR1("block flags: %d\n",block->flags); LOGFLUSH;
			TYPECHECK_ASSERT(false);
        }
#endif

        if (block->flags >= minflags) {
            block->flags = BBTYPECHECK_UNDEF;
        }
    }

    /* the first block is always reached */
	
    if (state->basicblockcount && state->basicblocks[0].flags == BBTYPECHECK_UNDEF)
        state->basicblocks[0].flags = BBTYPECHECK_REACHED;
}


/* typecheck_reset_flags *******************************************************
 
   Reset the flags of basic blocks we have not reached.
  
   IN:
       state............the current state of the verifier

*******************************************************************************/

void typecheck_reset_flags(verifier_state *state)
{
	basicblock *block;

	/* check for invalid flags at exit */
	
#ifdef TYPECHECK_DEBUG
	for (block = state->basicblocks; block; block = block->next) {
		if (block->flags != BBDELETED
			&& block->flags != BBUNDEF
			&& block->flags != BBFINISHED
			&& block->flags != BBTYPECHECK_UNDEF) /* typecheck may never reach
													 * some exception handlers,
													 * that's ok. */
		{
			LOG2("block L%03d has invalid flags after typecheck: %d",
				 block->nr,block->flags);
			TYPECHECK_ASSERT(false);
		}
	}
#endif
	
	/* Delete blocks we never reached */
	
	for (block = state->basicblocks; block; block = block->next) {
		if (block->flags == BBTYPECHECK_UNDEF)
			block->flags = BBDELETED;
	}
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
