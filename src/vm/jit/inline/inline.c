/* src/vm/jit/inline/inline.c - method inlining

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

   Authors: Edwin Steiner

   Changes:

   $Id: inline.c 5925 2006-11-05 23:11:27Z edwin $

*/

#include "config.h"

#include "vm/types.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "mm/memory.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/global.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/inline/inline.h"
#include "vm/jit/loop/loop.h"

#include "vm/class.h"
#include "vm/initialize.h"
#include "vm/method.h"
#include "vm/jit/jit.h"
#include "vm/jit/show.h"

#include "vm/jit/reg.h"
#include "vm/jit/stack.h"

#include "vm/jit/verify/typecheck.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#if !defined(NDEBUG)
#define INLINE_VERBOSE
bool inline_debug_log = 0;
bool inline_debug_log_names = 0;
int inline_debug_start_counter = 0;
int inline_debug_max_size = INT_MAX;
int inline_debug_min_size = 0;
int inline_debug_end_counter = INT_MAX;
int inline_count_methods = 0;
#define DOLOG(code) do{ if (inline_debug_log) { code; } }while(0)
#else
#define DOLOG(code)
#endif


/* types **********************************************************************/

typedef struct inline_node inline_node;
typedef struct inline_target_ref inline_target_ref;
typedef struct inline_context inline_context;
typedef struct inline_block_map inline_block_map;

struct inline_node {
	inline_context *ctx;

	jitdata *jd;
	methodinfo *m;
	inline_node *children;
	inline_node *next;                             /* next node at this depth */
	inline_node *prev;                             /* prev node at this depth */
	int depth;                                  /* inlining depth, 0 for root */

	/* info about the call site (if depth > 0)*/
	inline_node *parent;                /* node of the caller (NULL for root) */
	basicblock *callerblock;         /* original block containing the INVOKE* */
	instruction *callerins;               /* the original INVOKE* instruction */
	s4 callerpc;
	s4 *n_passthroughvars;
	int n_passthroughcount;
	int n_selfpassthroughcount;  /* # of pass-through vars of the call itself */
	exception_entry **o_handlers;
	int n_handlercount;                 /* # of handlers protecting this call */
	int n_resultlocal;
	int synclocal;                    /* variable used for synchr., or UNUSED */

	bool blockbefore;                  /* block boundary before inlined body? */
	bool blockafter;                   /* block boundary after inlined body?  */

	/* info about the callee */
	int localsoffset;
	int prolog_instructioncount;         /* # of ICMDs in the inlining prolog */
	int epilog_instructioncount;         /* # of ICMDs in the inlining epilog */
	int extra_instructioncount;
	int instructioncount;
	bool synchronize;                /* do we have to synchronize enter/exit? */
	basicblock *handler_monitorexit;     /* handler for synchronized inlinees */
	s4 *varmap;

	/* cumulative values */
	int cumul_instructioncount;  /* ICMDs in this node and its children       */
	int cumul_basicblockcount;   /* BBs started by this node and its children */
	int cumul_blockmapcount;
	int cumul_maxlocals;
	int cumul_exceptiontablelength;

	/* output */
	instruction *inlined_iinstr;
	instruction *inlined_iinstr_cursor;
	basicblock *inlined_basicblocks;
	basicblock *inlined_basicblocks_cursor;

	/* register data */
	registerdata *regdata;

	/* temporary */
	inline_target_ref *refs;
	instruction *inline_start_instruction;

	/* XXX debug */
	char *indent;
	int debugnr;
};

struct inline_target_ref {
	inline_target_ref *next;
	basicblock **ref;
	basicblock *target;
};

struct inline_block_map {
	inline_node *iln;
	basicblock *o_block;
	basicblock *n_block;
};

struct inline_context {
	inline_node *master;

	jitdata *resultjd;

	int next_block_number;
	inline_block_map *blockmap;
	int blockmap_index;

	int maxinoutdepth;

	bool calls_others;

	int next_debugnr; /* XXX debug */
};


/* prototypes *****************************************************************/

static bool inline_inline_intern(methodinfo *m, inline_node *iln);
static void inline_post_process(jitdata *jd);


/* debug helpers **************************************************************/

#if !defined(NDEBUG)
#include "inline_debug.inc"

void inline_print_stats()
{
	printf("inlined callers: %d\n", inline_count_methods);
}
#endif


/* compilation of callees *****************************************************/

static bool inline_jit_compile_intern(jitdata *jd)
{
	methodinfo *m;

	/* XXX should share code with jit.c */

	assert(jd);

	/* XXX initialize the static function's class */

	m = jd->m;

	/* call the compiler passes ***********************************************/

	/* call parse pass */

	DOLOG( log_message_class("Parsing ", m->class) );
	if (!parse(jd)) {
		return false;
	}

	/* call stack analysis pass */

	if (!stack_analyse(jd)) {
		return false;
	}

	return true;
}


static bool inline_jit_compile(inline_node *iln)
{
	bool                r;
	methodinfo         *m;
	jitdata            *jd;

	/* XXX should share code with jit.c */

	assert(iln);
	m = iln->m;
	assert(m);

#if defined(ENABLE_THREADS)
	/* enter a monitor on the method */
	lock_monitor_enter((java_objectheader *) m);
#endif

	/* allocate jitdata structure and fill it */

	jd = jit_jitdata_new(m);
	iln->jd = jd;

	jd->flags = 0; /* XXX */

	/* initialize the register allocator */

	reg_setup(jd);

	/* setup the codegendata memory */

	/* XXX do a pseudo setup */
	jd->cd = DNEW(codegendata);
	MZERO(jd->cd, codegendata, 1);
	jd->cd->maxstack = m->maxstack;
	jd->cd->method = m;
	/* XXX uses too much dump memory codegen_setup(jd); */

	/* now call internal compile function */

	r = inline_jit_compile_intern(jd);

	if (r) {
		iln->regdata = jd->rd;
	}

	/* free some memory */
#if 0

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp)
# endif
		codegen_free(jd);
#endif

#endif

#if defined(ENABLE_THREADS)
	/* leave the monitor */
	lock_monitor_exit((java_objectheader *) m );
#endif

	return r;
}


/* inlining tree handling *****************************************************/

static void insert_inline_node(inline_node *parent, inline_node *child)
{
	inline_node *first;
	inline_node *succ;

	assert(parent && child);

	child->parent = parent;

	child->debugnr = parent->ctx->next_debugnr++; /* XXX debug */

	first = parent->children;
	if (!first) {
		/* insert as only node */
		parent->children = child;
		child->next = child;
		child->prev = child;
		return;
	}

	/* {there is at least one child already there} */

	/* XXX is this search necessary, or could we always add at the end? */

	succ = first;
	while (succ->callerpc < child->callerpc) {
		succ = succ->next;
		if (succ == first) {
			/* insert as last node */
			child->prev = first->prev;
			child->next = first;
			child->prev->next = child;
			child->next->prev = child;
			return;
		}
	}

	assert(succ->callerpc > child->callerpc);

	/* insert before succ */

	child->prev = succ->prev;
	child->next = succ;
	child->prev->next = child;
	child->next->prev = child;
}


/* variable handling **********************************************************/

static s4 inline_new_variable(jitdata *jd, s4 type, s4 flags)
{
	s4 index;
	s4 newcount;

	index = jd->vartop++;
	if (index >= jd->varcount) {
		newcount = jd->vartop * 2; /* XXX */
		jd->var = DMREALLOC(jd->var, varinfo, jd->varcount, newcount);
		MZERO(jd->var + jd->varcount, varinfo, (newcount - jd->varcount));
		jd->varcount = newcount;
	}

	jd->var[index].type = type;
	jd->var[index].flags = flags;

	return index;
}


static s4 inline_new_variable_clone(jitdata *jd, jitdata *origjd, s4 origidx)
{
	varinfo *v;
	s4       newidx;

	v = &(origjd->var[origidx]);

	newidx = inline_new_variable(jd, v->type, v->flags);

	jd->var[newidx].vv = v->vv;

	return newidx;
}


static s4 inline_new_temp_variable(jitdata *jd, s4 type)
{
	return inline_new_variable(jd, type, 0);
}


static s4 inline_translate_variable(jitdata *jd, jitdata *origjd, s4 *varmap, s4 index)
{
	s4 idx;

	idx = varmap[index];

	if (idx < 0) {
		idx = inline_new_variable_clone(jd, origjd, index);
		varmap[index] = idx;
	}

	return idx;
}


