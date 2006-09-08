/* src/vm/jit/stack.c - stack analysis

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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

   Contact: cacao@cacaojvm.org

   Authors: Andreas Krall

   Changes: Edwin Steiner
            Christian Thalinger
            Christian Ullrich

   $Id: stack.c 5440 2006-09-08 23:59:52Z edwin $

*/


#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "vm/types.h"

#include "arch.h"
#include "md-abi.h"

#include "mm/memory.h"
#include "native/native.h"
#include "toolbox/logging.h"
#include "vm/global.h"
#include "vm/builtin.h"
#include "vm/options.h"
#include "vm/resolve.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/jit/cfg.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/abi.h"
#include "vm/jit/show.h"

#if defined(ENABLE_DISASSEMBLER)
# include "vm/jit/disass.h"
#endif

#include "vm/jit/jit.h"
#include "vm/jit/stack.h"

#if defined(ENABLE_LSRA)
# include "vm/jit/allocator/lsra.h"
#endif

/*#define STACK_VERBOSE*/


/* macro for saving #ifdefs ***************************************************/

#if defined(ENABLE_INTRP)
#define IF_INTRP(x) if (opt_intrp) { x }
#define IF_NO_INTRP(x) if (!opt_intrp) { x }
#else
#define IF_INTRP(x)
#define IF_NO_INTRP(x) { x }
#endif

#if defined(ENABLE_INTRP)
#if defined(ENABLE_JIT)
#define IF_JIT(x) if (!opt_intrp) { x }
#else
#define IF_JIT(x)
#endif
#else /* !defined(ENABLE_INTRP) */
#define IF_JIT(x) { x }
#endif /* defined(ENABLE_INTRP) */

#if defined(ENABLE_STATISTICS)
#define STATISTICS_STACKDEPTH_DISTRIBUTION(distr)                    \
    do {                                                             \
        if (opt_stat) {                                              \
            if (stackdepth >= 10)                                    \
                count_store_depth[10]++;                             \
            else                                                     \
                count_store_depth[stackdepth]++;                     \
        }                                                            \
    } while (0)
#else /* !defined(ENABLE_STATISTICS) */
#define STATISTICS_STACKDEPTH_DISTRIBUTION(distr)
#endif

/* stackdata_t ****************************************************************/

typedef struct stackdata_t stackdata_t;

struct stackdata_t {
    basicblock *bptr;
    stackptr new;
    s4 vartop;
    s4 localcount;
    s4 varcount;
    varinfo *var;
};


/* macros for allocating/releasing variable indices */

#define GET_NEW_INDEX(sd, new_varindex)                              \
    do {                                                             \
        assert((sd).vartop < (sd).varcount);                         \
        (new_varindex) = ((sd).vartop)++;                            \
    } while (0)

/* not implemented now, can be used to reuse varindices          */
/* pay attention to not release a localvar once implementing it! */
#define RELEASE_INDEX(sd, varindex)

#define GET_NEW_VAR(sd, new_varindex, newtype)                       \
    do {                                                             \
        GET_NEW_INDEX((sd), (new_varindex));                         \
        (sd).var[new_index].type = (newtype);                        \
    } while (0)

/* macros for querying variable properties **************************/

#define IS_OUTVAR(sp)                                                \
    (sd.var[(sp)->varnum].flags & OUTVAR)

#define IS_PREALLOC(sp)                                              \
    (sd.var[(sp)->varnum].flags & PREALLOC)

#define IS_TEMPVAR(sp)                                               \
    ( ((sp)->varnum >= sd.localcount)                                \
      && !(sd.var[(sp)->varnum].flags & (OUTVAR | PREALLOC)) )

#define IS_LOCALVAR_SD(sd, sp)                                       \
         ((sp)->varnum < (sd).localcount)

#define IS_LOCALVAR(sp)                                              \
    IS_LOCALVAR_SD(sd, (sp))


/* macros for setting variable properties ****************************/

#define SET_TEMPVAR(sp)                                              \
    do {                                                             \
        if (IS_LOCALVAR((sp))) {                                     \
            GET_NEW_VAR(sd, new_index, (sp)->type);                  \
            sd.var[new_index].flags = (sp)->flags;                   \
            (sp)->varnum = new_index;                                \
			(sp)->varkind = TEMPVAR;                                 \
			if ((sp)->creator)                                       \
				(sp)->creator->dst.varindex = new_index;             \
        }                                                            \
        sd.var[(sp)->varnum].flags &= ~(OUTVAR | PREALLOC);          \
    } while (0);

#define SET_PREALLOC(sp)                                             \
    do {                                                             \
        assert(!IS_LOCALVAR((sp)));                                  \
        sd.var[(sp)->varnum].flags |= PREALLOC;                      \
    } while (0);

/* macros for source operands ***************************************/

#define CLR_S1                                                       \
    (iptr->s1.varindex = -1)

#define USE_S1_LOCAL(type1)

#define USE_S1(type1)                                                \
    do {                                                             \
        REQUIRE_1;                                                   \
        CHECK_BASIC_TYPE(type1, curstack->type);                     \
        iptr->s1.varindex = curstack->varnum;                        \
    } while (0)

#define USE_S1_ANY                                                   \
    do {                                                             \
        REQUIRE_1;                                                   \
        iptr->s1.varindex = curstack->varnum;                        \
    } while (0)

#define USE_S1_S2(type1, type2)                                      \
    do {                                                             \
        REQUIRE_2;                                                   \
        CHECK_BASIC_TYPE(type1, curstack->prev->type);               \
        CHECK_BASIC_TYPE(type2, curstack->type);                     \
        iptr->sx.s23.s2.varindex = curstack->varnum;                 \
        iptr->s1.varindex = curstack->prev->varnum;                  \
    } while (0)

#define USE_S1_S2_ANY_ANY                                            \
    do {                                                             \
        REQUIRE_2;                                                   \
        iptr->sx.s23.s2.varindex = curstack->varnum;                 \
        iptr->s1.varindex = curstack->prev->varnum;                  \
    } while (0)

#define USE_S1_S2_S3(type1, type2, type3)                            \
    do {                                                             \
        REQUIRE_3;                                                   \
        CHECK_BASIC_TYPE(type1, curstack->prev->prev->type);         \
        CHECK_BASIC_TYPE(type2, curstack->prev->type);               \
        CHECK_BASIC_TYPE(type3, curstack->type);                     \
        iptr->sx.s23.s3.varindex = curstack->varnum;                 \
        iptr->sx.s23.s2.varindex = curstack->prev->varnum;           \
        iptr->s1.varindex = curstack->prev->prev->varnum;            \
    } while (0)

/* The POPANY macro does NOT check stackdepth, or set stackdepth!   */
#define POPANY                                                       \
    do {                                                             \
        if (curstack->varkind == UNDEFVAR)                           \
            curstack->varkind = TEMPVAR;                             \
        curstack = curstack->prev;                                   \
    } while (0)

#define POP_S1(type1)                                                \
    do {                                                             \
        USE_S1(type1);                                               \
        if (curstack->varkind == UNDEFVAR)                           \
            curstack->varkind = TEMPVAR;                             \
        curstack = curstack->prev;                                   \
    } while (0)

#define POP_S1_ANY                                                   \
    do {                                                             \
        USE_S1_ANY;                                                  \
        if (curstack->varkind == UNDEFVAR)                           \
            curstack->varkind = TEMPVAR;                             \
        curstack = curstack->prev;                                   \
    } while (0)

#define POP_S1_S2(type1, type2)                                      \
    do {                                                             \
        USE_S1_S2(type1, type2);                                     \
        if (curstack->varkind == UNDEFVAR)                           \
            curstack->varkind = TEMPVAR;                             \
        if (curstack->prev->varkind == UNDEFVAR)                     \
            curstack->prev->varkind = TEMPVAR;                       \
        curstack = curstack->prev->prev;                             \
    } while (0)

#define POP_S1_S2_ANY_ANY                                            \
    do {                                                             \
        USE_S1_S2_ANY_ANY;                                           \
        if (curstack->varkind == UNDEFVAR)                           \
            curstack->varkind = TEMPVAR;                             \
        if (curstack->prev->varkind == UNDEFVAR)                     \
            curstack->prev->varkind = TEMPVAR;                       \
        curstack = curstack->prev->prev;                             \
    } while (0)

#define POP_S1_S2_S3(type1, type2, type3)                            \
    do {                                                             \
        USE_S1_S2_S3(type1, type2, type3);                           \
        if (curstack->varkind == UNDEFVAR)                           \
            curstack->varkind = TEMPVAR;                             \
        if (curstack->prev->varkind == UNDEFVAR)                     \
            curstack->prev->varkind = TEMPVAR;                       \
        if (curstack->prev->prev->varkind == UNDEFVAR)               \
            curstack->prev->prev->varkind = TEMPVAR;                 \
        curstack = curstack->prev->prev->prev;                       \
    } while (0)

#define CLR_SX                                                       \
    (iptr->sx.val.l = 0)


/* macros for setting the destination operand ***********************/

#define CLR_DST                                                      \
    (iptr->dst.varindex = -1)

#define DST(typed, index)                                            \
    do {                                                             \
        NEWSTACKn((typed),(index));                                  \
        curstack->creator = iptr;                                    \
        iptr->dst.varindex = (index);                                \
    } while (0)

#define DST_LOCALVAR(typed, index)                                   \
    do {                                                             \
        NEWSTACK((typed), LOCALVAR, (index));                        \
        curstack->creator = iptr;                                    \
        iptr->dst.varindex = (index);                                \
    } while (0)


/* stack modelling macros *******************************************/

#define OP0_1(typed)                                                 \
    do {                                                             \
        CLR_S1;                                                      \
        GET_NEW_VAR(sd, new_index, (typed));                         \
        DST(typed, new_index);                                       \
        stackdepth++;                                                \
    } while (0)

#define OP1_0_ANY                                                    \
    do {                                                             \
        POP_S1_ANY;                                                  \
        CLR_DST;                                                     \
        stackdepth--;                                                \
    } while (0)

#define OP1_BRANCH(type1)                                            \
    do {                                                             \
        POP_S1(type1);                                               \
        stackdepth--;                                                \
    } while (0)

#define OP1_1(type1, typed)                                          \
    do {                                                             \
        POP_S1(type1);                                               \
        GET_NEW_VAR(sd, new_index, (typed));                         \
        DST(typed, new_index);                                       \
    } while (0)

#define OP2_1(type1, type2, typed)                                   \
    do {                                                             \
        POP_S1_S2(type1, type2);                                     \
        GET_NEW_VAR(sd, new_index, (typed));                         \
        DST(typed, new_index);                                       \
        stackdepth--;                                                \
    } while (0)

#define OP0_0                                                        \
    do {                                                             \
        CLR_S1;                                                      \
        CLR_DST;                                                     \
    } while (0)

#define OP0_BRANCH                                                   \
    do {                                                             \
        CLR_S1;                                                      \
    } while (0)

#define OP1_0(type1)                                                 \
    do {                                                             \
        POP_S1(type1);                                               \
        CLR_DST;                                                     \
        stackdepth--;                                                \
    } while (0)

#define OP2_0(type1, type2)                                          \
    do {                                                             \
        POP_S1_S2(type1, type2);                                     \
        CLR_DST;                                                     \
        stackdepth -= 2;                                             \
    } while (0)

#define OP2_BRANCH(type1, type2)                                     \
    do {                                                             \
        POP_S1_S2(type1, type2);                                     \
        stackdepth -= 2;                                             \
    } while (0)

#define OP2_0_ANY_ANY                                                \
    do {                                                             \
        POP_S1_S2_ANY_ANY;                                           \
        CLR_DST;                                                     \
        stackdepth -= 2;                                             \
    } while (0)

#define OP3_0(type1, type2, type3)                                   \
    do {                                                             \
        POP_S1_S2_S3(type1, type2, type3);                           \
        CLR_DST;                                                     \
        stackdepth -= 3;                                             \
    } while (0)

#define LOAD(type1, index)                                           \
    do {                                                             \
        DST_LOCALVAR(type1, index);                                  \
        stackdepth++;                                                \
    } while (0)

#define STORE(type1, index)                                          \
    do {                                                             \
        POP_S1(type1);                                               \
        stackdepth--;                                                \
    } while (0)


/* macros for DUP elimination ***************************************/

/* XXX turn off coalescing */
#if 0
#define DUP_SLOT(sp)                                                 \
    do {                                                             \
        if ((sp)->varkind != TEMPVAR) {                              \
            GET_NEW_VAR(sd, new_index, (sp)->type);                  \
            NEWSTACK((sp)->type, TEMPVAR, new_index);                \
        }                                                            \
        else                                                         \
            NEWSTACK((sp)->type, (sp)->varkind, (sp)->varnum);       \
    } while(0)
#else
#define DUP_SLOT(sp)                                                 \
    do {                                                             \
            GET_NEW_VAR(sd, new_index, (sp)->type);                  \
            NEWSTACK((sp)->type, TEMPVAR, new_index);                \
    } while(0)
#endif

/* does not check input stackdepth */
#define MOVE_UP(sp)                                                  \
    do {                                                             \
        iptr->opc = ICMD_MOVE;                                       \
        iptr->s1.varindex = (sp)->varnum;                            \
        DUP_SLOT(sp);                                                \
        curstack->creator = iptr;                                    \
        iptr->dst.varindex = curstack->varnum;                       \
        stackdepth++;                                                \
    } while (0)

/* does not check input stackdepth */
#define COPY_UP(sp)                                                  \
    do {                                                             \
        SET_TEMPVAR((sp));                                           \
        iptr->opc = ICMD_COPY;                                       \
        iptr->s1.varindex = (sp)->varnum;                            \
        DUP_SLOT(sp);                                                \
        curstack->creator = iptr;                                    \
        iptr->dst.varindex = curstack->varnum;                       \
        stackdepth++;                                                \
    } while (0)

#define COPY_DOWN(s, d)                                              \
    do {                                                             \
        SET_TEMPVAR((s));                                            \
        iptr->opc = ICMD_COPY;                                       \
        iptr->s1.varindex = (s)->varnum;                             \
        iptr->dst.varindex = (d)->varnum;                            \
        (d)->creator = iptr;                                         \
    } while (0)


/* macros for branching / reaching basic blocks *********************/

#if defined(ENABLE_VERIFIER)
#define MARKREACHED(b, c)                                            \
    do {                                                             \
        if (!stack_mark_reached(&sd, (b), curstack, stackdepth))     \
            return false;                                            \
    } while (0)
#else
#define MARKREACHED(b, c)                                            \
    do {                                                             \
        (void) stack_mark_reached(&sd, (b), curstack, stackdepth);   \
    } while (0)
#endif

#define BRANCH_TARGET(bt, tempbptr, tempsp)                          \
    do {                                                             \
        (bt).block = tempbptr = BLOCK_OF((bt).insindex);             \
        MARKREACHED(tempbptr, tempsp);                               \
    } while (0)

#define BRANCH(tempbptr, tempsp)                                     \
    do {                                                             \
        iptr->dst.block = tempbptr = BLOCK_OF(iptr->dst.insindex);   \
        MARKREACHED(tempbptr, tempsp);                               \
    } while (0)


/* stack_init ******************************************************************

   Initialized the stack analysis subsystem (called by jit_init).

*******************************************************************************/

bool stack_init(void)
{
	return true;
}


/* MARKREACHED marks the destination block <b> as reached. If this
 * block has been reached before we check if stack depth and types
 * match. Otherwise the destination block receives a copy of the
 * current stack as its input stack.
 *
 * b...destination block
 * c...current stack
 */

static bool stack_mark_reached(stackdata_t *sd, basicblock *b, stackptr curstack, int stackdepth) 
{
	stackptr sp, tsp;
	int i;
	s4 new_index;
#if defined(ENABLE_VERIFIER)
	int           expectedtype;   /* used by CHECK_BASIC_TYPE                 */
#endif

	if (b <= sd->bptr)
		b->bitflags |= BBFLAG_REPLACEMENT;

	if (b->flags < BBREACHED) {
		/* b is reached for the first time. Create its instack */
		COPYCURSTACK(*sd, sp);
		b->flags = BBREACHED;
		b->instack = sp;
		b->indepth = stackdepth;
		b->invars = DMNEW(s4, stackdepth);
		for (i = stackdepth; i--; sp = sp->prev) { 
			b->invars[i] = sp->varnum;
			sd->var[sp->varnum].flags |= OUTVAR;
		}
	} 
	else {
		/* b has been reached before. Check that its instack matches */
		sp = curstack;
		tsp = b->instack;
		CHECK_STACK_DEPTH(b->indepth, stackdepth);
		while (sp) {
			CHECK_BASIC_TYPE(sp->type,tsp->type);
			sp = sp->prev;
			tsp = tsp->prev;
		}
	}

	return true;

#if defined(ENABLE_VERIFIER)
throw_stack_depth_error:
	exceptions_throw_verifyerror(m,"Stack depth mismatch");
	return false;

throw_stack_type_error:
	exceptions_throw_verifyerror_for_stack(m, expectedtype);
	return false;
#endif
}


/* stack_analyse ***************************************************************

   Analyse_stack uses the intermediate code created by parse.c to
   build a model of the JVM operand stack for the current method.
   
   The following checks are performed:
     - check for operand stack underflow (before each instruction)
     - check for operand stack overflow (after[1] each instruction)
     - check for matching stack depth at merging points
     - check for matching basic types[2] at merging points
     - check basic types for instruction input (except for BUILTIN*
           opcodes, INVOKE* opcodes and MULTIANEWARRAY)
   
   [1]) Checking this after the instruction should be ok. parse.c
   counts the number of required stack slots in such a way that it is
   only vital that we don't exceed `maxstack` at basic block
   boundaries.
   
   [2]) 'basic types' means the distinction between INT, LONG, FLOAT,
   DOUBLE and ADDRESS types. Subtypes of INT and different ADDRESS
   types are not discerned.

*******************************************************************************/