static s4 *create_variable_map(inline_node *callee)
{
	s4 *varmap;
	s4 i, t;
	s4 idx;
	s4 n_idx;
	s4 avail;
	varinfo *v;
	varinfo vinfo;

	/* create the variable mapping */

	varmap = DMNEW(s4, callee->jd->varcount);
	for (i=0; i<callee->jd->varcount; ++i)
		varmap[i] = -1;

	/* translate local variables */

	for (i=0; i<callee->m->maxlocals; ++i) {
		for (t=0; t<5; ++t) {
			idx = callee->jd->local_map[5*i + t];
			if (idx == UNUSED)
				continue;

			v = &(callee->jd->var[idx]);
			assert(v->type == t || v->type == TYPE_VOID); /* XXX stack leaves VOID */
			v->type = t; /* XXX restore if it is TYPE_VOID */

			avail = callee->ctx->resultjd->local_map[5*(callee->localsoffset + i) + t];

			if (avail == UNUSED) {
				avail = inline_new_variable_clone(callee->ctx->resultjd, callee->jd, idx);
				callee->ctx->resultjd->local_map[5*(callee->localsoffset + i) + t] = avail;
			}

			varmap[idx] = avail;
		}
	}

	/* for synchronized instance methods we need an extra local */

	if (callee->synchronize && !(callee->m->flags & ACC_STATIC)) {
		n_idx = callee->localsoffset - 1;
		assert(n_idx >= 0);
		assert(callee->parent);
		assert(n_idx == callee->parent->localsoffset + callee->parent->m->maxlocals);

		avail = callee->ctx->resultjd->local_map[5*n_idx + TYPE_ADR];

		if (avail == UNUSED) {
			avail = inline_new_variable(callee->ctx->resultjd, TYPE_ADR, 0);
			callee->ctx->resultjd->local_map[5*n_idx + TYPE_ADR] = avail;
		}

		callee->synclocal = avail;
	}
	else {
		callee->synclocal = UNUSED;
	}

	return varmap;
}


/* basic block translation ****************************************************/

#define INLINE_RETURN_REFERENCE(callee)  \
	( (basicblock *) (ptrint) (0x333 + (callee)->depth) )


static void inline_add_block_reference(inline_node *iln, basicblock **blockp)
{
	inline_target_ref *ref;

	ref = DNEW(inline_target_ref);
	ref->ref = blockp;
	ref->next = iln->refs;
	iln->refs = ref;
}


static void inline_block_translation(inline_node *iln, basicblock *o_bptr, basicblock *n_bptr)
{
	inline_context *ctx;

	ctx = iln->ctx;
	assert(ctx->blockmap_index < ctx->master->cumul_blockmapcount);

	ctx->blockmap[ctx->blockmap_index].iln = iln;
	ctx->blockmap[ctx->blockmap_index].o_block = o_bptr;
	ctx->blockmap[ctx->blockmap_index].n_block = n_bptr;

	ctx->blockmap_index++;
}


static basicblock * inline_map_block(inline_node *iln,
									 basicblock *o_block,
									 inline_node *targetiln)
{
	inline_block_map *bm;
	inline_block_map *bmend;

	assert(iln);
	assert(targetiln);

	if (!o_block)
		return NULL;

	bm = iln->ctx->blockmap;
	bmend = bm + iln->ctx->blockmap_index;

	while (bm < bmend) {
		assert(bm->iln && bm->o_block && bm->n_block);
		if (bm->o_block == o_block && bm->iln == targetiln)
			return bm->n_block;
		bm++;
	}

	assert(false);
	return NULL; /* not reached */
}


static void inline_resolve_block_refs(inline_target_ref **refs,
									  basicblock *o_bptr,
									  basicblock *n_bptr)
{
	inline_target_ref *ref;
	inline_target_ref *prev;

	ref = *refs;
	prev = NULL;
	while (ref) {
		if (*(ref->ref) == o_bptr) {

#if defined(INLINE_VERBOSE)
			if (inline_debug_log) {
				if ((ptrint) o_bptr < (0x333+100)) { /* XXX */
					printf("resolving RETURN block reference %p -> new L%03d (%p)\n",
							(void*)o_bptr, n_bptr->nr, (void*)n_bptr);
				}
				else {
					printf("resolving block reference old L%03d (%p) -> new L%03d (%p)\n",
							o_bptr->nr, (void*)o_bptr, n_bptr->nr, (void*)n_bptr);
				}
			}
#endif

			*(ref->ref) = n_bptr;
			if (prev) {
				prev->next = ref->next;
			}
			else {
				*refs = ref->next;
			}
		}
		else {
			prev = ref;
		}
		ref = ref->next;
	}
}


/* basic block creation *******************************************************/

static basicblock * create_block(inline_node *container,
								 inline_node *iln,
								 inline_node *inner,
								 basicblock *o_bptr,
								 inline_target_ref **refs,
								 int indepth)
{
	basicblock  *n_bptr;
	inline_node *outer;
	s4           i;
	s4           depth;
	s4           varidx;
	s4           newvaridx;

	assert(container);
	assert(iln);
	assert(inner);
	assert(indepth >= 0);

	n_bptr = container->inlined_basicblocks_cursor++;
	assert(n_bptr);
	assert((n_bptr - container->inlined_basicblocks) < container->cumul_basicblockcount);

	BASICBLOCK_INIT(n_bptr, iln->m);

	n_bptr->iinstr = container->inlined_iinstr_cursor;
	n_bptr->next = n_bptr + 1;
	n_bptr->nr = container->ctx->next_block_number++;
	n_bptr->indepth = indepth;
	n_bptr->flags = BBFINISHED; /* XXX */

	if (indepth > container->ctx->maxinoutdepth)
		container->ctx->maxinoutdepth = indepth;

	if (indepth) {
		n_bptr->invars = DMNEW(s4, indepth);


		for (i=0; i<indepth; ++i)
			n_bptr->invars[i] = -1; /* XXX debug */

		/* pass-through variables enter the block */

		outer = inner->parent;
		while (outer != NULL) {
			depth = outer->n_passthroughcount;

			assert(depth + inner->n_selfpassthroughcount <= indepth);

			for (i=0; i<inner->n_selfpassthroughcount; ++i) {
				varidx = inner->n_passthroughvars[i];
				newvaridx =
					inline_new_variable_clone(container->ctx->resultjd,
											  outer->jd,
											  varidx);
				n_bptr->invars[depth + i] = newvaridx;
				outer->varmap[varidx] = newvaridx;
			}
			inner = outer;
			outer = outer->parent;
		}
	}
	else {
		n_bptr->invars = NULL;
	}

	/* XXX move this to callers with o_bptr != NULL? */
	if (o_bptr) {
		assert(refs);
		inline_resolve_block_refs(refs, o_bptr, n_bptr);
	}

	{
		varinfo *dv;

		/* XXX for the verifier. should not be here */

		dv = DMNEW(varinfo, iln->ctx->resultjd->localcount + VERIFIER_EXTRA_LOCALS);
		MZERO(dv, varinfo,  iln->ctx->resultjd->localcount + VERIFIER_EXTRA_LOCALS);
		n_bptr->inlocals = dv;
	}

	return n_bptr;
}


static basicblock * create_body_block(inline_node *iln,
									  basicblock *o_bptr, s4 *varmap)
{
	basicblock *n_bptr;
	s4 i;

	n_bptr = create_block(iln, iln, iln, o_bptr, &(iln->refs),
						  o_bptr->indepth + iln->n_passthroughcount);

	n_bptr->type = o_bptr->type;
	n_bptr->flags = o_bptr->flags;

	/* translate the invars of the original block */

	for (i=0; i<o_bptr->indepth; ++i) {
		n_bptr->invars[iln->n_passthroughcount + i] =
			inline_translate_variable(iln->ctx->resultjd, iln->jd,
				varmap,
				o_bptr->invars[i]);
	}

	return n_bptr;
}


static basicblock * create_epilog_block(inline_node *caller, inline_node *callee, s4 *varmap)
{
	basicblock *n_bptr;
	s4 retcount;
	s4 idx;
	varinfo vinfo;

	/* number of return variables */

	retcount = (callee->n_resultlocal == -1
				&& callee->m->parseddesc->returntype.type != TYPE_VOID) ? 1 : 0;

	/* start the epilog block */

	n_bptr = create_block(caller, caller, callee, INLINE_RETURN_REFERENCE(callee),
			&(callee->refs), callee->n_passthroughcount + retcount);

	/* return variable */

	if (retcount) {
		idx = inline_new_variable(caller->ctx->resultjd,
			   callee->m->parseddesc->returntype.type, 0 /* XXX */);
		n_bptr->invars[callee->n_passthroughcount] = idx;
		varmap[callee->callerins->dst.varindex] = idx;
	}

	n_bptr->flags = /* XXX original block flags */ BBFINISHED;
	n_bptr->type = BBTYPE_STD;

	return n_bptr;
}


static void close_block(inline_node *iln, inline_node *inner, basicblock *n_bptr, s4 outdepth)
{
	inline_node *outer;
	s4           i;
	s4           depth;
	s4           varidx;

	n_bptr->outdepth = outdepth;
	n_bptr->outvars = DMNEW(s4, outdepth);

	for (i=0; i<outdepth; ++i)
		n_bptr->outvars[i] = 0; /* XXX debug */

	if (outdepth > iln->ctx->maxinoutdepth)
		iln->ctx->maxinoutdepth = outdepth;

	/* pass-through variables leave the block */

	outer = inner->parent;
	while (outer != NULL) {
		depth = outer->n_passthroughcount;

		assert(depth + inner->n_selfpassthroughcount <= outdepth);

		for (i=0; i<inner->n_selfpassthroughcount; ++i) {
			varidx = inner->n_passthroughvars[i];
			n_bptr->outvars[depth + i] =
				inline_translate_variable(iln->ctx->resultjd,
										  outer->jd,
										  outer->varmap,
										  varidx);
		}
		inner = outer;
		outer = outer->parent;
	}
}


static void close_prolog_block(inline_node *iln,
							   basicblock *n_bptr,
							   inline_node *nextcall)
{
	/* XXX add original outvars! */
	close_block(iln, nextcall, n_bptr, nextcall->n_passthroughcount);

	/* pass-through variables */

	DOLOG( printf("closed prolog block:\n");
		   show_basicblock(iln->ctx->resultjd, n_bptr, SHOW_STACK); );
}


static void close_body_block(inline_node *iln,
							 basicblock *n_bptr,
							 basicblock *o_bptr,
							 s4 *varmap,
							 s4 retcount,
							 s4 retidx)
{
	s4 i;

	close_block(iln, iln, n_bptr, iln->n_passthroughcount + o_bptr->outdepth + retcount);

	/* translate the outvars of the original block */

	/* XXX reuse code */
	for (i=0; i<o_bptr->outdepth; ++i) {
		n_bptr->outvars[iln->n_passthroughcount + i] =
			inline_translate_variable(iln->ctx->resultjd, iln->jd, varmap,
					o_bptr->outvars[i]);
	}

	/* set the return variable, if any */

	if (retcount) {
		assert(retidx >= 0);
		n_bptr->outvars[iln->n_passthroughcount + o_bptr->outdepth] = retidx;
	}
}


/* inlined code generation ****************************************************/

static s4 emit_inlining_prolog(inline_node *iln,
							   inline_node *callee,
							   instruction *o_iptr,
							   s4 *varmap)
{
	methodinfo *calleem;
	methoddesc *md;
	int i;
	int localindex;
	int type;
	bool isstatic;
	instruction *n_ins;
	insinfo_inline *insinfo;
	s4 argvar;

	assert(iln && callee && o_iptr);

	calleem = callee->m;
	md = calleem->parseddesc;
	isstatic = (calleem->flags & ACC_STATIC);

	localindex = callee->localsoffset + md->paramslots;

	for (i=md->paramcount-1; i>=0; --i) {
		assert(iln);

		n_ins = (iln->inlined_iinstr_cursor++);
		assert((n_ins - iln->inlined_iinstr) < iln->cumul_instructioncount);

		type = md->paramtypes[i].type;

		localindex -= IS_2_WORD_TYPE(type) ? 2 : 1;
		assert(callee->regdata);

		/* translate the argument variable */

		argvar = varmap[o_iptr->sx.s23.s2.args[i]];
		assert(argvar != UNUSED);

		/* remove preallocation from the argument variable */

		iln->ctx->resultjd->var[argvar].flags &= ~(PREALLOC | INMEMORY);

		/* check the instance slot against NULL */

		if (!isstatic && i == 0) {
			assert(type == TYPE_ADR);
			n_ins->opc = ICMD_CHECKNULL;
			n_ins->s1.varindex = argvar;
			n_ins->dst.varindex = n_ins->s1.varindex;
			n_ins->line = o_iptr->line;

			n_ins = (iln->inlined_iinstr_cursor++);
			assert((n_ins - iln->inlined_iinstr) < iln->cumul_instructioncount);
		}

		/* store argument into local variable of inlined callee */

		if (callee->jd->local_map[5*(localindex - callee->localsoffset) + type] != UNUSED)
		{
			/* this value is used in the callee */

			if (i == 0 && callee->synclocal != UNUSED) {
				/* we also need it for synchronization, so copy it */
				assert(type == TYPE_ADR);
				n_ins->opc = ICMD_COPY;
			}
			else {
				n_ins->opc = ICMD_ISTORE + type;
			}
			n_ins->s1.varindex = argvar;
			n_ins->dst.varindex = iln->ctx->resultjd->local_map[5*localindex + type];
			assert(n_ins->dst.varindex != UNUSED);
		}
		else if (i == 0 && callee->synclocal != UNUSED) {
			/* the value is not used inside the callee, but we need it for */
			/* synchronization                                             */
			/* XXX In this case it actually makes no sense to create a     */
			/*     separate synchronization variable.                      */

			n_ins->opc = ICMD_NOP;
		}
		else {
			/* this value is not used, pop it */

			n_ins->opc = ICMD_POP;
			n_ins->s1.varindex = argvar;
		}
		n_ins->line = o_iptr->line;

		DOLOG( printf("%sprolog: ", iln->indent);
			   show_icmd(iln->ctx->resultjd, n_ins, false, SHOW_STACK); printf("\n"); );
	}

	/* ASTORE for synchronized instance methods */

	if (callee->synclocal != UNUSED) {
		n_ins = (iln->inlined_iinstr_cursor++);
		assert((n_ins - iln->inlined_iinstr) < iln->cumul_instructioncount);

		n_ins->opc = ICMD_ASTORE;
		n_ins->s1.varindex = varmap[o_iptr->sx.s23.s2.args[0]];
		n_ins->dst.varindex = callee->synclocal;
		n_ins->line = o_iptr->line;

		assert(n_ins->s1.varindex != UNUSED);
	}

	/* INLINE_START instruction */

	n_ins = (iln->inlined_iinstr_cursor++);
	assert((n_ins - iln->inlined_iinstr) < iln->cumul_instructioncount);

	insinfo = DNEW(insinfo_inline);
	insinfo->method = callee->m;
	insinfo->outer = iln->m;
	insinfo->synclocal = callee->synclocal;
	insinfo->synchronize = callee->synchronize;

	n_ins->opc = ICMD_INLINE_START;
	n_ins->sx.s23.s3.inlineinfo = insinfo;
	n_ins->line = o_iptr->line;
	iln->inline_start_instruction = n_ins;

	DOLOG( printf("%sprolog: ", iln->indent);
		   show_icmd(iln->ctx->resultjd, n_ins, false, SHOW_STACK); printf("\n"); );

	return 0; /* XXX */
}


static void emit_inlining_epilog(inline_node *iln, inline_node *callee, instruction *o_iptr)
{
	instruction *n_ins;

	assert(iln && callee && o_iptr);
	assert(iln->inline_start_instruction);

	/* INLINE_END instruction */

	n_ins = (iln->inlined_iinstr_cursor++);
	assert((n_ins - iln->inlined_iinstr) < iln->cumul_instructioncount);

	n_ins->opc = ICMD_INLINE_END;
	n_ins->sx.s23.s3.inlineinfo = iln->inline_start_instruction->sx.s23.s3.inlineinfo;
	n_ins->line = o_iptr->line;

	DOLOG( printf("%sepilog: ", iln->indent);
		   show_icmd(iln->ctx->resultjd, n_ins, false, SHOW_STACK); printf("\n"); );
}


#define TRANSLATE_VAROP(vo)  \
	n_iptr->vo.varindex = inline_translate_variable(jd, origjd, varmap, n_iptr->vo.varindex)