bool new_stack_analyse(jitdata *jd)
{
	methodinfo   *m;              /* method being analyzed                    */
	codeinfo     *code;
	codegendata  *cd;
	registerdata *rd;
	stackdata_t   sd;
	int           b_count;        /* basic block counter                      */
	int           b_index;        /* basic block index                        */
	int           stackdepth;
	stackptr      curstack;       /* current stack top                        */
	stackptr      copy;
	int           opcode;         /* opcode of current instruction            */
	int           i, j;
	int           javaindex;
	int           len;            /* # of instructions after the current one  */
	bool          superblockend;  /* if true, no fallthrough to next block    */
	bool          repeat;         /* if true, outermost loop must run again   */
	bool          deadcode;       /* true if no live code has been reached    */
	instruction  *iptr;           /* the current instruction                  */
	basicblock   *tbptr;

	stackptr     *last_store_boundary;
	stackptr      coalescing_boundary;

	stackptr      src1, src2, src3, src4, dst1, dst2;

	branch_target_t *table;
	lookup_target_t *lookup;
#if defined(ENABLE_VERIFIER)
	int           expectedtype;   /* used by CHECK_BASIC_TYPE                 */
#endif
	builtintable_entry *bte;
	methoddesc         *md;
	constant_FMIref    *fmiref;
#if defined(ENABLE_STATISTICS)
	int           iteration_count;  /* number of iterations of analysis       */
#endif
	int           new_index; /* used to get a new var index with GET_NEW_INDEX*/

#if defined(STACK_VERBOSE)
	new_show_method(jd, SHOW_PARSE);
#endif

	/* get required compiler data - initialization */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	rd   = jd->rd;

	/* initialize the stackdata_t struct */

	sd.varcount = jd->varcount;
	sd.vartop =  jd->vartop;
	sd.localcount = jd->localcount;
	sd.var = jd->var;
	sd.new = jd->new_stack;

#if defined(ENABLE_LSRA)
	m->maxlifetimes = 0;
#endif

#if defined(ENABLE_STATISTICS)
	iteration_count = 0;
#endif

	/* init jd->interface_map */

	jd->interface_map = DMNEW(interface_info, m->maxstack * 5);
	for (i = 0; i < m->maxstack * 5; i++)
		jd->interface_map[i].flags = UNUSED;

	last_store_boundary = DMNEW(stackptr, cd->maxlocals);

	/* initialize in-stack of first block */

	jd->new_basicblocks[0].flags = BBREACHED;
	jd->new_basicblocks[0].instack = NULL;
	jd->new_basicblocks[0].invars = NULL;
	jd->new_basicblocks[0].indepth = 0;

	/* initialize in-stack of exception handlers */

	for (i = 0; i < cd->exceptiontablelength; i++) {
		sd.bptr = BLOCK_OF(cd->exceptiontable[i].handlerpc);
		sd.bptr->flags = BBREACHED;
		sd.bptr->type = BBTYPE_EXH;
		sd.bptr->instack = sd.new;
		sd.bptr->indepth = 1;
		sd.bptr->predecessorcount = CFG_UNKNOWN_PREDECESSORS;
		curstack = NULL; stackdepth = 0;
		GET_NEW_VAR(sd, new_index, TYPE_ADR);
		sd.bptr->invars = DMNEW(s4, 1);
		sd.bptr->invars[0] = new_index;
		sd.var[new_index].flags |= OUTVAR;
		NEWSTACK(TYPE_ADR, STACKVAR, new_index);
		curstack->creator = NULL;

		jd->interface_map[0 * 5 + TYPE_ADR].flags = 0;
	}

	/* stack analysis loop (until fixpoint reached) **************************/

	do {
#if defined(ENABLE_STATISTICS)
		iteration_count++;
#endif

		/* initialize loop over basic blocks */

		b_count = jd->new_basicblockcount;
		sd.bptr = jd->new_basicblocks;
		superblockend = true;
		repeat = false;
		curstack = NULL; stackdepth = 0;
		deadcode = true;

		/* iterate over basic blocks *****************************************/

		while (--b_count >= 0) {
#if defined(STACK_VERBOSE)
			printf("----\nANALYZING BLOCK L%03d ", sd.bptr->nr);
			if (sd.bptr->type == BBTYPE_EXH) printf("EXH\n");
			else if (sd.bptr->type == BBTYPE_SBR) printf("SBR\n");
			else printf("STD\n");
			   
#endif

			if (sd.bptr->flags == BBDELETED) {
				/* This block has been deleted - do nothing. */
			}
			else if (superblockend && (sd.bptr->flags < BBREACHED)) {
				/* This block has not been reached so far, and we      */
				/* don't fall into it, so we'll have to iterate again. */
				repeat = true;
			}
			else if (sd.bptr->flags <= BBREACHED) {
				if (superblockend) {
					/* We know that sd.bptr->flags == BBREACHED. */
					/* This block has been reached before.    */
					stackdepth = sd.bptr->indepth;
				}
				else if (sd.bptr->flags < BBREACHED) {
					/* This block is reached for the first time now */
					/* by falling through from the previous block.  */
					/* Create the instack (propagated).             */
					COPYCURSTACK(sd, copy);
					sd.bptr->instack = copy;

					sd.bptr->invars = DMNEW(s4, stackdepth);
					for (i=stackdepth; i--; copy = copy->prev)
						sd.bptr->invars[i] = copy->varnum;
					sd.bptr->indepth = stackdepth;
				}
				else {
					/* This block has been reached before. now we are */
					/* falling into it from the previous block.       */
					/* Check that stack depth is well-defined.        */
					CHECK_STACK_DEPTH(sd.bptr->indepth, stackdepth);

					/* XXX check stack types? */
				}

				/* set up local variables for analyzing this block */

				curstack = sd.bptr->instack;
				deadcode = false;
				superblockend = false;
				len = sd.bptr->icount;
				iptr = sd.bptr->iinstr;
				b_index = sd.bptr - jd->new_basicblocks;

				/* mark the block as analysed */

				sd.bptr->flags = BBFINISHED;

				/* reset variables for dependency checking */

				coalescing_boundary = sd.new;
				for( i = 0; i < cd->maxlocals; i++)
					last_store_boundary[i] = sd.new;

				/* XXX store the start of the block's stack representation */

				sd.bptr->stack = sd.new;

#if defined(STACK_VERBOSE)
				printf("INVARS\n");
					for( copy = sd.bptr->instack; copy; copy = copy->prev ) {
						printf("%2d(%d", copy->varnum, copy->type);
						if (IS_OUTVAR(copy))
							printf("S");
						if (IS_PREALLOC(copy))
							printf("A");
						printf(") ");
					}
					printf("\n");

					printf("INVARS - indices:\t\n");
					for (i=0; i<sd.bptr->indepth; ++i) {
						printf("%d ", sd.bptr->invars[i]);
					}
					printf("\n\n");

#endif

				/* iterate over ICMDs ****************************************/

				while (--len >= 0)  {

#if defined(STACK_VERBOSE)
					new_show_icmd(jd, iptr, false, SHOW_PARSE); printf("\n");
					for( copy = curstack; copy; copy = copy->prev ) {
						printf("%2d(%d", copy->varnum, copy->type);
						if (IS_OUTVAR(copy))
							printf("S");
						if (IS_PREALLOC(copy))
							printf("A");
						printf(") ");
					}
					printf("\n");
#endif

					/* fetch the current opcode */

					opcode = iptr->opc;

					/* automatically replace some ICMDs with builtins */

#if defined(USEBUILTINTABLE)
					IF_NO_INTRP(
						bte = builtintable_get_automatic(opcode);

						if (bte && bte->opcode == opcode) {
							iptr->opc           = ICMD_BUILTIN;
							iptr->flags.bits    = 0;
							iptr->sx.s23.s3.bte = bte;
							/* iptr->line is already set */
							jd->isleafmethod = false;
							goto icmd_BUILTIN;
						}
					);
#endif /* defined(USEBUILTINTABLE) */

					/* main opcode switch *************************************/

					switch (opcode) {

						/* pop 0 push 0 */

					case ICMD_NOP:
icmd_NOP:
						CLR_SX;
						OP0_0;
						break;

					case ICMD_CHECKNULL:
						coalescing_boundary = sd.new;
						COUNT(count_check_null);
						USE_S1(TYPE_ADR);
						CLR_SX;
						CLR_DST; /* XXX live through? */
						break;

					case ICMD_RET:
						iptr->s1.varindex = 
							jd->local_map[iptr->s1.varindex * 5 + TYPE_ADR];
		
						USE_S1_LOCAL(TYPE_ADR);
						CLR_SX;
						CLR_DST;
#if 0
						IF_NO_INTRP( rd->locals[iptr->s1.localindex/*XXX invalid here*/][TYPE_ADR].type = TYPE_ADR; );
#endif
						superblockend = true;
						break;

					case ICMD_RETURN:
						COUNT(count_pcmd_return);
						CLR_SX;
						OP0_0;
						superblockend = true;
						break;


						/* pop 0 push 1 const */

	/************************** ICONST OPTIMIZATIONS **************************/

					case ICMD_ICONST:
						COUNT(count_pcmd_load);
						if (len == 0)
							goto normal_ICONST;

						switch (iptr[1].opc) {
							case ICMD_IADD:
								iptr->opc = ICMD_IADDCONST;
								/* FALLTHROUGH */

							icmd_iconst_tail:
								iptr[1].opc = ICMD_NOP;
								OP1_1(TYPE_INT, TYPE_INT);
								COUNT(count_pcmd_op);
								break;

							case ICMD_ISUB:
								iptr->opc = ICMD_ISUBCONST;
								goto icmd_iconst_tail;
#if SUPPORT_CONST_MUL
							case ICMD_IMUL:
								iptr->opc = ICMD_IMULCONST;
								goto icmd_iconst_tail;
#else /* SUPPORT_CONST_MUL */
							case ICMD_IMUL:
								if (iptr->sx.val.i == 0x00000002)
									iptr->sx.val.i = 1;
								else if (iptr->sx.val.i == 0x00000004)
									iptr->sx.val.i = 2;
								else if (iptr->sx.val.i == 0x00000008)
									iptr->sx.val.i = 3;
								else if (iptr->sx.val.i == 0x00000010)
									iptr->sx.val.i = 4;
								else if (iptr->sx.val.i == 0x00000020)
									iptr->sx.val.i = 5;
								else if (iptr->sx.val.i == 0x00000040)
									iptr->sx.val.i = 6;
								else if (iptr->sx.val.i == 0x00000080)
									iptr->sx.val.i = 7;
								else if (iptr->sx.val.i == 0x00000100)
									iptr->sx.val.i = 8;
								else if (iptr->sx.val.i == 0x00000200)
									iptr->sx.val.i = 9;
								else if (iptr->sx.val.i == 0x00000400)
									iptr->sx.val.i = 10;
								else if (iptr->sx.val.i == 0x00000800)
									iptr->sx.val.i = 11;
								else if (iptr->sx.val.i == 0x00001000)
									iptr->sx.val.i = 12;
								else if (iptr->sx.val.i == 0x00002000)
									iptr->sx.val.i = 13;
								else if (iptr->sx.val.i == 0x00004000)
									iptr->sx.val.i = 14;
								else if (iptr->sx.val.i == 0x00008000)
									iptr->sx.val.i = 15;
								else if (iptr->sx.val.i == 0x00010000)
									iptr->sx.val.i = 16;
								else if (iptr->sx.val.i == 0x00020000)
									iptr->sx.val.i = 17;
								else if (iptr->sx.val.i == 0x00040000)
									iptr->sx.val.i = 18;
								else if (iptr->sx.val.i == 0x00080000)
									iptr->sx.val.i = 19;
								else if (iptr->sx.val.i == 0x00100000)
									iptr->sx.val.i = 20;
								else if (iptr->sx.val.i == 0x00200000)
									iptr->sx.val.i = 21;
								else if (iptr->sx.val.i == 0x00400000)
									iptr->sx.val.i = 22;
								else if (iptr->sx.val.i == 0x00800000)
									iptr->sx.val.i = 23;
								else if (iptr->sx.val.i == 0x01000000)
									iptr->sx.val.i = 24;
								else if (iptr->sx.val.i == 0x02000000)
									iptr->sx.val.i = 25;
								else if (iptr->sx.val.i == 0x04000000)
									iptr->sx.val.i = 26;
								else if (iptr->sx.val.i == 0x08000000)
									iptr->sx.val.i = 27;
								else if (iptr->sx.val.i == 0x10000000)
									iptr->sx.val.i = 28;
								else if (iptr->sx.val.i == 0x20000000)
									iptr->sx.val.i = 29;
								else if (iptr->sx.val.i == 0x40000000)
									iptr->sx.val.i = 30;
								else if (iptr->sx.val.i == 0x80000000)
									iptr->sx.val.i = 31;
								else
									goto normal_ICONST;

								iptr->opc = ICMD_IMULPOW2;
								goto icmd_iconst_tail;
#endif /* SUPPORT_CONST_MUL */
							case ICMD_IDIV:
								if (iptr->sx.val.i == 0x00000002)
									iptr->sx.val.i = 1;
								else if (iptr->sx.val.i == 0x00000004)
									iptr->sx.val.i = 2;
								else if (iptr->sx.val.i == 0x00000008)
									iptr->sx.val.i = 3;
								else if (iptr->sx.val.i == 0x00000010)
									iptr->sx.val.i = 4;
								else if (iptr->sx.val.i == 0x00000020)
									iptr->sx.val.i = 5;
								else if (iptr->sx.val.i == 0x00000040)
									iptr->sx.val.i = 6;
								else if (iptr->sx.val.i == 0x00000080)
									iptr->sx.val.i = 7;
								else if (iptr->sx.val.i == 0x00000100)
									iptr->sx.val.i = 8;
								else if (iptr->sx.val.i == 0x00000200)
									iptr->sx.val.i = 9;
								else if (iptr->sx.val.i == 0x00000400)
									iptr->sx.val.i = 10;
								else if (iptr->sx.val.i == 0x00000800)
									iptr->sx.val.i = 11;
								else if (iptr->sx.val.i == 0x00001000)
									iptr->sx.val.i = 12;
								else if (iptr->sx.val.i == 0x00002000)
									iptr->sx.val.i = 13;
								else if (iptr->sx.val.i == 0x00004000)
									iptr->sx.val.i = 14;
								else if (iptr->sx.val.i == 0x00008000)
									iptr->sx.val.i = 15;
								else if (iptr->sx.val.i == 0x00010000)
									iptr->sx.val.i = 16;
								else if (iptr->sx.val.i == 0x00020000)
									iptr->sx.val.i = 17;
								else if (iptr->sx.val.i == 0x00040000)
									iptr->sx.val.i = 18;
								else if (iptr->sx.val.i == 0x00080000)
									iptr->sx.val.i = 19;
								else if (iptr->sx.val.i == 0x00100000)
									iptr->sx.val.i = 20;
								else if (iptr->sx.val.i == 0x00200000)
									iptr->sx.val.i = 21;
								else if (iptr->sx.val.i == 0x00400000)
									iptr->sx.val.i = 22;
								else if (iptr->sx.val.i == 0x00800000)
									iptr->sx.val.i = 23;
								else if (iptr->sx.val.i == 0x01000000)
									iptr->sx.val.i = 24;
								else if (iptr->sx.val.i == 0x02000000)
									iptr->sx.val.i = 25;
								else if (iptr->sx.val.i == 0x04000000)
									iptr->sx.val.i = 26;
								else if (iptr->sx.val.i == 0x08000000)
									iptr->sx.val.i = 27;
								else if (iptr->sx.val.i == 0x10000000)
									iptr->sx.val.i = 28;
								else if (iptr->sx.val.i == 0x20000000)
									iptr->sx.val.i = 29;
								else if (iptr->sx.val.i == 0x40000000)
									iptr->sx.val.i = 30;
								else if (iptr->sx.val.i == 0x80000000)
									iptr->sx.val.i = 31;
								else
									goto normal_ICONST;

								iptr->opc = ICMD_IDIVPOW2;
								goto icmd_iconst_tail;

							case ICMD_IREM:
								/*log_text("stack.c: ICMD_ICONST/ICMD_IREM");*/
								if ((iptr->sx.val.i == 0x00000002) ||
									(iptr->sx.val.i == 0x00000004) ||
									(iptr->sx.val.i == 0x00000008) ||
									(iptr->sx.val.i == 0x00000010) ||
									(iptr->sx.val.i == 0x00000020) ||
									(iptr->sx.val.i == 0x00000040) ||
									(iptr->sx.val.i == 0x00000080) ||
									(iptr->sx.val.i == 0x00000100) ||
									(iptr->sx.val.i == 0x00000200) ||
									(iptr->sx.val.i == 0x00000400) ||
									(iptr->sx.val.i == 0x00000800) ||
									(iptr->sx.val.i == 0x00001000) ||
									(iptr->sx.val.i == 0x00002000) ||
									(iptr->sx.val.i == 0x00004000) ||
									(iptr->sx.val.i == 0x00008000) ||
									(iptr->sx.val.i == 0x00010000) ||
									(iptr->sx.val.i == 0x00020000) ||
									(iptr->sx.val.i == 0x00040000) ||
									(iptr->sx.val.i == 0x00080000) ||
									(iptr->sx.val.i == 0x00100000) ||
									(iptr->sx.val.i == 0x00200000) ||
									(iptr->sx.val.i == 0x00400000) ||
									(iptr->sx.val.i == 0x00800000) ||
									(iptr->sx.val.i == 0x01000000) ||
									(iptr->sx.val.i == 0x02000000) ||
									(iptr->sx.val.i == 0x04000000) ||
									(iptr->sx.val.i == 0x08000000) ||
									(iptr->sx.val.i == 0x10000000) ||
									(iptr->sx.val.i == 0x20000000) ||
									(iptr->sx.val.i == 0x40000000) ||
									(iptr->sx.val.i == 0x80000000))
								{
									iptr->opc = ICMD_IREMPOW2;
									iptr->sx.val.i -= 1;
									goto icmd_iconst_tail;
								}
								goto normal_ICONST;
#if SUPPORT_CONST_LOGICAL
							case ICMD_IAND:
								iptr->opc = ICMD_IANDCONST;
								goto icmd_iconst_tail;

							case ICMD_IOR:
								iptr->opc = ICMD_IORCONST;
								goto icmd_iconst_tail;

							case ICMD_IXOR:
								iptr->opc = ICMD_IXORCONST;
								goto icmd_iconst_tail;

#endif /* SUPPORT_CONST_LOGICAL */
							case ICMD_ISHL:
								iptr->opc = ICMD_ISHLCONST;
								goto icmd_iconst_tail;

							case ICMD_ISHR:
								iptr->opc = ICMD_ISHRCONST;
								goto icmd_iconst_tail;

							case ICMD_IUSHR:
								iptr->opc = ICMD_IUSHRCONST;
								goto icmd_iconst_tail;
#if SUPPORT_LONG_SHIFT
							case ICMD_LSHL:
								iptr->opc = ICMD_LSHLCONST;
								goto icmd_lconst_tail;

							case ICMD_LSHR:
								iptr->opc = ICMD_LSHRCONST;
								goto icmd_lconst_tail;

							case ICMD_LUSHR:
								iptr->opc = ICMD_LUSHRCONST;
								goto icmd_lconst_tail;
#endif /* SUPPORT_LONG_SHIFT */
							case ICMD_IF_ICMPEQ:
								iptr[1].opc = ICMD_IFEQ;
								/* FALLTHROUGH */

							icmd_if_icmp_tail:
								/* set the constant for the following icmd */
								iptr[1].sx.val.i = iptr->sx.val.i;

								/* this instruction becomes a nop */
								iptr->opc = ICMD_NOP;
								goto icmd_NOP;

							case ICMD_IF_ICMPLT:
								iptr[1].opc = ICMD_IFLT;
								goto icmd_if_icmp_tail;

							case ICMD_IF_ICMPLE:
								iptr[1].opc = ICMD_IFLE;
								goto icmd_if_icmp_tail;

							case ICMD_IF_ICMPNE:
								iptr[1].opc = ICMD_IFNE;
								goto icmd_if_icmp_tail;

							case ICMD_IF_ICMPGT:
								iptr[1].opc = ICMD_IFGT;
								goto icmd_if_icmp_tail;

							case ICMD_IF_ICMPGE:
								iptr[1].opc = ICMD_IFGE;
								goto icmd_if_icmp_tail;

#if SUPPORT_CONST_STORE
							case ICMD_IASTORE:
							case ICMD_BASTORE:
							case ICMD_CASTORE:
							case ICMD_SASTORE:
								IF_INTRP( goto normal_ICONST; )
# if SUPPORT_CONST_STORE_ZERO_ONLY
								if (iptr->sx.val.i != 0)
									goto normal_ICONST;
# endif
								switch (iptr[1].opc) {
									case ICMD_IASTORE:
										iptr->opc = ICMD_IASTORECONST;
										iptr->flags.bits |= INS_FLAG_CHECK;
										break;
									case ICMD_BASTORE:
										iptr->opc = ICMD_BASTORECONST;
										iptr->flags.bits |= INS_FLAG_CHECK;
										break;
									case ICMD_CASTORE:
										iptr->opc = ICMD_CASTORECONST;
										iptr->flags.bits |= INS_FLAG_CHECK;
										break;
									case ICMD_SASTORE:
										iptr->opc = ICMD_SASTORECONST;
										iptr->flags.bits |= INS_FLAG_CHECK;
										break;
								}

								iptr[1].opc = ICMD_NOP;

								/* copy the constant to s3 */
								/* XXX constval -> astoreconstval? */
								iptr->sx.s23.s3.constval = iptr->sx.val.i;
								OP2_0(TYPE_ADR, TYPE_INT);
								COUNT(count_pcmd_op);
								break;

							case ICMD_PUTSTATIC:
							case ICMD_PUTFIELD:
								IF_INTRP( goto normal_ICONST; )
# if SUPPORT_CONST_STORE_ZERO_ONLY
								if (iptr->sx.val.i != 0)
									goto normal_ICONST;
# endif
								/* XXX check field type? */

								/* copy the constant to s2 */
								/* XXX constval -> fieldconstval? */
								iptr->sx.s23.s2.constval = iptr->sx.val.i;

putconst_tail:
								/* set the field reference (s3) */
								if (iptr[1].flags.bits & INS_FLAG_UNRESOLVED) {
									iptr->sx.s23.s3.uf = iptr[1].sx.s23.s3.uf;
									iptr->flags.bits |= INS_FLAG_UNRESOLVED;
								}
								else {
									iptr->sx.s23.s3.fmiref = iptr[1].sx.s23.s3.fmiref;
								}
								
								switch (iptr[1].opc) {
									case ICMD_PUTSTATIC:
										iptr->opc = ICMD_PUTSTATICCONST;
										OP0_0;
										break;
									case ICMD_PUTFIELD:
										iptr->opc = ICMD_PUTFIELDCONST;
										OP1_0(TYPE_ADR);
										break;
								}

								iptr[1].opc = ICMD_NOP;
								COUNT(count_pcmd_op);
								break;
#endif /* SUPPORT_CONST_STORE */

							default:
								goto normal_ICONST;
						}

						/* if we get here, the ICONST has been optimized */
						break;

normal_ICONST:
						/* normal case of an unoptimized ICONST */
						OP0_1(TYPE_INT);
						break;

	/************************** LCONST OPTIMIZATIONS **************************/

					case ICMD_LCONST:
						COUNT(count_pcmd_load);
						if (len == 0)
							goto normal_LCONST;

						/* switch depending on the following instruction */

						switch (iptr[1].opc) {
#if SUPPORT_LONG_ADD
							case ICMD_LADD:
								iptr->opc = ICMD_LADDCONST;
								/* FALLTHROUGH */

							icmd_lconst_tail:
								/* instruction of type LONG -> LONG */
								iptr[1].opc = ICMD_NOP;
								OP1_1(TYPE_LNG, TYPE_LNG);
								COUNT(count_pcmd_op);
								break;

							case ICMD_LSUB:
								iptr->opc = ICMD_LSUBCONST;
								goto icmd_lconst_tail;

#endif /* SUPPORT_LONG_ADD */
#if SUPPORT_LONG_MUL && SUPPORT_CONST_MUL
							case ICMD_LMUL:
								iptr->opc = ICMD_LMULCONST;
								goto icmd_lconst_tail;
#else /* SUPPORT_LONG_MUL && SUPPORT_CONST_MUL */
# if SUPPORT_LONG_SHIFT
							case ICMD_LMUL:
								if (iptr->sx.val.l == 0x00000002)
									iptr->sx.val.i = 1;
								else if (iptr->sx.val.l == 0x00000004)
									iptr->sx.val.i = 2;
								else if (iptr->sx.val.l == 0x00000008)
									iptr->sx.val.i = 3;
								else if (iptr->sx.val.l == 0x00000010)
									iptr->sx.val.i = 4;
								else if (iptr->sx.val.l == 0x00000020)
									iptr->sx.val.i = 5;
								else if (iptr->sx.val.l == 0x00000040)
									iptr->sx.val.i = 6;
								else if (iptr->sx.val.l == 0x00000080)
									iptr->sx.val.i = 7;
								else if (iptr->sx.val.l == 0x00000100)
									iptr->sx.val.i = 8;
								else if (iptr->sx.val.l == 0x00000200)
									iptr->sx.val.i = 9;
								else if (iptr->sx.val.l == 0x00000400)
									iptr->sx.val.i = 10;
								else if (iptr->sx.val.l == 0x00000800)
									iptr->sx.val.i = 11;
								else if (iptr->sx.val.l == 0x00001000)
									iptr->sx.val.i = 12;
								else if (iptr->sx.val.l == 0x00002000)
									iptr->sx.val.i = 13;
								else if (iptr->sx.val.l == 0x00004000)
									iptr->sx.val.i = 14;
								else if (iptr->sx.val.l == 0x00008000)
									iptr->sx.val.i = 15;
								else if (iptr->sx.val.l == 0x00010000)
									iptr->sx.val.i = 16;
								else if (iptr->sx.val.l == 0x00020000)
									iptr->sx.val.i = 17;
								else if (iptr->sx.val.l == 0x00040000)
									iptr->sx.val.i = 18;
								else if (iptr->sx.val.l == 0x00080000)
									iptr->sx.val.i = 19;
								else if (iptr->sx.val.l == 0x00100000)
									iptr->sx.val.i = 20;
								else if (iptr->sx.val.l == 0x00200000)
									iptr->sx.val.i = 21;
								else if (iptr->sx.val.l == 0x00400000)
									iptr->sx.val.i = 22;
								else if (iptr->sx.val.l == 0x00800000)
									iptr->sx.val.i = 23;
								else if (iptr->sx.val.l == 0x01000000)
									iptr->sx.val.i = 24;
								else if (iptr->sx.val.l == 0x02000000)
									iptr->sx.val.i = 25;
								else if (iptr->sx.val.l == 0x04000000)
									iptr->sx.val.i = 26;
								else if (iptr->sx.val.l == 0x08000000)
									iptr->sx.val.i = 27;
								else if (iptr->sx.val.l == 0x10000000)
									iptr->sx.val.i = 28;
								else if (iptr->sx.val.l == 0x20000000)
									iptr->sx.val.i = 29;
								else if (iptr->sx.val.l == 0x40000000)
									iptr->sx.val.i = 30;
								else if (iptr->sx.val.l == 0x80000000)
									iptr->sx.val.i = 31;
								else {
									goto normal_LCONST;
								}
								iptr->opc = ICMD_LMULPOW2;
								goto icmd_lconst_tail;
# endif /* SUPPORT_LONG_SHIFT */
#endif /* SUPPORT_LONG_MUL && SUPPORT_CONST_MUL */
#if SUPPORT_LONG_DIV_POW2
							case ICMD_LDIV:
								if (iptr->sx.val.l == 0x00000002)
									iptr->sx.val.i = 1;
								else if (iptr->sx.val.l == 0x00000004)
									iptr->sx.val.i = 2;
								else if (iptr->sx.val.l == 0x00000008)
									iptr->sx.val.i = 3;
								else if (iptr->sx.val.l == 0x00000010)
									iptr->sx.val.i = 4;
								else if (iptr->sx.val.l == 0x00000020)
									iptr->sx.val.i = 5;
								else if (iptr->sx.val.l == 0x00000040)
									iptr->sx.val.i = 6;
								else if (iptr->sx.val.l == 0x00000080)
									iptr->sx.val.i = 7;
								else if (iptr->sx.val.l == 0x00000100)
									iptr->sx.val.i = 8;
								else if (iptr->sx.val.l == 0x00000200)
									iptr->sx.val.i = 9;
								else if (iptr->sx.val.l == 0x00000400)
									iptr->sx.val.i = 10;
								else if (iptr->sx.val.l == 0x00000800)
									iptr->sx.val.i = 11;
								else if (iptr->sx.val.l == 0x00001000)
									iptr->sx.val.i = 12;
								else if (iptr->sx.val.l == 0x00002000)
									iptr->sx.val.i = 13;
								else if (iptr->sx.val.l == 0x00004000)
									iptr->sx.val.i = 14;
								else if (iptr->sx.val.l == 0x00008000)
									iptr->sx.val.i = 15;
								else if (iptr->sx.val.l == 0x00010000)
									iptr->sx.val.i = 16;
								else if (iptr->sx.val.l == 0x00020000)
									iptr->sx.val.i = 17;
								else if (iptr->sx.val.l == 0x00040000)
									iptr->sx.val.i = 18;
								else if (iptr->sx.val.l == 0x00080000)
									iptr->sx.val.i = 19;
								else if (iptr->sx.val.l == 0x00100000)
									iptr->sx.val.i = 20;
								else if (iptr->sx.val.l == 0x00200000)
									iptr->sx.val.i = 21;
								else if (iptr->sx.val.l == 0x00400000)
									iptr->sx.val.i = 22;
								else if (iptr->sx.val.l == 0x00800000)
									iptr->sx.val.i = 23;
								else if (iptr->sx.val.l == 0x01000000)
									iptr->sx.val.i = 24;
								else if (iptr->sx.val.l == 0x02000000)
									iptr->sx.val.i = 25;
								else if (iptr->sx.val.l == 0x04000000)
									iptr->sx.val.i = 26;
								else if (iptr->sx.val.l == 0x08000000)
									iptr->sx.val.i = 27;
								else if (iptr->sx.val.l == 0x10000000)
									iptr->sx.val.i = 28;
								else if (iptr->sx.val.l == 0x20000000)
									iptr->sx.val.i = 29;
								else if (iptr->sx.val.l == 0x40000000)
									iptr->sx.val.i = 30;
								else if (iptr->sx.val.l == 0x80000000)
									iptr->sx.val.i = 31;
								else {
									goto normal_LCONST;
								}
								iptr->opc = ICMD_LDIVPOW2;
								goto icmd_lconst_tail;
#endif /* SUPPORT_LONG_DIV_POW2 */

#if SUPPORT_LONG_REM_POW2
							case ICMD_LREM:
								if ((iptr->sx.val.l == 0x00000002) ||
									(iptr->sx.val.l == 0x00000004) ||
									(iptr->sx.val.l == 0x00000008) ||
									(iptr->sx.val.l == 0x00000010) ||
									(iptr->sx.val.l == 0x00000020) ||
									(iptr->sx.val.l == 0x00000040) ||
									(iptr->sx.val.l == 0x00000080) ||
									(iptr->sx.val.l == 0x00000100) ||
									(iptr->sx.val.l == 0x00000200) ||
									(iptr->sx.val.l == 0x00000400) ||
									(iptr->sx.val.l == 0x00000800) ||
									(iptr->sx.val.l == 0x00001000) ||
									(iptr->sx.val.l == 0x00002000) ||
									(iptr->sx.val.l == 0x00004000) ||
									(iptr->sx.val.l == 0x00008000) ||
									(iptr->sx.val.l == 0x00010000) ||
									(iptr->sx.val.l == 0x00020000) ||
									(iptr->sx.val.l == 0x00040000) ||
									(iptr->sx.val.l == 0x00080000) ||
									(iptr->sx.val.l == 0x00100000) ||
									(iptr->sx.val.l == 0x00200000) ||
									(iptr->sx.val.l == 0x00400000) ||
									(iptr->sx.val.l == 0x00800000) ||
									(iptr->sx.val.l == 0x01000000) ||
									(iptr->sx.val.l == 0x02000000) ||
									(iptr->sx.val.l == 0x04000000) ||
									(iptr->sx.val.l == 0x08000000) ||
									(iptr->sx.val.l == 0x10000000) ||
									(iptr->sx.val.l == 0x20000000) ||
									(iptr->sx.val.l == 0x40000000) ||
									(iptr->sx.val.l == 0x80000000))
								{
									iptr->opc = ICMD_LREMPOW2;
									iptr->sx.val.l -= 1;
									goto icmd_lconst_tail;
								}
								goto normal_LCONST;
#endif /* SUPPORT_LONG_REM_POW2 */

#if SUPPORT_LONG_LOGICAL && SUPPORT_CONST_LOGICAL

							case ICMD_LAND:
								iptr->opc = ICMD_LANDCONST;
								goto icmd_lconst_tail;

							case ICMD_LOR:
								iptr->opc = ICMD_LORCONST;
								goto icmd_lconst_tail;

							case ICMD_LXOR:
								iptr->opc = ICMD_LXORCONST;
								goto icmd_lconst_tail;
#endif /* SUPPORT_LONG_LOGICAL && SUPPORT_CONST_LOGICAL */

#if SUPPORT_LONG_CMP_CONST
							case ICMD_LCMP:
								if ((len <= 1) || (iptr[2].sx.val.i != 0))
									goto normal_LCONST;

								/* switch on the instruction after LCONST - LCMP */

								switch (iptr[2].opc) {
									case ICMD_IFEQ:
										iptr->opc = ICMD_IF_LEQ;
										/* FALLTHROUGH */

									icmd_lconst_lcmp_tail:
										/* convert LCONST, LCMP, IFXX to IF_LXX */
										iptr->dst.insindex = iptr[2].dst.insindex;
										iptr[1].opc = ICMD_NOP;
										iptr[2].opc = ICMD_NOP;

										OP1_BRANCH(TYPE_LNG);
										BRANCH(tbptr, copy);
										COUNT(count_pcmd_bra);
										COUNT(count_pcmd_op);
										break;

									case ICMD_IFNE:
										iptr->opc = ICMD_IF_LNE;
										goto icmd_lconst_lcmp_tail;

									case ICMD_IFLT:
										iptr->opc = ICMD_IF_LLT;
										goto icmd_lconst_lcmp_tail;

									case ICMD_IFGT:
										iptr->opc = ICMD_IF_LGT;
										goto icmd_lconst_lcmp_tail;

									case ICMD_IFLE:
										iptr->opc = ICMD_IF_LLE;
										goto icmd_lconst_lcmp_tail;

									case ICMD_IFGE:
										iptr->opc = ICMD_IF_LGE;
										goto icmd_lconst_lcmp_tail;

									default:
										goto normal_LCONST;
								} /* end switch on opcode after LCONST - LCMP */
								break;
#endif /* SUPPORT_LONG_CMP_CONST */

#if SUPPORT_CONST_STORE
							case ICMD_LASTORE:
								IF_INTRP( goto normal_LCONST; )
# if SUPPORT_CONST_STORE_ZERO_ONLY
								if (iptr->sx.val.l != 0)
									goto normal_LCONST;
# endif
#if SIZEOF_VOID_P == 4
								/* the constant must fit into a ptrint */
								if (iptr->sx.val.l < -0x80000000L || iptr->sx.val.l >= 0x80000000L)
									goto normal_LCONST;
#endif
								/* move the constant to s3 */
								iptr->sx.s23.s3.constval = iptr->sx.val.l;

								iptr->opc = ICMD_LASTORECONST;
								iptr->flags.bits |= INS_FLAG_CHECK;
								OP2_0(TYPE_ADR, TYPE_INT);

								iptr[1].opc = ICMD_NOP;
								COUNT(count_pcmd_op);
								break;

							case ICMD_PUTSTATIC:
							case ICMD_PUTFIELD:
								IF_INTRP( goto normal_LCONST; )
# if SUPPORT_CONST_STORE_ZERO_ONLY
								if (iptr->sx.val.l != 0)
									goto normal_LCONST;
# endif
#if SIZEOF_VOID_P == 4
								/* the constant must fit into a ptrint */
								if (iptr->sx.val.l < -0x80000000L || iptr->sx.val.l >= 0x80000000L)
									goto normal_LCONST;
#endif
								/* XXX check field type? */

								/* copy the constant to s2 */
								/* XXX constval -> fieldconstval? */
								iptr->sx.s23.s2.constval = iptr->sx.val.l;

								goto putconst_tail;

#endif /* SUPPORT_CONST_STORE */

							default:
								goto normal_LCONST;
						} /* end switch opcode after LCONST */

						/* if we get here, the LCONST has been optimized */
						break;

normal_LCONST:
						/* the normal case of an unoptimized LCONST */
						OP0_1(TYPE_LNG);
						break;

	/************************ END OF LCONST OPTIMIZATIONS *********************/

					case ICMD_FCONST:
						COUNT(count_pcmd_load);
						OP0_1(TYPE_FLT);
						break;

					case ICMD_DCONST:
						COUNT(count_pcmd_load);
						OP0_1(TYPE_DBL);
						break;

	/************************** ACONST OPTIMIZATIONS **************************/

					case ICMD_ACONST:
						coalescing_boundary = sd.new;
						COUNT(count_pcmd_load);
#if SUPPORT_CONST_STORE
						IF_INTRP( goto normal_ACONST; )

						/* We can only optimize if the ACONST is resolved
						 * and there is an instruction after it. */

						if ((len == 0) || (iptr->flags.bits & INS_FLAG_UNRESOLVED))
							goto normal_ACONST;

						switch (iptr[1].opc) {
							case ICMD_AASTORE:
								/* We can only optimize for NULL values
								 * here because otherwise a checkcast is
								 * required. */
								if (iptr->sx.val.anyptr != NULL)
									goto normal_ACONST;

								/* copy the constant (NULL) to s3 */
								iptr->sx.s23.s3.constval = 0;
								iptr->opc = ICMD_AASTORECONST;
								iptr->flags.bits |= INS_FLAG_CHECK;
								OP2_0(TYPE_ADR, TYPE_INT);

								iptr[1].opc = ICMD_NOP;
								COUNT(count_pcmd_op);
								break;

							case ICMD_PUTSTATIC:
							case ICMD_PUTFIELD:
# if SUPPORT_CONST_STORE_ZERO_ONLY
								if (iptr->sx.val.anyptr != NULL)
									goto normal_ACONST;
# endif
								/* XXX check field type? */
								/* copy the constant to s2 */
								/* XXX constval -> fieldconstval? */
								iptr->sx.s23.s2.constval = (ptrint) iptr->sx.val.anyptr;

								goto putconst_tail;

							default:
								goto normal_ACONST;
						}

						/* if we get here the ACONST has been optimized */
						break;

normal_ACONST:
#endif /* SUPPORT_CONST_STORE */
						OP0_1(TYPE_ADR);
						break;


						/* pop 0 push 1 load */

					case ICMD_ILOAD:
					case ICMD_LLOAD:
					case ICMD_FLOAD:
					case ICMD_DLOAD:
					case ICMD_ALOAD:
						COUNT(count_load_instruction);
						i = opcode - ICMD_ILOAD; /* type */

						iptr->s1.varindex = 
							jd->local_map[iptr->s1.varindex * 5 + i];
		
						LOAD(i, iptr->s1.varindex);
						break;

						/* pop 2 push 1 */

					case ICMD_LALOAD:
					case ICMD_FALOAD:
					case ICMD_DALOAD:
					case ICMD_AALOAD:
						coalescing_boundary = sd.new;
						iptr->flags.bits |= INS_FLAG_CHECK;
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);
						OP2_1(TYPE_ADR, TYPE_INT, opcode - ICMD_IALOAD);
						break;

					case ICMD_IALOAD:
					case ICMD_BALOAD:
					case ICMD_CALOAD:
					case ICMD_SALOAD:
						coalescing_boundary = sd.new;
						iptr->flags.bits |= INS_FLAG_CHECK;
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);
						OP2_1(TYPE_ADR, TYPE_INT, TYPE_INT);
						break;

						/* pop 0 push 0 iinc */

					case ICMD_IINC:
						STATISTICS_STACKDEPTH_DISTRIBUTION(count_store_depth);

						last_store_boundary[iptr->s1.varindex] = sd.new;

						iptr->s1.varindex = 
							jd->local_map[iptr->s1.varindex * 5 + TYPE_INT];

						copy = curstack;
						i = stackdepth - 1;
						while (copy) {
							if ((copy->varkind == LOCALVAR) &&
								(copy->varnum == iptr->s1.varindex))
							{
								assert(IS_LOCALVAR(copy));
								SET_TEMPVAR(copy);
							}
							i--;
							copy = copy->prev;
						}

						iptr->dst.varindex = iptr->s1.varindex;
						break;

						/* pop 1 push 0 store */

					case ICMD_ISTORE:
					case ICMD_LSTORE:
					case ICMD_FSTORE:
					case ICMD_DSTORE:
					case ICMD_ASTORE:
						REQUIRE_1;

						i = opcode - ICMD_ISTORE; /* type */
						javaindex = iptr->dst.varindex;
						j = iptr->dst.varindex = 
							jd->local_map[javaindex * 5 + i];


#if defined(ENABLE_STATISTICS)
						if (opt_stat) {
							count_pcmd_store++;
							i = sd.new - curstack;
							if (i >= 20)
								count_store_length[20]++;
							else
								count_store_length[i]++;
							i = stackdepth - 1;
							if (i >= 10)
								count_store_depth[10]++;
							else
								count_store_depth[i]++;
						}
#endif
						/* check for conflicts as described in Figure 5.2 */

						copy = curstack->prev;
						i = stackdepth - 2;
						while (copy) {
							if ((copy->varkind == LOCALVAR) &&
								(copy->varnum == j))
							{
								copy->varkind = TEMPVAR;
								assert(IS_LOCALVAR(copy));
								SET_TEMPVAR(copy);
							}
							i--;
							copy = copy->prev;
						}

						/* if the variable is already coalesced, don't bother */

						if (IS_OUTVAR(curstack)
							|| (curstack->varkind == LOCALVAR 
								&& curstack->varnum != j))
							goto store_tail;

						/* there is no STORE Lj while curstack is live */

						if (curstack < last_store_boundary[javaindex])
							goto assume_conflict;

						/* curstack must be after the coalescing boundary */

						if (curstack < coalescing_boundary)
							goto assume_conflict;

						/* there is no DEF LOCALVAR(j) while curstack is live */

						copy = sd.new; /* most recent stackslot created + 1 */
						while (--copy > curstack) {
							if (copy->varkind == LOCALVAR && copy->varnum == j)
								goto assume_conflict;
						}

						/* coalesce the temporary variable with Lj */
						assert( (CURKIND == TEMPVAR) || (CURKIND == UNDEFVAR));
						assert(!IS_LOCALVAR(curstack));
						assert(!IS_OUTVAR(curstack));
						assert(!IS_PREALLOC(curstack));

						assert(curstack->creator);
						assert(curstack->creator->dst.varindex == curstack->varnum);
						RELEASE_INDEX(sd, curstack);
						curstack->varkind = LOCALVAR;
						curstack->varnum = j;
						curstack->creator->dst.varindex = j;
						goto store_tail;

						/* revert the coalescing, if it has been done earlier */
assume_conflict:
						if ((curstack->varkind == LOCALVAR)
							&& (curstack->varnum == j))
						{
							assert(IS_LOCALVAR(curstack));
							SET_TEMPVAR(curstack);
						}

						/* remember the stack boundary at this store */
store_tail:
						last_store_boundary[javaindex] = sd.new;

						STORE(opcode - ICMD_ISTORE, j);
						break;

					/* pop 3 push 0 */

					case ICMD_AASTORE:
						coalescing_boundary = sd.new;
						iptr->flags.bits |= INS_FLAG_CHECK;
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);

						bte = builtintable_get_internal(BUILTIN_canstore);
						md = bte->md;

						if (md->memuse > rd->memuse)
							rd->memuse = md->memuse;
						if (md->argintreguse > rd->argintreguse)
							rd->argintreguse = md->argintreguse;
						/* XXX non-leaf method? */

						/* make all stack variables saved */

						copy = curstack;
						while (copy) {
							sd.var[copy->varnum].flags |= SAVEDVAR;
							/* in case copy->varnum is/will be a LOCALVAR */
							/* once and set back to a non LOCALVAR        */
							/* the correct SAVEDVAR flag has to be        */
							/* remembered in copy->flags, too             */
							copy->flags |= SAVEDVAR;
							copy = copy->prev;
						}

						OP3_0(TYPE_ADR, TYPE_INT, TYPE_ADR);
						break;


					case ICMD_LASTORE:
					case ICMD_FASTORE:
					case ICMD_DASTORE:
						coalescing_boundary = sd.new;
						iptr->flags.bits |= INS_FLAG_CHECK;
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);
						OP3_0(TYPE_ADR, TYPE_INT, opcode - ICMD_IASTORE);
						break;

					case ICMD_IASTORE:
					case ICMD_BASTORE:
					case ICMD_CASTORE:
					case ICMD_SASTORE:
						coalescing_boundary = sd.new;
						iptr->flags.bits |= INS_FLAG_CHECK;
						COUNT(count_check_null);
						COUNT(count_check_bound);
						COUNT(count_pcmd_mem);
						OP3_0(TYPE_ADR, TYPE_INT, TYPE_INT);
						break;

						/* pop 1 push 0 */

					case ICMD_POP:
#ifdef ENABLE_VERIFIER
						if (opt_verify) {
							REQUIRE_1;
							if (IS_2_WORD_TYPE(curstack->type))
								goto throw_stack_category_error;
						}
#endif
						OP1_0_ANY;
						break;

					case ICMD_IRETURN:
					case ICMD_LRETURN:
					case ICMD_FRETURN:
					case ICMD_DRETURN:
					case ICMD_ARETURN:
						coalescing_boundary = sd.new;
						IF_JIT( md_return_alloc(jd, curstack); )
						COUNT(count_pcmd_return);
						OP1_0(opcode - ICMD_IRETURN);
						superblockend = true;
						break;

					case ICMD_ATHROW:
						coalescing_boundary = sd.new;
						COUNT(count_check_null);
						OP1_0(TYPE_ADR);
						curstack = NULL; stackdepth = 0;
						superblockend = true;
						break;

					case ICMD_PUTSTATIC:
						coalescing_boundary = sd.new;
						COUNT(count_pcmd_mem);
						INSTRUCTION_GET_FIELDREF(iptr, fmiref);
						OP1_0(fmiref->parseddesc.fd->type);
						break;

						/* pop 1 push 0 branch */

					case ICMD_IFNULL:
					case ICMD_IFNONNULL:
						COUNT(count_pcmd_bra);
						OP1_BRANCH(TYPE_ADR);
						BRANCH(tbptr, copy);
						break;

					case ICMD_IFEQ:
					case ICMD_IFNE:
					case ICMD_IFLT:
					case ICMD_IFGE:
					case ICMD_IFGT:
					case ICMD_IFLE:
						COUNT(count_pcmd_bra);
						/* iptr->sx.val.i is set implicitly in parse by
						   clearing the memory or from IF_ICMPxx
						   optimization. */

						OP1_BRANCH(TYPE_INT);
/* 						iptr->sx.val.i = 0; */
						BRANCH(tbptr, copy);
						break;

						/* pop 0 push 0 branch */

					case ICMD_GOTO:
						COUNT(count_pcmd_bra);
						OP0_BRANCH;
						BRANCH(tbptr, copy);
						superblockend = true;
						break;

						/* pop 1 push 0 table branch */

					case ICMD_TABLESWITCH:
						COUNT(count_pcmd_table);
						OP1_BRANCH(TYPE_INT);

						table = iptr->dst.table;
						BRANCH_TARGET(*table, tbptr, copy);
						table++;

						i = iptr->sx.s23.s3.tablehigh
						  - iptr->sx.s23.s2.tablelow + 1;

						while (--i >= 0) {
							BRANCH_TARGET(*table, tbptr, copy);
							table++;
						}
						superblockend = true;
						break;

						/* pop 1 push 0 table branch */

					case ICMD_LOOKUPSWITCH:
						COUNT(count_pcmd_table);
						OP1_BRANCH(TYPE_INT);

						BRANCH_TARGET(iptr->sx.s23.s3.lookupdefault, tbptr, copy);

						lookup = iptr->dst.lookup;

						i = iptr->sx.s23.s2.lookupcount;

						while (--i >= 0) {
							BRANCH_TARGET(lookup->target, tbptr, copy);
							lookup++;
						}
						superblockend = true;
						break;

					case ICMD_MONITORENTER:
					case ICMD_MONITOREXIT:
						coalescing_boundary = sd.new;
						COUNT(count_check_null);
						OP1_0(TYPE_ADR);
						break;

						/* pop 2 push 0 branch */

					case ICMD_IF_ICMPEQ:
					case ICMD_IF_ICMPNE:
					case ICMD_IF_ICMPLT:
					case ICMD_IF_ICMPGE:
					case ICMD_IF_ICMPGT:
					case ICMD_IF_ICMPLE:
						COUNT(count_pcmd_bra);
						OP2_BRANCH(TYPE_INT, TYPE_INT);
						BRANCH(tbptr, copy);
						break;

					case ICMD_IF_ACMPEQ:
					case ICMD_IF_ACMPNE:
						COUNT(count_pcmd_bra);
						OP2_BRANCH(TYPE_ADR, TYPE_ADR);
						BRANCH(tbptr, copy);
						break;

						/* pop 2 push 0 */

					case ICMD_PUTFIELD:
						coalescing_boundary = sd.new;
						COUNT(count_check_null);
						COUNT(count_pcmd_mem);
						INSTRUCTION_GET_FIELDREF(iptr, fmiref);
						OP2_0(TYPE_ADR, fmiref->parseddesc.fd->type);
						break;

					case ICMD_POP2:
						REQUIRE_1;
						if (!IS_2_WORD_TYPE(curstack->type)) {
							/* ..., cat1 */
#ifdef ENABLE_VERIFIER
							if (opt_verify) {
								REQUIRE_2;
								if (IS_2_WORD_TYPE(curstack->prev->type))
									goto throw_stack_category_error;
							}
#endif
							OP2_0_ANY_ANY; /* pop two slots */
						}
						else {
							iptr->opc = ICMD_POP;
							OP1_0_ANY; /* pop one (two-word) slot */
						}
						break;

						/* pop 0 push 1 dup */

					case ICMD_DUP:
#ifdef ENABLE_VERIFIER
						if (opt_verify) {
							REQUIRE_1;
							if (IS_2_WORD_TYPE(curstack->type))
								goto throw_stack_category_error;
						}
#endif
						COUNT(count_dup_instruction);

icmd_DUP:
						src1 = curstack;

						COPY_UP(src1);
						coalescing_boundary = sd.new - 1;
						break;

					case ICMD_DUP2:
						REQUIRE_1;
						if (IS_2_WORD_TYPE(curstack->type)) {
							/* ..., cat2 */
							iptr->opc = ICMD_DUP;
							goto icmd_DUP;
						}
						else {
							REQUIRE_2;
							/* ..., ????, cat1 */
#ifdef ENABLE_VERIFIER
							if (opt_verify) {
								if (IS_2_WORD_TYPE(curstack->prev->type))
									goto throw_stack_category_error;
							}
#endif
							src1 = curstack->prev;
							src2 = curstack;

							COPY_UP(src1); iptr++; len--;
							COPY_UP(src2);

							coalescing_boundary = sd.new;
						}
						break;

						/* pop 2 push 3 dup */

					case ICMD_DUP_X1:
#ifdef ENABLE_VERIFIER
						if (opt_verify) {
							REQUIRE_2;
							if (IS_2_WORD_TYPE(curstack->type) ||
								IS_2_WORD_TYPE(curstack->prev->type))
									goto throw_stack_category_error;
						}
#endif

icmd_DUP_X1:
						src1 = curstack->prev;
						src2 = curstack;
						POPANY; POPANY;
						stackdepth -= 2;

						DUP_SLOT(src2); dst1 = curstack; stackdepth++;

						MOVE_UP(src1); iptr++; len--;
						MOVE_UP(src2); iptr++; len--;

						COPY_DOWN(curstack, dst1);

						coalescing_boundary = sd.new;
						break;

					case ICMD_DUP2_X1:
						REQUIRE_2;
						if (IS_2_WORD_TYPE(curstack->type)) {
							/* ..., ????, cat2 */
#ifdef ENABLE_VERIFIER
							if (opt_verify) {
								if (IS_2_WORD_TYPE(curstack->prev->type))
									goto throw_stack_category_error;
							}
#endif
							iptr->opc = ICMD_DUP_X1;
							goto icmd_DUP_X1;
						}
						else {
							/* ..., ????, cat1 */
#ifdef ENABLE_VERIFIER
							if (opt_verify) {
								REQUIRE_3;
								if (IS_2_WORD_TYPE(curstack->prev->type)
									|| IS_2_WORD_TYPE(curstack->prev->prev->type))
										goto throw_stack_category_error;
							}
#endif

icmd_DUP2_X1:
							src1 = curstack->prev->prev;
							src2 = curstack->prev;
							src3 = curstack;
							POPANY; POPANY; POPANY;
							stackdepth -= 3;

							DUP_SLOT(src2); dst1 = curstack; stackdepth++;
							DUP_SLOT(src3); dst2 = curstack; stackdepth++;

							MOVE_UP(src1); iptr++; len--;
							MOVE_UP(src2); iptr++; len--;
							MOVE_UP(src3); iptr++; len--;

							COPY_DOWN(curstack, dst2); iptr++; len--;
							COPY_DOWN(curstack->prev, dst1);

							coalescing_boundary = sd.new;
						}
						break;

						/* pop 3 push 4 dup */

					case ICMD_DUP_X2:
						REQUIRE_2;
						if (IS_2_WORD_TYPE(curstack->prev->type)) {
							/* ..., cat2, ???? */
#ifdef ENABLE_VERIFIER
							if (opt_verify) {
								if (IS_2_WORD_TYPE(curstack->type))
									goto throw_stack_category_error;
							}
#endif
							iptr->opc = ICMD_DUP_X1;
							goto icmd_DUP_X1;
						}
						else {
							/* ..., cat1, ???? */
#ifdef ENABLE_VERIFIER
							if (opt_verify) {
								REQUIRE_3;
								if (IS_2_WORD_TYPE(curstack->type)
									|| IS_2_WORD_TYPE(curstack->prev->prev->type))
											goto throw_stack_category_error;
							}
#endif
icmd_DUP_X2:
							src1 = curstack->prev->prev;
							src2 = curstack->prev;
							src3 = curstack;
							POPANY; POPANY; POPANY;
							stackdepth -= 3;

							DUP_SLOT(src3); dst1 = curstack; stackdepth++;

							MOVE_UP(src1); iptr++; len--;
							MOVE_UP(src2); iptr++; len--;
							MOVE_UP(src3); iptr++; len--;

							COPY_DOWN(curstack, dst1);

							coalescing_boundary = sd.new;
						}
						break;

					case ICMD_DUP2_X2:
						REQUIRE_2;
						if (IS_2_WORD_TYPE(curstack->type)) {
							/* ..., ????, cat2 */
							if (IS_2_WORD_TYPE(curstack->prev->type)) {
								/* ..., cat2, cat2 */
								iptr->opc = ICMD_DUP_X1;
								goto icmd_DUP_X1;
							}
							else {
								/* ..., cat1, cat2 */
#ifdef ENABLE_VERIFIER
								if (opt_verify) {
									REQUIRE_3;
									if (IS_2_WORD_TYPE(curstack->prev->prev->type))
											goto throw_stack_category_error;
								}
#endif
								iptr->opc = ICMD_DUP_X2;
								goto icmd_DUP_X2;
							}
						}

						REQUIRE_3;
						/* ..., ????, ????, cat1 */

						if (IS_2_WORD_TYPE(curstack->prev->prev->type)) {
							/* ..., cat2, ????, cat1 */
#ifdef ENABLE_VERIFIER
							if (opt_verify) {
								if (IS_2_WORD_TYPE(curstack->prev->type))
									goto throw_stack_category_error;
							}
#endif
							iptr->opc = ICMD_DUP2_X1;
							goto icmd_DUP2_X1;
						}
						else {
							/* ..., cat1, ????, cat1 */
#ifdef ENABLE_VERIFIER
							if (opt_verify) {
								REQUIRE_4;
								if (IS_2_WORD_TYPE(curstack->prev->type)
									|| IS_2_WORD_TYPE(curstack->prev->prev->prev->type))
									goto throw_stack_category_error;
							}
#endif

							src1 = curstack->prev->prev->prev;
							src2 = curstack->prev->prev;
							src3 = curstack->prev;
							src4 = curstack;
							POPANY; POPANY; POPANY; POPANY;
							stackdepth -= 4;

							DUP_SLOT(src3); dst1 = curstack; stackdepth++;
							DUP_SLOT(src4); dst2 = curstack; stackdepth++;

							MOVE_UP(src1); iptr++; len--;
							MOVE_UP(src2); iptr++; len--;
							MOVE_UP(src3); iptr++; len--;
							MOVE_UP(src4); iptr++; len--;

							COPY_DOWN(curstack, dst2); iptr++; len--;
							COPY_DOWN(curstack->prev, dst1);

							coalescing_boundary = sd.new;
						}
						break;

						/* pop 2 push 2 swap */

					case ICMD_SWAP:
#ifdef ENABLE_VERIFIER
						if (opt_verify) {
							REQUIRE_2;
							if (IS_2_WORD_TYPE(curstack->type)
								|| IS_2_WORD_TYPE(curstack->prev->type))
								goto throw_stack_category_error;
						}
#endif

						src1 = curstack->prev;
						src2 = curstack;
						POPANY; POPANY;
						stackdepth -= 2;

						MOVE_UP(src2); iptr++; len--;
						MOVE_UP(src1);

						coalescing_boundary = sd.new;
						break;

						/* pop 2 push 1 */

					case ICMD_IDIV:
					case ICMD_IREM:
						coalescing_boundary = sd.new;
#if !SUPPORT_DIVISION
						bte = iptr->sx.s23.s3.bte;
						md = bte->md;

						if (md->memuse > rd->memuse)
							rd->memuse = md->memuse;
						if (md->argintreguse > rd->argintreguse)
							rd->argintreguse = md->argintreguse;

						/* make all stack variables saved */

						copy = curstack;
						while (copy) {
							sd.var[copy->varnum].flags |= SAVEDVAR;
							copy->flags |= SAVEDVAR;
							copy = copy->prev;
						}
						/* FALLTHROUGH */

#endif /* !SUPPORT_DIVISION */

					case ICMD_ISHL:
					case ICMD_ISHR:
					case ICMD_IUSHR:
					case ICMD_IADD:
					case ICMD_ISUB:
					case ICMD_IMUL:
					case ICMD_IAND:
					case ICMD_IOR:
					case ICMD_IXOR:
						COUNT(count_pcmd_op);
						OP2_1(TYPE_INT, TYPE_INT, TYPE_INT);
						break;

					case ICMD_LDIV:
					case ICMD_LREM:
						coalescing_boundary = sd.new;
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
						bte = iptr->sx.s23.s3.bte;
						md = bte->md;

						if (md->memuse > rd->memuse)
							rd->memuse = md->memuse;
						if (md->argintreguse > rd->argintreguse)
							rd->argintreguse = md->argintreguse;
						/* XXX non-leaf method? */

						/* make all stack variables saved */

						copy = curstack;
						while (copy) {
							sd.var[copy->varnum].flags |= SAVEDVAR;
							copy->flags |= SAVEDVAR;
							copy = copy->prev;
						}
						/* FALLTHROUGH */

#endif /* !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV) */

					case ICMD_LMUL:
					case ICMD_LADD:
					case ICMD_LSUB:
#if SUPPORT_LONG_LOGICAL
					case ICMD_LAND:
					case ICMD_LOR:
					case ICMD_LXOR:
#endif /* SUPPORT_LONG_LOGICAL */
						COUNT(count_pcmd_op);
						OP2_1(TYPE_LNG, TYPE_LNG, TYPE_LNG);
						break;

					case ICMD_LSHL:
					case ICMD_LSHR:
					case ICMD_LUSHR:
						COUNT(count_pcmd_op);
						OP2_1(TYPE_LNG, TYPE_INT, TYPE_LNG);
						break;

					case ICMD_FADD:
					case ICMD_FSUB:
					case ICMD_FMUL:
					case ICMD_FDIV:
					case ICMD_FREM:
						COUNT(count_pcmd_op);
						OP2_1(TYPE_FLT, TYPE_FLT, TYPE_FLT);
						break;

					case ICMD_DADD:
					case ICMD_DSUB:
					case ICMD_DMUL:
					case ICMD_DDIV:
					case ICMD_DREM:
						COUNT(count_pcmd_op);
						OP2_1(TYPE_DBL, TYPE_DBL, TYPE_DBL);
						break;

					case ICMD_LCMP:
						COUNT(count_pcmd_op);
#if SUPPORT_LONG_CMP_CONST
						if ((len == 0) || (iptr[1].sx.val.i != 0))
							goto normal_LCMP;

						switch (iptr[1].opc) {
						case ICMD_IFEQ:
							iptr->opc = ICMD_IF_LCMPEQ;
						icmd_lcmp_if_tail:
							iptr->dst.insindex = iptr[1].dst.insindex;
							iptr[1].opc = ICMD_NOP;

							OP2_BRANCH(TYPE_LNG, TYPE_LNG);
							BRANCH(tbptr, copy);

							COUNT(count_pcmd_bra);
							break;
						case ICMD_IFNE:
							iptr->opc = ICMD_IF_LCMPNE;
							goto icmd_lcmp_if_tail;
						case ICMD_IFLT:
							iptr->opc = ICMD_IF_LCMPLT;
							goto icmd_lcmp_if_tail;
						case ICMD_IFGT:
							iptr->opc = ICMD_IF_LCMPGT;
							goto icmd_lcmp_if_tail;
						case ICMD_IFLE:
							iptr->opc = ICMD_IF_LCMPLE;
							goto icmd_lcmp_if_tail;
						case ICMD_IFGE:
							iptr->opc = ICMD_IF_LCMPGE;
							goto icmd_lcmp_if_tail;
						default:
							goto normal_LCMP;
						}
						break;
normal_LCMP:
#endif /* SUPPORT_LONG_CMP_CONST */
							OP2_1(TYPE_LNG, TYPE_LNG, TYPE_INT);
						break;

						/* XXX why is this deactivated? */
#if 0
					case ICMD_FCMPL:
						COUNT(count_pcmd_op);
						if ((len == 0) || (iptr[1].sx.val.i != 0))
							goto normal_FCMPL;

						switch (iptr[1].opc) {
						case ICMD_IFEQ:
							iptr->opc = ICMD_IF_FCMPEQ;
						icmd_if_fcmpl_tail:
							iptr->dst.insindex = iptr[1].dst.insindex;
							iptr[1].opc = ICMD_NOP;

							OP2_BRANCH(TYPE_FLT, TYPE_FLT);
							BRANCH(tbptr, copy);

							COUNT(count_pcmd_bra);
							break;
						case ICMD_IFNE:
							iptr->opc = ICMD_IF_FCMPNE;
							goto icmd_if_fcmpl_tail;
						case ICMD_IFLT:
							iptr->opc = ICMD_IF_FCMPL_LT;
							goto icmd_if_fcmpl_tail;
						case ICMD_IFGT:
							iptr->opc = ICMD_IF_FCMPL_GT;
							goto icmd_if_fcmpl_tail;
						case ICMD_IFLE:
							iptr->opc = ICMD_IF_FCMPL_LE;
							goto icmd_if_fcmpl_tail;
						case ICMD_IFGE:
							iptr->opc = ICMD_IF_FCMPL_GE;
							goto icmd_if_fcmpl_tail;
						default:
							goto normal_FCMPL;
						}
						break;

normal_FCMPL:
						OPTT2_1(TYPE_FLT, TYPE_FLT, TYPE_INT);
						break;

					case ICMD_FCMPG:
						COUNT(count_pcmd_op);
						if ((len == 0) || (iptr[1].sx.val.i != 0))
							goto normal_FCMPG;

						switch (iptr[1].opc) {
						case ICMD_IFEQ:
							iptr->opc = ICMD_IF_FCMPEQ;
						icmd_if_fcmpg_tail:
							iptr->dst.insindex = iptr[1].dst.insindex;
							iptr[1].opc = ICMD_NOP;

							OP2_BRANCH(TYPE_FLT, TYPE_FLT);
							BRANCH(tbptr, copy);

							COUNT(count_pcmd_bra);
							break;
						case ICMD_IFNE:
							iptr->opc = ICMD_IF_FCMPNE;
							goto icmd_if_fcmpg_tail;
						case ICMD_IFLT:
							iptr->opc = ICMD_IF_FCMPG_LT;
							goto icmd_if_fcmpg_tail;
						case ICMD_IFGT:
							iptr->opc = ICMD_IF_FCMPG_GT;
							goto icmd_if_fcmpg_tail;
						case ICMD_IFLE:
							iptr->opc = ICMD_IF_FCMPG_LE;
							goto icmd_if_fcmpg_tail;
						case ICMD_IFGE:
							iptr->opc = ICMD_IF_FCMPG_GE;
							goto icmd_if_fcmpg_tail;
						default:
							goto normal_FCMPG;
						}
						break;

normal_FCMPG:
						OP2_1(TYPE_FLT, TYPE_FLT, TYPE_INT);
						break;

					case ICMD_DCMPL:
						COUNT(count_pcmd_op);
						if ((len == 0) || (iptr[1].sx.val.i != 0))
							goto normal_DCMPL;

						switch (iptr[1].opc) {
						case ICMD_IFEQ:
							iptr->opc = ICMD_IF_DCMPEQ;
						icmd_if_dcmpl_tail:
							iptr->dst.insindex = iptr[1].dst.insindex;
							iptr[1].opc = ICMD_NOP;

							OP2_BRANCH(TYPE_DBL, TYPE_DBL);
							BRANCH(tbptr, copy);

							COUNT(count_pcmd_bra);
							break;
						case ICMD_IFNE:
							iptr->opc = ICMD_IF_DCMPNE;
							goto icmd_if_dcmpl_tail;
						case ICMD_IFLT:
							iptr->opc = ICMD_IF_DCMPL_LT;
							goto icmd_if_dcmpl_tail;
						case ICMD_IFGT:
							iptr->opc = ICMD_IF_DCMPL_GT;
							goto icmd_if_dcmpl_tail;
						case ICMD_IFLE:
							iptr->opc = ICMD_IF_DCMPL_LE;
							goto icmd_if_dcmpl_tail;
						case ICMD_IFGE:
							iptr->opc = ICMD_IF_DCMPL_GE;
							goto icmd_if_dcmpl_tail;
						default:
							goto normal_DCMPL;
						}
						break;

normal_DCMPL:
						OPTT2_1(TYPE_DBL, TYPE_INT);
						break;

					case ICMD_DCMPG:
						COUNT(count_pcmd_op);
						if ((len == 0) || (iptr[1].sx.val.i != 0))
							goto normal_DCMPG;

						switch (iptr[1].opc) {
						case ICMD_IFEQ:
							iptr->opc = ICMD_IF_DCMPEQ;
						icmd_if_dcmpg_tail:
							iptr->dst.insindex = iptr[1].dst.insindex;
							iptr[1].opc = ICMD_NOP;

							OP2_BRANCH(TYPE_DBL, TYPE_DBL);
							BRANCH(tbptr, copy);

							COUNT(count_pcmd_bra);
							break;
						case ICMD_IFNE:
							iptr->opc = ICMD_IF_DCMPNE;
							goto icmd_if_dcmpg_tail;
						case ICMD_IFLT:
							iptr->opc = ICMD_IF_DCMPG_LT;
							goto icmd_if_dcmpg_tail;
						case ICMD_IFGT:
							iptr->opc = ICMD_IF_DCMPG_GT;
							goto icmd_if_dcmpg_tail;
						case ICMD_IFLE:
							iptr->opc = ICMD_IF_DCMPG_LE;
							goto icmd_if_dcmpg_tail;
						case ICMD_IFGE:
							iptr->opc = ICMD_IF_DCMPG_GE;
							goto icmd_if_dcmpg_tail;
						default:
							goto normal_DCMPG;
						}
						break;

normal_DCMPG:
						OP2_1(TYPE_DBL, TYPE_DBL, TYPE_INT);
						break;
#else
					case ICMD_FCMPL:
					case ICMD_FCMPG:
						COUNT(count_pcmd_op);
						OP2_1(TYPE_FLT, TYPE_FLT, TYPE_INT);
						break;

					case ICMD_DCMPL:
					case ICMD_DCMPG:
						COUNT(count_pcmd_op);
						OP2_1(TYPE_DBL, TYPE_DBL, TYPE_INT);
						break;
#endif

						/* pop 1 push 1 */

					case ICMD_INEG:
					case ICMD_INT2BYTE:
					case ICMD_INT2CHAR:
					case ICMD_INT2SHORT:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_INT, TYPE_INT);
						break;
					case ICMD_LNEG:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_LNG, TYPE_LNG);
						break;
					case ICMD_FNEG:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_FLT, TYPE_FLT);
						break;
					case ICMD_DNEG:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_DBL, TYPE_DBL);
						break;

					case ICMD_I2L:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_INT, TYPE_LNG);
						break;
					case ICMD_I2F:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_INT, TYPE_FLT);
						break;
					case ICMD_I2D:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_INT, TYPE_DBL);
						break;
					case ICMD_L2I:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_LNG, TYPE_INT);
						break;
					case ICMD_L2F:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_LNG, TYPE_FLT);
						break;
					case ICMD_L2D:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_LNG, TYPE_DBL);
						break;
					case ICMD_F2I:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_FLT, TYPE_INT);
						break;
					case ICMD_F2L:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_FLT, TYPE_LNG);
						break;
					case ICMD_F2D:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_FLT, TYPE_DBL);
						break;
					case ICMD_D2I:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_DBL, TYPE_INT);
						break;
					case ICMD_D2L:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_DBL, TYPE_LNG);
						break;
					case ICMD_D2F:
						COUNT(count_pcmd_op);
						OP1_1(TYPE_DBL, TYPE_FLT);
						break;

					case ICMD_CHECKCAST:
						coalescing_boundary = sd.new;
						if (iptr->flags.bits & INS_FLAG_ARRAY) {
							/* array type cast-check */

							bte = builtintable_get_internal(BUILTIN_arraycheckcast);
							md = bte->md;

							if (md->memuse > rd->memuse)
								rd->memuse = md->memuse;
							if (md->argintreguse > rd->argintreguse)
								rd->argintreguse = md->argintreguse;

							/* make all stack variables saved */

							copy = curstack;
							while (copy) {
								sd.var[copy->varnum].flags |= SAVEDVAR;
								copy->flags |= SAVEDVAR;
								copy = copy->prev;
							}
						}
						OP1_1(TYPE_ADR, TYPE_ADR);
						break;

					case ICMD_INSTANCEOF:
					case ICMD_ARRAYLENGTH:
						coalescing_boundary = sd.new;
						OP1_1(TYPE_ADR, TYPE_INT);
						break;

					case ICMD_NEWARRAY:
					case ICMD_ANEWARRAY:
						coalescing_boundary = sd.new;
						OP1_1(TYPE_INT, TYPE_ADR);
						break;

					case ICMD_GETFIELD:
						coalescing_boundary = sd.new;
						COUNT(count_check_null);
						COUNT(count_pcmd_mem);
						INSTRUCTION_GET_FIELDREF(iptr, fmiref);
						OP1_1(TYPE_ADR, fmiref->parseddesc.fd->type);
						break;

						/* pop 0 push 1 */

					case ICMD_GETSTATIC:
 						coalescing_boundary = sd.new;
						COUNT(count_pcmd_mem);
						INSTRUCTION_GET_FIELDREF(iptr, fmiref);
						OP0_1(fmiref->parseddesc.fd->type);
						break;

					case ICMD_NEW:
 						coalescing_boundary = sd.new;
						OP0_1(TYPE_ADR);
						break;

					case ICMD_JSR:
						OP0_1(TYPE_ADR);

						BRANCH_TARGET(iptr->sx.s23.s3.jsrtarget, tbptr, copy);

						tbptr->type = BBTYPE_SBR;

						/* We need to check for overflow right here because
						 * the pushed value is poped afterwards */
						CHECKOVERFLOW;

						/* calculate stack after return */
						POPANY;
						stackdepth--;
						break;

					/* pop many push any */

					case ICMD_BUILTIN:
icmd_BUILTIN:
						bte = iptr->sx.s23.s3.bte;
						md = bte->md;
						goto _callhandling;

					case ICMD_INVOKESTATIC:
					case ICMD_INVOKESPECIAL:
					case ICMD_INVOKEVIRTUAL:
					case ICMD_INVOKEINTERFACE:
						COUNT(count_pcmd_met);

						/* Check for functions to replace with builtin
						 * functions. */

						if (builtintable_replace_function(iptr))
							goto icmd_BUILTIN;

						INSTRUCTION_GET_METHODDESC(iptr, md);
						/* XXX resurrect this COUNT? */
/*                          if (lm->flags & ACC_STATIC) */
/*                              {COUNT(count_check_null);} */

					_callhandling:

						coalescing_boundary = sd.new;

						i = md->paramcount;

						if (md->memuse > rd->memuse)
							rd->memuse = md->memuse;
						if (md->argintreguse > rd->argintreguse)
							rd->argintreguse = md->argintreguse;
						if (md->argfltreguse > rd->argfltreguse)
							rd->argfltreguse = md->argfltreguse;

						REQUIRE(i);

						/* XXX optimize for <= 2 args */
						/* XXX not for ICMD_BUILTIN */
						iptr->s1.argcount = stackdepth;
						iptr->sx.s23.s2.args = DMNEW(s4, stackdepth);

						copy = curstack;
						for (i-- ; i >= 0; i--) {
							iptr->sx.s23.s2.args[i] = copy->varnum;

							/* do not change STACKVARs or LOCALVARS to ARGVAR*/
							/* ->  won't help anyway */
							if (!(IS_OUTVAR(copy) || IS_LOCALVAR(copy))) {

#if defined(SUPPORT_PASS_FLOATARGS_IN_INTREGS)
			/* If we pass float arguments in integer argument registers, we
			 * are not allowed to precolor them here. Floats have to be moved
			 * to this regs explicitly in codegen().
			 * Only arguments that are passed by stack anyway can be precolored
			 * (michi 2005/07/24) */
							if (!(sd.var[copy->varnum].flags & SAVEDVAR) &&
							   (!IS_FLT_DBL_TYPE(copy->type) 
								|| md->params[i].inmemory)) {
#else
							if (!(sd.var[copy->varnum].flags & SAVEDVAR)) {
#endif

								SET_PREALLOC(copy);

#if defined(ENABLE_INTRP)
								if (!opt_intrp) {
#endif
									if (md->params[i].inmemory) {
										sd.var[copy->varnum].regoff =
											md->params[i].regoff;
										sd.var[copy->varnum].flags |= 
											INMEMORY;
									}
									else {
										if (IS_FLT_DBL_TYPE(copy->type)) {
#if defined(SUPPORT_PASS_FLOATARGS_IN_INTREGS)
											assert(0); /* XXX is this assert ok? */
#else
											sd.var[copy->varnum].regoff = 
										rd->argfltregs[md->params[i].regoff];
#endif /* SUPPORT_PASS_FLOATARGS_IN_INTREGS */
										}
										else {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
											if (IS_2_WORD_TYPE(copy->type))
												sd.var[copy->varnum].regoff = 
				PACK_REGS( rd->argintregs[GET_LOW_REG(md->params[i].regoff)],
						   rd->argintregs[GET_HIGH_REG(md->params[i].regoff)]);

											else
#endif /* SUPPORT_COMBINE_INTEGER_REGISTERS */
												sd.var[copy->varnum].regoff = 
										rd->argintregs[md->params[i].regoff];
										}
									}
#if defined(ENABLE_INTRP)
								} /* end if (!opt_intrp) */
#endif
							}
							}
							copy = copy->prev;
						}

						/* deal with live-through stack slots "under" the */
						/* arguments */
						/* XXX not for ICMD_BUILTIN */

						i = md->paramcount;

						while (copy) {
							SET_TEMPVAR(copy);
							iptr->sx.s23.s2.args[i++] = copy->varnum;
							sd.var[copy->varnum].flags |= SAVEDVAR;
							copy = copy->prev;
						}

						/* pop the arguments */

						i = md->paramcount;

						stackdepth -= i;
						while (--i >= 0) {
							POPANY;
						}

						/* push the return value */

						if (md->returntype.type != TYPE_VOID) {
							GET_NEW_VAR(sd, new_index, md->returntype.type);
							DST(md->returntype.type, new_index);
							stackdepth++;
						}
						break;

					case ICMD_INLINE_START:
					case ICMD_INLINE_END:
						CLR_S1;
						CLR_DST;
						break;

					case ICMD_MULTIANEWARRAY:
						coalescing_boundary = sd.new;
						if (rd->argintreguse < 3)
							rd->argintreguse = 3;

						i = iptr->s1.argcount;

						REQUIRE(i);

						iptr->sx.s23.s2.args = DMNEW(s4, i);

#if defined(SPECIALMEMUSE)
# if defined(__DARWIN__)
						if (rd->memuse < (i + INT_ARG_CNT +LA_SIZE_IN_POINTERS))
							rd->memuse = i + LA_SIZE_IN_POINTERS + INT_ARG_CNT;
# else
						if (rd->memuse < (i + LA_SIZE_IN_POINTERS + 3))
							rd->memuse = i + LA_SIZE_IN_POINTERS + 3;
# endif
#else
# if defined(__I386__)
						if (rd->memuse < i + 3)
							rd->memuse = i + 3; /* n integer args spilled on stack */
# elif defined(__MIPS__) && SIZEOF_VOID_P == 4
						if (rd->memuse < i + 2)
							rd->memuse = i + 2; /* 4*4 bytes callee save space */
# else
						if (rd->memuse < i)
							rd->memuse = i; /* n integer args spilled on stack */
# endif /* defined(__I386__) */
#endif
						copy = curstack;
						while (--i >= 0) {
					/* check INT type here? Currently typecheck does this. */
							iptr->sx.s23.s2.args[i] = copy->varnum;
							if (!(sd.var[copy->varnum].flags & SAVEDVAR)
								&& (!IS_OUTVAR(copy))
								&& (!IS_LOCALVAR(copy)) ) {
								copy->varkind = ARGVAR;
								sd.var[copy->varnum].flags |=
									INMEMORY & PREALLOC;
#if defined(SPECIALMEMUSE)
# if defined(__DARWIN__)
								sd.var[copy->varnum].regoff = i + 
									LA_SIZE_IN_POINTERS + INT_ARG_CNT;
# else
								sd.var[copy->varnum].regoff = i + 
									LA_SIZE_IN_POINTERS + 3;
# endif
#else
# if defined(__I386__)
								sd.var[copy->varnum].regoff = i + 3;
# elif defined(__MIPS__) && SIZEOF_VOID_P == 4
								sd.var[copy->varnum].regoff = i + 2;
# else
								sd.var[copy->varnum].regoff = i;
# endif /* defined(__I386__) */
#endif /* defined(SPECIALMEMUSE) */
							}
							copy = copy->prev;
						}
						while (copy) {
							sd.var[copy->varnum].flags |= SAVEDVAR;
							copy = copy->prev;
						}

						i = iptr->s1.argcount;
						stackdepth -= i;
						while (--i >= 0) {
							POPANY;
						}
						GET_NEW_VAR(sd, new_index, TYPE_ADR);
						DST(TYPE_ADR, new_index);
						stackdepth++;
						break;

					default:
						*exceptionptr =
							new_internalerror("Unknown ICMD %d", opcode);
						return false;
					} /* switch */

					CHECKOVERFLOW;
					iptr++;
				} /* while instructions */

				/* stack slots at basic block end become interfaces */

				sd.bptr->outstack = curstack;
				sd.bptr->outdepth = stackdepth;
				sd.bptr->outvars = DMNEW(s4, stackdepth);

				i = stackdepth - 1;
				for (copy = curstack; copy; i--, copy = copy->prev) {
					varinfo *v;

					/* with the new vars rd->interfaces will be removed */
					/* and all in and outvars have to be STACKVARS!     */
					/* in the moment i.e. SWAP with in and out vars can */
					/* create an unresolvable conflict */

					SET_TEMPVAR(copy);

					v = sd.var + copy->varnum;
					v->flags |= OUTVAR;

					if (jd->interface_map[i*5 + copy->type].flags == UNUSED) {
						/* no interface var until now for this depth and */
						/* type */
						jd->interface_map[i*5 + copy->type].flags = v->flags;
					}
					else {
						jd->interface_map[i*5 + copy->type].flags |= v->flags;
					}

					sd.bptr->outvars[i] = copy->varnum;
				}

				/* check if interface slots at basic block begin must be saved */
				IF_NO_INTRP(
					for (i=0; i<sd.bptr->indepth; ++i) {
						varinfo *v = sd.var + sd.bptr->invars[i];

						if (jd->interface_map[i*5 + v->type].flags == UNUSED) {
							/* no interface var until now for this depth and */
							/* type */
							jd->interface_map[i*5 + v->type].flags = v->flags;
						}
						else {
							jd->interface_map[i*5 + v->type].flags |= v->flags;
						}
					}
				);