static void inline_clone_instruction(inline_node *iln,
									 jitdata *jd,
									 jitdata *origjd,
									 s4 *varmap,
									 instruction *o_iptr,
									 instruction *n_iptr)
{
	icmdtable_entry_t *icmdt;
	builtintable_entry *bte;
	methoddesc *md;
	s4 i, j;
	branch_target_t *table;
	lookup_target_t *lookup;
	inline_node *scope;

	*n_iptr = *o_iptr;

	icmdt = &(icmd_table[o_iptr->opc]);

	switch (icmdt->dataflow) {
		case DF_0_TO_0:
			break;

		case DF_3_TO_0:
			TRANSLATE_VAROP(sx.s23.s3);
		case DF_2_TO_0:
			TRANSLATE_VAROP(sx.s23.s2);
		case DF_1_TO_0:
			TRANSLATE_VAROP(s1);
			break;

		case DF_2_TO_1:
			TRANSLATE_VAROP(sx.s23.s2);
		case DF_1_TO_1:
		case DF_COPY:
		case DF_MOVE:
			TRANSLATE_VAROP(s1);
		case DF_0_TO_1:
			TRANSLATE_VAROP(dst);
			break;

		case DF_N_TO_1:
			n_iptr->sx.s23.s2.args = DMNEW(s4, n_iptr->s1.argcount);
			for (i=0; i<n_iptr->s1.argcount; ++i) {
				n_iptr->sx.s23.s2.args[i] =
					inline_translate_variable(jd, origjd, varmap,
							o_iptr->sx.s23.s2.args[i]);
			}
			TRANSLATE_VAROP(dst);
			break;

		case DF_INVOKE:
			INSTRUCTION_GET_METHODDESC(n_iptr, md);
clone_call:
			n_iptr->s1.argcount += iln->n_passthroughcount;
			n_iptr->sx.s23.s2.args = DMNEW(s4, n_iptr->s1.argcount);
			for (i=0; i<o_iptr->s1.argcount; ++i) {
				n_iptr->sx.s23.s2.args[i] =
					inline_translate_variable(jd, origjd, varmap,
							o_iptr->sx.s23.s2.args[i]);
			}
			for (scope = iln; scope != NULL; scope = scope->parent) {
				for (j = 0; j < scope->n_selfpassthroughcount; ++j) {
					n_iptr->sx.s23.s2.args[i++] =
						scope->parent->varmap[scope->n_passthroughvars[j]];
				}
			}
			if (md->returntype.type != TYPE_VOID)
				TRANSLATE_VAROP(dst);
			break;

		case DF_BUILTIN:
			bte = n_iptr->sx.s23.s3.bte;
			md = bte->md;
			goto clone_call;

		default:
			assert(0);
	}

	switch (icmdt->controlflow) {
		case CF_RET:
			TRANSLATE_VAROP(s1); /* XXX should be handled by data-flow */
			/* FALLTHROUGH */
		case CF_IF:
		case CF_GOTO:
			inline_add_block_reference(iln, &(n_iptr->dst.block));
			break;

		case CF_JSR:
			inline_add_block_reference(iln, &(n_iptr->sx.s23.s3.jsrtarget.block));
			break;

		case CF_TABLE:
			i = n_iptr->sx.s23.s3.tablehigh - n_iptr->sx.s23.s2.tablelow + 1 + 1 /* default */;

			table = DMNEW(branch_target_t, i);
			MCOPY(table, o_iptr->dst.table, branch_target_t, i);
			n_iptr->dst.table = table;

			while (--i >= 0) {
				inline_add_block_reference(iln, &(table->block));
				table++;
			}
			break;

		case CF_LOOKUP:
			inline_add_block_reference(iln, &(n_iptr->sx.s23.s3.lookupdefault.block));

			i = n_iptr->sx.s23.s2.lookupcount;
			lookup = DMNEW(lookup_target_t, i);
			MCOPY(lookup, o_iptr->dst.lookup, lookup_target_t, i);
			n_iptr->dst.lookup = lookup;

			while (--i >= 0) {
				inline_add_block_reference(iln, &(lookup->target.block));
				lookup++;
			}
			break;
	}
}


static void rewrite_method(inline_node *iln)
{
	basicblock *o_bptr;
	s4 len;
	instruction *o_iptr;
	instruction *n_iptr;
	inline_node *nextcall;
	basicblock *n_bptr;
	inline_block_map *bm;
	int i;
	int icount;
	jitdata *resultjd;
	jitdata *origjd;
	char indent[100]; /* XXX debug */
	s4 retcount;
	s4 retidx;

	assert(iln);

	resultjd = iln->ctx->resultjd;
	origjd = iln->jd;

	n_bptr = NULL;
	nextcall = iln->children;

	/* XXX debug */
	for (i=0; i<iln->depth; ++i)
		indent[i] = '\t';
	indent[i] = 0;
	iln->indent = indent;

	DOLOG( printf("%srewriting: ", indent); method_println(iln->m);
		   printf("%s(passthrough: %d+%d)\n",
				indent, iln->n_passthroughcount - iln->n_selfpassthroughcount,
				iln->n_passthroughcount); );

	/* set memory cursors */

	iln->inlined_iinstr_cursor = iln->inlined_iinstr;
	iln->inlined_basicblocks_cursor = iln->inlined_basicblocks;

	/* loop over basic blocks */

	o_bptr = iln->jd->basicblocks;
	for (; o_bptr; o_bptr = o_bptr->next) {

		if (o_bptr->flags < BBREACHED) {

			/* ignore the dummy end block */

			if (o_bptr->icount == 0 && o_bptr->next == NULL) {
				/* enter the following block as translation, for exception handler ranges */
				inline_block_translation(iln, o_bptr, iln->inlined_basicblocks_cursor);
				continue;
			}

			DOLOG(
			printf("%s* skipping old L%03d (flags=%d, type=%d, oid=%d) of ",
					indent,
					o_bptr->nr, o_bptr->flags, o_bptr->type,
					o_bptr->indepth);
			method_println(iln->m);
			);

			n_bptr = create_body_block(iln, o_bptr, iln->varmap);

			/* enter it in the blockmap */

			inline_block_translation(iln, o_bptr, n_bptr);

			close_body_block(iln, n_bptr, o_bptr, iln->varmap, 0, -1);
			continue;
		}

		len = o_bptr->icount;
		o_iptr = o_bptr->iinstr;

		DOLOG(
		printf("%s* rewriting old L%03d (flags=%d, type=%d, oid=%d) of ",
				indent,
				o_bptr->nr, o_bptr->flags, o_bptr->type,
				o_bptr->indepth);
		method_println(iln->m);
		show_basicblock(iln->jd, o_bptr, SHOW_STACK);
		);

		if (iln->blockbefore || o_bptr != iln->jd->basicblocks) {
			/* create an inlined clone of this block */

			n_bptr = create_body_block(iln, o_bptr, iln->varmap);
			icount = 0;

			/* enter it in the blockmap */

			inline_block_translation(iln, o_bptr, n_bptr);
		}
		else {
			/* continue caller block */

			n_bptr = iln->inlined_basicblocks_cursor - 1;
			icount = n_bptr->icount;
		}

		retcount = 0;
		retidx = UNUSED;

		while (--len >= 0) {

			DOLOG( fputs(indent, stdout); show_icmd(iln->jd, o_iptr, false,  SHOW_STACK);
				   printf("\n") );

			/* handle calls that will be inlined */

			if (nextcall && o_iptr == nextcall->callerins) {

				/* write the inlining prolog */

				(void) emit_inlining_prolog(iln, nextcall, o_iptr, iln->varmap);
				icount += nextcall->prolog_instructioncount;

				/* end current block, or glue blocks together */

				n_bptr->icount = icount;

				if (nextcall->blockbefore) {
					close_prolog_block(iln, n_bptr, nextcall);
				}
				else {
					/* XXX */
				}

				/* check if the result is a local variable */

				if (nextcall->m->parseddesc->returntype.type != TYPE_VOID
						&& o_iptr->dst.varindex < iln->jd->localcount)
				{
					nextcall->n_resultlocal = iln->varmap[o_iptr->dst.varindex];
				}
				else
					nextcall->n_resultlocal = -1;

				/* set memory pointers in the callee */

				nextcall->inlined_iinstr = iln->inlined_iinstr_cursor;
				nextcall->inlined_basicblocks = iln->inlined_basicblocks_cursor;

				/* recurse */

				DOLOG( printf("%sentering inline ", indent);
					   show_icmd(origjd, o_iptr, false, SHOW_STACK); printf("\n") );

				rewrite_method(nextcall);

				DOLOG( printf("%sleaving inline ", indent);
					   show_icmd(origjd, o_iptr, false, SHOW_STACK); printf("\n") );

				/* update memory cursors */

				assert(nextcall->inlined_iinstr_cursor
						<= iln->inlined_iinstr_cursor + nextcall->cumul_instructioncount);
				assert(nextcall->inlined_basicblocks_cursor
						== iln->inlined_basicblocks_cursor + nextcall->cumul_basicblockcount);
				iln->inlined_iinstr_cursor = nextcall->inlined_iinstr_cursor;
				iln->inlined_basicblocks_cursor = nextcall->inlined_basicblocks_cursor;

				/* start new block, or glue blocks together */

				if (nextcall->blockafter) {
					n_bptr = create_epilog_block(iln, nextcall, iln->varmap);
					icount = 0;
				}
				else {
					n_bptr = iln->inlined_basicblocks_cursor - 1;
					icount = n_bptr->icount;
					/* XXX */
				}

				/* emit inlining epilog */

				emit_inlining_epilog(iln, nextcall, o_iptr);
				icount++; /* XXX epilog instructions */

				/* proceed to next call */

				nextcall = nextcall->next;
			}
			else {
				n_iptr = (iln->inlined_iinstr_cursor++);
				assert((n_iptr - iln->inlined_iinstr) < iln->cumul_instructioncount);

				switch (o_iptr->opc) {
					case ICMD_RETURN:
						if (iln->depth == 0)
							goto default_clone;
						goto return_tail;

					case ICMD_IRETURN:
					case ICMD_ARETURN:
					case ICMD_LRETURN:
					case ICMD_FRETURN:
					case ICMD_DRETURN:
						if (iln->depth == 0)
							goto default_clone;
						retcount = 1;
						retidx = iln->varmap[o_iptr->s1.varindex];
						if (iln->n_resultlocal != -1) {
							/* store result in a local variable */

							DOLOG( printf("USING RESULTLOCAL %d\n", iln->n_resultlocal); );
							/* This relies on the same sequence of types for */
							/* ?STORE and ?RETURN opcodes.                   */
							n_iptr->opc = ICMD_ISTORE + (o_iptr->opc - ICMD_IRETURN);
							n_iptr->s1.varindex = retidx;
							n_iptr->dst.varindex = iln->n_resultlocal;

							retcount = 0;
							retidx = UNUSED;

							n_iptr = (iln->inlined_iinstr_cursor++);
							assert((n_iptr - iln->inlined_iinstr) < iln->cumul_instructioncount);
							icount++;
						}
						else if ((retidx < resultjd->localcount && iln->blockafter)
								|| !iln->blockafter) /* XXX do we really always need the MOVE? */
						{
							/* local must not become outvar, insert a MOVE */

							n_iptr->opc = ICMD_MOVE;
							n_iptr->s1.varindex = retidx;
							retidx = inline_new_temp_variable(resultjd,
															  resultjd->var[retidx].type);
							n_iptr->dst.varindex = retidx;

							n_iptr = (iln->inlined_iinstr_cursor++);
							assert((n_iptr - iln->inlined_iinstr) < iln->cumul_instructioncount);
							icount++;
						}
return_tail:
						if (iln->blockafter) {
							n_iptr->opc = ICMD_GOTO;
							n_iptr->dst.block = INLINE_RETURN_REFERENCE(iln);
							inline_add_block_reference(iln, &(n_iptr->dst.block));
						}
						else {
							n_iptr->opc = ICMD_NOP;
						}
						break;
#if 0
						if (o_bptr->next == NULL || (o_bptr->next->icount==0 && o_bptr->next->next == NULL)) {
							n_iptr->opc = ICMD_NOP;
							break;
						}
						goto default_clone;
						break;
#endif

					default:
default_clone:
						inline_clone_instruction(iln, resultjd, iln->jd, iln->varmap, o_iptr, n_iptr);
				}

				DOLOG( fputs(indent, stdout); show_icmd(resultjd, n_iptr, false, SHOW_STACK);
					   printf("\n"););

				icount++;
			}

			o_iptr++;
		}

		/* end of basic block */

		if (iln->blockafter || (o_bptr->next && o_bptr->next->next)) {
			close_body_block(iln, n_bptr, o_bptr, iln->varmap, retcount, retidx);
			n_bptr->icount = icount;

			DOLOG( printf("closed body block:\n");
				   show_basicblock(resultjd, n_bptr, SHOW_STACK); );
		}
		else {
			n_bptr->icount = icount;
			assert(iln->parent);
			if (retidx != UNUSED)
				iln->parent->varmap[iln->callerins->dst.varindex] = retidx;
		}
	}

	bm = iln->ctx->blockmap;
	for (i=0; i<iln->ctx->blockmap_index; ++i, ++bm) {
		assert(bm->iln && bm->o_block && bm->n_block);
		if (bm->iln == iln)
			inline_resolve_block_refs(&(iln->refs), bm->o_block, bm->n_block);
	}

#if !defined(NDEBUG)
	if (iln->refs) {
		inline_target_ref *ref;
		ref = iln->refs;
		while (ref) {
			if (!iln->depth || *(ref->ref) != INLINE_RETURN_REFERENCE(iln)) {
				DOLOG( printf("XXX REMAINING REF at depth %d: %p\n", iln->depth,
					   (void*)*(ref->ref)) );
				assert(false);
			}
			ref = ref->next;
		}
	}
#endif
}


static exception_entry * inline_exception_tables(inline_node *iln,
												 exception_entry *n_extable,
												 exception_entry **prevextable)
{
	inline_node *child;
	inline_node *scope;
	exception_entry *et;

	assert(iln);
	assert(n_extable);
	assert(prevextable);

	child = iln->children;
	if (child) {
		do {
			n_extable = inline_exception_tables(child, n_extable, prevextable);
			child = child->next;
		} while (child != iln->children);
	}

	et = iln->jd->exceptiontable;
	for (; et != NULL; et = et->down) {
		assert(et);
		MZERO(n_extable, exception_entry, 1);
		n_extable->start     = inline_map_block(iln, et->start  , iln);
		n_extable->end       = inline_map_block(iln, et->end    , iln);
		n_extable->handler   = inline_map_block(iln, et->handler, iln);
		n_extable->catchtype = et->catchtype;

		if (*prevextable) {
			(*prevextable)->down = n_extable;
		}
		*prevextable = n_extable;

		n_extable++;
	}

	if (iln->handler_monitorexit) {
		exception_entry **activehandlers;

		MZERO(n_extable, exception_entry, 1);
		n_extable->start   = iln->inlined_basicblocks;
		n_extable->end     = iln->inlined_basicblocks_cursor;
		n_extable->handler = iln->handler_monitorexit;
		n_extable->catchtype.any = NULL; /* finally */

		if (*prevextable) {
			(*prevextable)->down = n_extable;
		}
		*prevextable = n_extable;

		n_extable++;

		/* We have to protect the created handler with the same handlers */
		/* that protect the method body itself.                          */

		for (scope = iln; scope->parent != NULL; scope = scope->parent) {

			activehandlers = scope->o_handlers;
			assert(activehandlers);

			while (*activehandlers) {

				assert(scope->parent);

				MZERO(n_extable, exception_entry, 1);
				n_extable->start     = iln->handler_monitorexit;
				n_extable->end       = iln->handler_monitorexit + 1; /* XXX ok in this case? */
				n_extable->handler   = inline_map_block(scope->parent,
														(*activehandlers)->handler,
														scope->parent);
				n_extable->catchtype = (*activehandlers)->catchtype;

				if (*prevextable) {
					(*prevextable)->down = n_extable;
				}
				*prevextable = n_extable;

				n_extable++;
				activehandlers++;
			}
		}
	}

	return n_extable;
}


static void inline_locals(inline_node *iln)
{
	inline_node *child;

	assert(iln);

	iln->varmap = create_variable_map(iln);

	child = iln->children;
	if (child) {
		do {
			inline_locals(child);
			child = child->next;
		} while (child != iln->children);
	}

	if (iln->regdata->memuse > iln->ctx->resultjd->rd->memuse)
		iln->ctx->resultjd->rd->memuse = iln->regdata->memuse;
	if (iln->regdata->argintreguse > iln->ctx->resultjd->rd->argintreguse)
		iln->ctx->resultjd->rd->argintreguse = iln->regdata->argintreguse;
	if (iln->regdata->argfltreguse > iln->ctx->resultjd->rd->argfltreguse)
		iln->ctx->resultjd->rd->argfltreguse = iln->regdata->argfltreguse;
}


static void inline_interface_variables(inline_node *iln)
{
	basicblock *bptr;
	jitdata *resultjd;
	s4 i;
	varinfo *v;

	resultjd = iln->ctx->resultjd;

	resultjd->interface_map = DMNEW(interface_info, 5*iln->ctx->maxinoutdepth);
	for (i=0; i<5*iln->ctx->maxinoutdepth; ++i)
		resultjd->interface_map[i].flags = UNUSED;

	for (bptr = resultjd->basicblocks; bptr != NULL; bptr = bptr->next) {
		assert(bptr->indepth  <= iln->ctx->maxinoutdepth);
		assert(bptr->outdepth <= iln->ctx->maxinoutdepth);

		for (i=0; i<bptr->indepth; ++i) {
			v = &(resultjd->var[bptr->invars[i]]);
			v->flags |= INOUT;
			v->flags &= ~PREALLOC;
			v->flags &= ~INMEMORY;
			assert(bptr->invars[i] >= resultjd->localcount);

			if (resultjd->interface_map[5*i + v->type].flags == UNUSED) {
				resultjd->interface_map[5*i + v->type].flags = v->flags;
			}
			else {
				resultjd->interface_map[5*i + v->type].flags |= v->flags;
			}
		}

		for (i=0; i<bptr->outdepth; ++i) {
			v = &(resultjd->var[bptr->outvars[i]]);
			v->flags |= INOUT;
			v->flags &= ~PREALLOC;
			v->flags &= ~INMEMORY;
			assert(bptr->outvars[i] >= resultjd->localcount);

			if (resultjd->interface_map[5*i + v->type].flags == UNUSED) {
				resultjd->interface_map[5*i + v->type].flags = v->flags;
			}
			else {
				resultjd->interface_map[5*i + v->type].flags |= v->flags;
			}
		}
	}
}