#if defined(STACK_VERBOSE)
				printf("OUTVARS\n");
				for( copy = sd.bptr->outstack; copy; copy = copy->prev ) {
					printf("%2d(%d", copy->varnum, copy->type);
					if (IS_OUTVAR(copy))
						printf("S");
					if (IS_PREALLOC(copy))
						printf("A");
					printf(") ");
				}
				printf("\n");
#endif
		    } /* if */
			else
				superblockend = true;

			sd.bptr++;
		} /* while blocks */
	} while (repeat && !deadcode);

	/* gather statistics *****************************************************/

#if defined(ENABLE_STATISTICS)
	if (opt_stat) {
		if (jd->new_basicblockcount > count_max_basic_blocks)
			count_max_basic_blocks = jd->new_basicblockcount;
		count_basic_blocks += jd->new_basicblockcount;
		if (jd->new_instructioncount > count_max_javainstr)
			count_max_javainstr = jd->new_instructioncount;
		count_javainstr += jd->new_instructioncount;
		if (jd->new_stackcount > count_upper_bound_new_stack)
			count_upper_bound_new_stack = jd->new_stackcount;
		if ((sd.new - jd->new_stack) > count_max_new_stack)
			count_max_new_stack = (sd.new - jd->new_stack);

		b_count = jd->new_basicblockcount;
		sd.bptr = jd->new_basicblocks;
		while (--b_count >= 0) {
			if (sd.bptr->flags > BBREACHED) {
				if (sd.bptr->indepth >= 10)
					count_block_stack[10]++;
				else
					count_block_stack[sd.bptr->indepth]++;
				len = sd.bptr->icount;
				if (len < 10)
					count_block_size_distribution[len]++;
				else if (len <= 12)
					count_block_size_distribution[10]++;
				else if (len <= 14)
					count_block_size_distribution[11]++;
				else if (len <= 16)
					count_block_size_distribution[12]++;
				else if (len <= 18)
					count_block_size_distribution[13]++;
				else if (len <= 20)
					count_block_size_distribution[14]++;
				else if (len <= 25)
					count_block_size_distribution[15]++;
				else if (len <= 30)
					count_block_size_distribution[16]++;
				else
					count_block_size_distribution[17]++;
			}
			sd.bptr++;
		}

		if (iteration_count == 1)
			count_analyse_iterations[0]++;
		else if (iteration_count == 2)
			count_analyse_iterations[1]++;
		else if (iteration_count == 3)
			count_analyse_iterations[2]++;
		else if (iteration_count == 4)
			count_analyse_iterations[3]++;
		else
			count_analyse_iterations[4]++;

		if (jd->new_basicblockcount <= 5)
			count_method_bb_distribution[0]++;
		else if (jd->new_basicblockcount <= 10)
			count_method_bb_distribution[1]++;
		else if (jd->new_basicblockcount <= 15)
			count_method_bb_distribution[2]++;
		else if (jd->new_basicblockcount <= 20)
			count_method_bb_distribution[3]++;
		else if (jd->new_basicblockcount <= 30)
			count_method_bb_distribution[4]++;
		else if (jd->new_basicblockcount <= 40)
			count_method_bb_distribution[5]++;
		else if (jd->new_basicblockcount <= 50)
			count_method_bb_distribution[6]++;
		else if (jd->new_basicblockcount <= 75)
			count_method_bb_distribution[7]++;
		else
			count_method_bb_distribution[8]++;
	}
#endif /* defined(ENABLE_STATISTICS) */

	/* everything's ok *******************************************************/

	return true;

	/* goto labels for throwing verifier exceptions **************************/

#if defined(ENABLE_VERIFIER)

throw_stack_underflow:
	exceptions_throw_verifyerror(m, "Unable to pop operand off an empty stack");
	return false;

throw_stack_overflow:
	exceptions_throw_verifyerror(m, "Stack size too large");
	return false;

throw_stack_depth_error:
	exceptions_throw_verifyerror(m,"Stack depth mismatch");
	return false;

throw_stack_type_error:
	exceptions_throw_verifyerror_for_stack(m, expectedtype);
	return false;

throw_stack_category_error:
	exceptions_throw_verifyerror(m, "Attempt to split long or double on the stack");
	return false;

#endif
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