static void inline_write_exception_handlers(inline_node *master, inline_node *iln)
{
	basicblock *n_bptr;
	instruction *n_ins;
	inline_node *child;
	builtintable_entry *bte;
	s4 exvar;
	s4 syncvar;
	varinfo vinfo;
	s4 i;

	child = iln->children;
	if (child) {
		do {
			inline_write_exception_handlers(master, child);
			child = child->next;
		} while (child != iln->children);
	}

	if (iln->synchronize) {
		/* create the monitorexit handler */
		n_bptr = create_block(master, iln, iln, NULL, NULL,
							  iln->n_passthroughcount + 1);
		n_bptr->type = BBTYPE_EXH;
		n_bptr->flags = BBFINISHED;

		exvar = inline_new_variable(master->ctx->resultjd, TYPE_ADR, 0);
		n_bptr->invars[iln->n_passthroughcount] = exvar;

		iln->handler_monitorexit = n_bptr;

		/* ACONST / ALOAD */

		n_ins = master->inlined_iinstr_cursor++;
		if (iln->m->flags & ACC_STATIC) {
			n_ins->opc = ICMD_ACONST;
			n_ins->sx.val.c.cls = iln->m->class;
			n_ins->flags.bits = INS_FLAG_CLASS;
		}
		else {
			n_ins->opc = ICMD_ALOAD;
			n_ins->s1.varindex = iln->synclocal;
			assert(n_ins->s1.varindex != UNUSED);
		}
		/* XXX could be PREALLOCed for  builtin call */
		syncvar = inline_new_variable(master->ctx->resultjd, TYPE_ADR, 0);
		n_ins->dst.varindex = syncvar;
		n_ins->line = 0;

		/* MONITOREXIT */

		bte = builtintable_get_internal(LOCK_monitor_exit);

		n_ins = master->inlined_iinstr_cursor++;
		n_ins->opc = ICMD_BUILTIN;
		n_ins->s1.argcount = 1 + iln->n_passthroughcount + 1;
		n_ins->sx.s23.s2.args = DMNEW(s4, n_ins->s1.argcount);
		n_ins->sx.s23.s2.args[0] = syncvar;
		for (i=0; i < iln->n_passthroughcount + 1; ++i) {
			n_ins->sx.s23.s2.args[1 + i] = n_bptr->invars[i];
		}
		n_ins->sx.s23.s3.bte = bte;
		n_ins->line = 0;

		/* ATHROW */

		n_ins = master->inlined_iinstr_cursor++;
		n_ins->opc = ICMD_ATHROW;
		n_ins->flags.bits = 0;
		n_ins->s1.varindex = exvar;
		n_ins->line = 0;

		/* close basic block */

		close_block(iln, iln, n_bptr, iln->n_passthroughcount);
		n_bptr->icount = 3;
	}
}


/* second pass driver *********************************************************/

static bool test_inlining(inline_node *iln, jitdata *jd,
		jitdata **resultjd)
{
	instruction *n_ins;
	basicblock *n_bb;
	basicblock *n_bptr;
	exception_entry *n_ext;
	exception_entry *prevext;
	codegendata *n_cd;
	jitdata *n_jd;
	s4 i;


	static int debug_verify_inlined_code = 1; /* XXX */
#if !defined(NDEBUG)
	static int debug_compile_inlined_code_counter = 0;
#endif

	DOLOG( dump_inline_tree(iln, 0); );

	assert(iln && jd && resultjd);

	*resultjd = jd;

	n_ins = DMNEW(instruction, iln->cumul_instructioncount);
	iln->inlined_iinstr = n_ins;

	n_bb = DMNEW(basicblock, iln->cumul_basicblockcount);
	MZERO(n_bb, basicblock, iln->cumul_basicblockcount);
	iln->inlined_basicblocks = n_bb;

	iln->ctx->blockmap = DMNEW(inline_block_map, iln->cumul_blockmapcount);

	n_jd = jit_jitdata_new(iln->m);
	n_jd->flags = jd->flags;
	iln->ctx->resultjd = n_jd;

	reg_setup(n_jd);

	/* create the local_map */

	n_jd->local_map = DMNEW(s4, 5*iln->cumul_maxlocals);
	for (i=0; i<5*iln->cumul_maxlocals; ++i)
		n_jd->local_map[i] = UNUSED;

	/* create / coalesce local variables */

	n_jd->varcount = 0;
	n_jd->vartop = 0;
	n_jd->var = NULL;

	inline_locals(iln);

	n_jd->localcount = n_jd->vartop;

	/* extra variables for verification (DEBUG) */

	if (debug_verify_inlined_code) {
		n_jd->vartop   += VERIFIER_EXTRA_LOCALS + VERIFIER_EXTRA_VARS + 100 /* XXX m->maxstack */;
		if (n_jd->vartop > n_jd->varcount) {
			/* XXX why? */
			n_jd->var = DMREALLOC(n_jd->var, varinfo, n_jd->varcount, n_jd->vartop);
			n_jd->varcount = n_jd->vartop;
		}
	}

	/* write inlined code */

	rewrite_method(iln);

	/* create exception handlers */

	inline_write_exception_handlers(iln, iln);

	/* write the dummy end block */

	n_bptr = create_block(iln, iln, iln, NULL, NULL, 0);
	n_bptr->flags = BBUNDEF;
	n_bptr->type = BBTYPE_STD;

	/* store created code in jitdata */

	n_jd->basicblocks = iln->inlined_basicblocks;
	n_jd->basicblockindex = NULL;
	n_jd->instructioncount = iln->cumul_instructioncount;
	n_jd->instructions = iln->inlined_iinstr;

	/* link the basic blocks (dummy end block is not counted) */

	n_jd->basicblockcount = (iln->inlined_basicblocks_cursor - iln->inlined_basicblocks) - 1;
	for (i=0; i<n_jd->basicblockcount + 1; ++i)
		n_jd->basicblocks[i].next = &(n_jd->basicblocks[i+1]);
	if (i)
		n_jd->basicblocks[i-1].next = NULL;

	/* check basicblock numbers */

#if !defined(NDEBUG)
	jit_check_basicblock_numbers(n_jd);
#endif

	/* create the exception table */

	if (iln->cumul_exceptiontablelength) {
		exception_entry *tableend;

		n_ext = DMNEW(exception_entry, iln->cumul_exceptiontablelength);
		prevext = NULL;
		tableend = inline_exception_tables(iln, n_ext, &prevext);
		assert(tableend == n_ext + iln->cumul_exceptiontablelength);
		if (prevext)
			prevext->down = NULL;

		n_jd->exceptiontablelength = iln->cumul_exceptiontablelength;
		n_jd->exceptiontable = n_ext;
	}
	else {
		n_ext = NULL;
	}

	/*******************************************************************************/

	n_cd = n_jd->cd;
	memcpy(n_cd, jd->cd, sizeof(codegendata));

	n_cd->method = NULL; /* XXX */
	n_jd->maxlocals = iln->cumul_maxlocals;
	n_jd->maxinterfaces = iln->ctx->maxinoutdepth;

	inline_post_process(n_jd);

	inline_interface_variables(iln);

#if defined(ENABLE_VERIFIER)
	if (debug_verify_inlined_code) {
		debug_verify_inlined_code = 0;
		DOLOG( printf("VERIFYING INLINED RESULT...\n"); fflush(stdout); );
		if (!typecheck(n_jd)) {
			*exceptionptr = NULL;
			DOLOG( printf("XXX INLINED RESULT DID NOT PASS VERIFIER XXX\n") );
			return false;
		}
		else {
			DOLOG( printf("VERIFICATION PASSED.\n") );
		}
		debug_verify_inlined_code = 1;
	}
#endif /* defined(ENABLE_VERIFIER) */

	/* we need bigger free memory stacks (XXX these should not be allocated in reg_setup) */

	n_jd->rd->freemem = DMNEW(s4, iln->ctx->maxinoutdepth + 1000) /* XXX max vars/block */;
#if defined(HAS_4BYTE_STACKSLOT)
	n_jd->rd->freemem_2 = DMNEW(s4, iln->ctx->maxinoutdepth + 1000) /* XXX max vars/block */;
#endif

#if !defined(NDEBUG)
	if (n_jd->instructioncount >= inline_debug_min_size
			&& n_jd->instructioncount <= inline_debug_max_size)
	{
	   if (debug_compile_inlined_code_counter >= inline_debug_start_counter
			   && debug_compile_inlined_code_counter <= inline_debug_end_counter)
#endif /* NDEBUG */
	   {
			*resultjd = n_jd;

#if !defined(NDEBUG)
			inline_count_methods++;

			/* inline_debug_log++; */
			DOLOG(
			printf("==== %d.INLINE ==================================================================\n", debug_compile_inlined_code_counter);
			printf("\ninline tree:\n");
			dump_inline_tree(iln, 0);
			n_jd->flags |= JITDATA_FLAG_SHOWINTERMEDIATE | JITDATA_FLAG_SHOWDISASSEMBLE;
			/* debug_dump_inlined_code(iln, n_method, n_cd, n_rd); */
			printf("-------- DONE -----------------------------------------------------------\n");
			fflush(stdout);
			);
			/* inline_debug_log--; */
#endif
	   }

#if !defined(NDEBUG)
		debug_compile_inlined_code_counter++;
	}
#endif
	return true;
}


/* first pass: build inlining tree ********************************************/

static bool inline_analyse_callee(inline_node *caller,
								  methodinfo *callee,
								  basicblock *callerblock,
								  instruction *calleriptr,
								  s4 callerpc,
								  exception_entry **handlers,
								  s4 nhandlers)
{
	inline_node *cn;              /* the callee inline_node */
	s4           argi;
	bool         isstatic;
	s4           i, j;
	basicblock  *bptr;
	instruction *iptr;

	/* create an inline tree node */

	cn = DNEW(inline_node);
	MZERO(cn, inline_node, 1);

	cn->ctx = caller->ctx;
	cn->m = callee;
	cn->synchronize = (callee->flags & ACC_SYNCHRONIZED);
	isstatic = (callee->flags & ACC_STATIC);

	/* get the intermediate representation of the callee */

	if (!inline_jit_compile(cn))
		return false;

	/* info about the call site */

	cn->depth = caller->depth + 1;
	cn->callerblock = callerblock;
	cn->callerins = calleriptr;
	cn->callerpc = callerpc;
	cn->o_handlers = handlers;
	cn->n_handlercount = caller->n_handlercount + nhandlers;

	/* determine if we need basic block boundaries before/after */

	cn->blockbefore = false;
	cn->blockafter = false;

	if (cn->jd->branchtoentry)
		cn->blockbefore = true;

	if (cn->jd->branchtoend)
		cn->blockafter = true;

	if (cn->jd->returncount > 1)
		cn->blockafter = true;

	/* XXX make safer and reusable (maybe store last real block) */
	for (bptr = cn->jd->basicblocks; bptr && bptr->next && bptr->next->next; bptr = bptr->next)
		;

	if (cn->jd->returnblock != bptr)
		cn->blockafter = true;

	/* info about the callee */

	cn->localsoffset = caller->localsoffset + caller->m->maxlocals;
	cn->prolog_instructioncount = callee->parseddesc->paramcount + 1;
	cn->epilog_instructioncount = 1; /* INLINE_END */
	cn->extra_instructioncount = 0;

	/* we need a CHECKNULL for instance methods */

	if (!isstatic)
		cn->prolog_instructioncount += 1;

	/* deal with synchronized callees */

	if (cn->synchronize) {
		methoddesc         *md;
		builtintable_entry *bte;

		/* we need basic block boundaries because of the handler */

		cn->blockbefore = true;
		cn->blockafter = true;

		/* for synchronized instance methods */
		/* we need an ASTORE in the prolog   */

		if (!isstatic) {
			cn->prolog_instructioncount += 1;
			cn->localsoffset += 1;
		}

		/* and exception handler */
		/* ALOAD, builtin_monitorexit, ATHROW */

		cn->extra_instructioncount += 3;

		/* exception table entries */

		caller->cumul_exceptiontablelength += 1 + cn->n_handlercount;

		/* we must call the builtins */

		bte = builtintable_get_internal(LOCK_monitor_enter);
		md = bte->md;
		if (md->memuse > cn->regdata->memuse)
			cn->regdata->memuse = md->memuse;
		if (md->argintreguse > cn->regdata->argintreguse)
			cn->regdata->argintreguse = md->argintreguse;

		bte = builtintable_get_internal(LOCK_monitor_exit);
		md = bte->md;
		if (md->memuse > cn->regdata->memuse)
			cn->regdata->memuse = md->memuse;
		if (md->argintreguse > cn->regdata->argintreguse)
			cn->regdata->argintreguse = md->argintreguse;

		caller->ctx->calls_others = true;
	}

	/* determine pass-through variables */

	i = calleriptr->s1.argcount - callee->parseddesc->paramcount; /* max # of pass-though vars */

	cn->n_passthroughvars = DMNEW(s4, i);
	j = 0;
	for (argi = calleriptr->s1.argcount - 1; argi >= callee->parseddesc->paramcount; --argi) {
		s4 idx = calleriptr->sx.s23.s2.args[argi];
		if (idx >= caller->jd->localcount) {
			cn->n_passthroughvars[j] = idx;
			j++;
		}
		else {
			DOLOG( printf("PASSING THROUGH LOCAL VARIABLE %d\n", idx); );
		}
	}
	assert(j <= i);
	cn->n_selfpassthroughcount = j;
	cn->n_passthroughcount = caller->n_passthroughcount + cn->n_selfpassthroughcount;

	/* insert the node into the inline tree */

	insert_inline_node(caller, cn);

	/* analyse recursively */

	if (!inline_inline_intern(callee, cn))
		return false;

	/* subtract one block if we continue the caller block */

	if (!cn->blockbefore)
		cn->cumul_basicblockcount -= 1;

	/* add exception handler block for synchronized callees */

	if (cn->synchronize) {
		caller->ctx->master->cumul_basicblockcount++;
		caller->ctx->master->cumul_blockmapcount++;
	}

	/* cumulate counters */

	caller->cumul_instructioncount += cn->prolog_instructioncount;
	caller->cumul_instructioncount += cn->epilog_instructioncount;
	caller->cumul_instructioncount += cn->extra_instructioncount;
	caller->cumul_instructioncount += cn->cumul_instructioncount - 1 /*invoke*/;

	caller->cumul_basicblockcount += cn->cumul_basicblockcount;
	caller->cumul_blockmapcount += cn->cumul_blockmapcount;
	caller->cumul_exceptiontablelength += cn->cumul_exceptiontablelength;
	if (cn->cumul_maxlocals > caller->cumul_maxlocals)
		caller->cumul_maxlocals = cn->cumul_maxlocals;

	if (caller->cumul_basicblockcount > 10*caller->ctx->master->jd->basicblockcount) {
#if 0
		printf("STOPPING to avoid code explosion (%d blocks)\n",
				caller->cumul_basicblockcount);
#endif
		return false;
	}

	/* XXX extra block after inlined call */
	if (cn->blockafter) {
		caller->cumul_basicblockcount += 1;
		caller->cumul_blockmapcount += 1;
	}

	return true;
}


static bool inline_inline_intern(methodinfo *m, inline_node *iln)
{
	basicblock *bptr;
	s4 len;
	instruction *iptr;
	int opcode;                                   /* invocation opcode */
	methodinfo *callee;
	inline_node *active;
	exception_entry **handlers;
	exception_entry *ex;
	s4 nhandlers;
	jitdata *mjd;
	bool speculative;

	assert(iln);

	mjd = iln->jd;

	/* initialize cumulative counters */

	iln->cumul_maxlocals = iln->localsoffset + m->maxlocals;
	iln->cumul_exceptiontablelength += mjd->exceptiontablelength;

	/* iterate over basic blocks */

	for (bptr = mjd->basicblocks; bptr; bptr = bptr->next) {

		/* ignore dummy end blocks (but count them for the blockmap) */

		iln->cumul_blockmapcount++;
		if (bptr->icount > 0 || bptr->next != NULL)
			iln->cumul_basicblockcount++;

		/* skip dead code */

		if (bptr->flags < BBREACHED)
			continue;

		/* allocate the buffer of active exception handlers */
		/* XXX this wastes some memory, but probably it does not matter */

		handlers = DMNEW(exception_entry*, mjd->exceptiontablelength + 1);

		/* determine the active exception handlers for this block     */
		/* XXX maybe the handlers of a block should be part of our IR */
		/* XXX this should share code with the type checkers          */
		nhandlers = 0;
		for (ex = mjd->exceptiontable; ex ; ex = ex->down) {
			if ((ex->start->nr <= bptr->nr) && (ex->end->nr > bptr->nr)) {
				handlers[nhandlers++] = ex;
			}
		}
		handlers[nhandlers] = NULL;

		len = bptr->icount;
		iptr = bptr->iinstr;

		iln->instructioncount += len;
		iln->cumul_instructioncount += len;

		for (; --len >= 0; ++iptr) {

			opcode = iptr->opc;

			switch (opcode) {
				/****************************************/
				/* INVOKATIONS                          */

				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKEINTERFACE:

					if (!INSTRUCTION_IS_UNRESOLVED(iptr)) {
						callee = iptr->sx.s23.s3.fmiref->p.method;

#if 0
						if (
							(strcmp(callee->class->name->text, "spec/benchmarks/_201_compress/Code_Table") == 0
								&& (strcmp(callee->name->text, "of") == 0
								 || strcmp(callee->name->text, "set") == 0))
							||
							(strcmp(callee->class->name->text, "spec/benchmarks/_201_compress/Compressor") == 0
								&& (strcmp(callee->name->text, "output") == 0))
							||
							(strcmp(callee->class->name->text, "spec/benchmarks/_201_compress/Decompressor") == 0
								&& (strcmp(callee->name->text, "getcode") == 0))
							||
							(strcmp(callee->class->name->text, "spec/benchmarks/_201_compress/Output_Buffer") == 0
								&& (strcmp(callee->name->text, "putbyte") == 0))
							||
							(strcmp(callee->class->name->text, "spec/benchmarks/_201_compress/Input_Buffer") == 0
								&& (strcmp(callee->name->text, "getbyte") == 0
								 || strcmp(callee->name->text, "readbytes") == 0
								 ))
							)
							goto force_inline;

						if (callee->jcodelength > 0)
							goto dont_inline;
#endif

						if (callee->flags & ACC_NATIVE)
							goto dont_inline;

						if (iln->depth >= 3)
							goto dont_inline;

						if ((callee->flags & (ACC_STATIC | ACC_FINAL | ACC_PRIVATE)
									|| opcode == ICMD_INVOKESPECIAL)) {
							speculative = false;
							goto maybe_inline;
						}

						if (callee->flags & ACC_METHOD_MONOMORPHIC) {
							/* XXX */
							if (0
								&& strncmp(callee->class->name->text, "java/", 5) != 0
								&& strncmp(callee->class->name->text, "gnu/", 4) != 0
							   )
							{
								printf("SPECULATIVE INLINE: "); method_println(callee);
								speculative = true;
								goto maybe_inline;
							}
						}

						/* polymorphic call site */
						goto dont_inline;

maybe_inline:
						for (active = iln; active; active = active->parent) {
							if (callee == active->m) {
								DOLOG( printf("RECURSIVE!\n") );
								goto dont_inline;
							}
						}

force_inline:
						if (!inline_analyse_callee(iln, callee,
									bptr,
									iptr,
									iln->instructioncount - len - 1 /* XXX ugly */,
									handlers,
									nhandlers))
							return false;

						if (speculative)
							method_add_assumption_monomorphic(callee, iln->ctx->master->m);
					}
dont_inline:
					break;

				case ICMD_RETURN:
				case ICMD_IRETURN:
				case ICMD_ARETURN:
				case ICMD_LRETURN:
				case ICMD_FRETURN:
				case ICMD_DRETURN:
					/* extra ICMD_MOVE may be necessary */
					iln->cumul_instructioncount++;
					break;
			}
		}

		/* end of basic block */
	}

	return true;
}


/* post processing ************************************************************/

#define POSTPROCESS_SRC(varindex)  live[varindex]--
#define POSTPROCESS_DST(varindex)  live[varindex]++

#define POSTPROCESS_SRCOP(s)  POSTPROCESS_SRC(iptr->s.varindex)
#define POSTPROCESS_DSTOP(d)  POSTPROCESS_DST(iptr->d.varindex)

#define MARKSAVED(varindex)  jd->var[varindex].flags |= SAVEDVAR

#define MARK_ALL_SAVED                                               \
    do {                                                             \
        for (i=0; i<jd->vartop; ++i)                                 \
            if (live[i])                                             \
                MARKSAVED(i);                                        \
    } while (0)

static void inline_post_process(jitdata *jd)
{
	basicblock *bptr;
	instruction *iptr;
	instruction *iend;
	s4 i;
	icmdtable_entry_t *icmdt;
	s4 *live;
	methoddesc *md;
	builtintable_entry *bte;

	/* reset the SAVEDVAR flag of all variables */

	for (i=0; i<jd->vartop; ++i)
		jd->var[i].flags &= ~SAVEDVAR;

	/* allocate the life counters */

	live = DMNEW(s4, jd->vartop);
	MZERO(live, s4, jd->vartop);

	/* iterate over all basic blocks */

	for (bptr = jd->basicblocks; bptr != NULL; bptr = bptr->next) {
		if (bptr->flags < BBREACHED)
			continue;

		/* make invars live */

		for (i=0; i<bptr->indepth; ++i)
			POSTPROCESS_DST(bptr->invars[i]);

		iptr = bptr->iinstr;
		iend = iptr + bptr->icount;

		for (; iptr < iend; ++iptr) {

			icmdt = &(icmd_table[iptr->opc]);

			switch (icmdt->dataflow) {
				case DF_3_TO_0:
					POSTPROCESS_SRCOP(sx.s23.s3);
				case DF_2_TO_0:
					POSTPROCESS_SRCOP(sx.s23.s2);
				case DF_1_TO_0:
					POSTPROCESS_SRCOP(s1);
				case DF_0_TO_0:
					if (icmdt->flags & ICMDTABLE_CALLS) {
						jd->isleafmethod = false;
						MARK_ALL_SAVED;
					}
					break;

				case DF_2_TO_1:
					POSTPROCESS_SRCOP(sx.s23.s2);
				case DF_1_TO_1:
				case DF_MOVE:
					POSTPROCESS_SRCOP(s1);
				case DF_0_TO_1:
					if (icmdt->flags & ICMDTABLE_CALLS) {
						jd->isleafmethod = false;
						MARK_ALL_SAVED;
					}
				case DF_COPY:
					POSTPROCESS_DSTOP(dst);
					break;

				case DF_N_TO_1:
					for (i=0; i<iptr->s1.argcount; ++i) {
						POSTPROCESS_SRC(iptr->sx.s23.s2.args[i]);
					}
					if (icmdt->flags & ICMDTABLE_CALLS) {
						jd->isleafmethod = false;
						MARK_ALL_SAVED;
					}
					POSTPROCESS_DSTOP(dst);
					break;

				case DF_INVOKE:
					INSTRUCTION_GET_METHODDESC(iptr, md);
		post_process_call:
					jd->isleafmethod = false;
					for (i=0; i<md->paramcount; ++i) {
						POSTPROCESS_SRC(iptr->sx.s23.s2.args[i]);
					}
					for (; i<iptr->s1.argcount; ++i) {
						MARKSAVED(iptr->sx.s23.s2.args[i]);
					}
					if (md->returntype.type != TYPE_VOID)
						POSTPROCESS_DSTOP(dst);
					break;

				case DF_BUILTIN:
					bte = iptr->sx.s23.s3.bte;
					md = bte->md;
					goto post_process_call;

				default:
					assert(0);
			}

		} /* end instruction loop */

		/* consume outvars */

		for (i=0; i<bptr->outdepth; ++i)
			POSTPROCESS_SRC(bptr->outvars[i]);

#if !defined(NDEBUG)
		for (i=jd->localcount; i < jd->vartop; ++i)
			assert(live[i] == 0);
#endif

	} /* end basic block loop */
}


/* main driver function *******************************************************/

bool inline_inline(jitdata *jd, jitdata **resultjd)
{
	inline_node *iln;
	methodinfo *m;

	m = jd->m;

	*resultjd = jd;

	DOLOG( printf("==== INLINE ==================================================================\n");
		   show_method(jd, SHOW_STACK); );

	iln = DNEW(inline_node);
	MZERO(iln, inline_node, 1);

	iln->ctx = DNEW(inline_context);
	MZERO(iln->ctx, inline_context, 1);
	iln->ctx->master = iln;
	iln->ctx->calls_others = false;
	iln->m = m;
	iln->jd = jd;
	iln->regdata = jd->rd;
	iln->ctx->next_debugnr = 1; /* XXX debug */

	iln->blockbefore = true;
	iln->blockafter = true;

	/* we cannot use m->instructioncount because it may be greater than
	 * the actual number of instructions in the basic blocks. */
	iln->instructioncount = 0;
	iln->cumul_instructioncount = 0;
	iln->cumul_basicblockcount = 1 /* dummy end block */;

	if (inline_inline_intern(m, iln)) {

		DOLOG( printf("==== TEST INLINE =============================================================\n"); );

		if (iln->children)
			test_inlining(iln, jd, resultjd);
	}

	DOLOG( printf("-------- DONE -----------------------------------------------------------\n");
	fflush(stdout); );

	return true;
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
