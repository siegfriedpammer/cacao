/* src/vm/jit/inline/inline.c - code inliner

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

   $Id: inline.c 4716 2006-03-31 12:38:33Z edwin $

*/

#include "config.h"

#include "vm/types.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "mm/memory.h"
#include "toolbox/logging.h"
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

#include "vm/jit/reg.h"
#include "vm/jit/stack.h"

#include "vm/jit/verify/typecheck.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
# endif
#endif

#ifndef NDEBUG
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

typedef struct inline_node inline_node;
typedef struct inline_target_ref inline_target_ref;
typedef struct inline_context inline_context;
typedef struct inline_stack_translation inline_stack_translation;
typedef struct inline_block_map inline_block_map;

struct inline_node {
	inline_context *ctx;
	
	methodinfo *m;
	inline_node *children;
	inline_node *next;
	inline_node *prev;
	int depth;
	
	/* info about the call site (if depth > 0)*/
	inline_node *parent;
	basicblock *callerblock;
	instruction *callerins;
	s4 callerpc;
	stackptr n_callerstack;
	int n_callerstackdepth;
	stackptr o_callerstack;

	/* info about the callee */
	int localsoffset;
	int prolog_instructioncount;
	int instructioncount;
	int stackcount;
	
	/* cumulative values */
	int cumul_instructioncount;
	int cumul_stackcount;
	int cumul_basicblockcount;
	int cumul_maxstack;
	int cumul_maxlocals;
	int cumul_exceptiontablelength;

	/* output */
	instruction *inlined_iinstr;
	instruction *inlined_iinstr_cursor;
	stackptr n_inlined_stack;
	stackptr n_inlined_stack_cursor;
	basicblock *inlined_basicblocks;
	basicblock *inlined_basicblocks_cursor;

	/* register data */
	registerdata *regdata;

	/* temporary */
	inline_target_ref *refs;
	instruction *inline_start_instruction;
};

struct inline_target_ref {
	inline_target_ref *next;
	basicblock **ref;
	basicblock *target;
};

struct inline_stack_translation {
	stackptr o_sp;
	stackptr n_sp;
};

struct inline_block_map {
	inline_node *iln;
	basicblock *o_block;
	basicblock *n_block;
};

struct inline_context {
	int next_block_number;

	inline_block_map *blockmap;
	int blockmap_index;
	
	stackptr o_translationlimit; /* if a stackptr is smaller than this, look it up in the table */
	stackptr n_debug_stackbase;
	inline_stack_translation *stacktranslationstart;

	inline_stack_translation stacktranslation[1];
};

static int stack_depth(stackptr sp)
{
	int depth = 0;
	while (sp) {
		depth++;
		sp = sp->prev;
	}
	return depth;
}

#ifndef NDEBUG
#include "inline_debug.c"

void inline_print_stats()
{
	printf("inlined callers: %d\n",inline_count_methods);
}
#endif

static bool inline_jit_compile_intern(jitdata *jd)
{
	methodinfo *m;
	
	assert(jd);
	
	/* XXX initialize the static function's class */

	m = jd->m;
	m->isleafmethod = true;

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
	s4 i;

	assert(iln);
	m = iln->m;
	assert(m);

#if defined(USE_THREADS)
	/* enter a monitor on the method */
	builtin_monitorenter((java_objectheader *) m);
#endif
	
	/* XXX dont parse these a second time because parse is not idempotent */
	for (i=0; i<m->jcodelength; ++i) {
		if (m->jcode[i] == JAVA_TABLESWITCH || m->jcode[i] == JAVA_LOOKUPSWITCH) {
			r = false;
			goto return_r;
		}
	}

	/* allocate jitdata structure and fill it */

	jd = DNEW(jitdata);

	jd->m     = m;
	jd->cd    = DNEW(codegendata);
	jd->rd    = DNEW(registerdata);
#if defined(ENABLE_LOOP)
	jd->ld    = DNEW(loopdata);
#endif
	jd->flags = 0;

	/* Allocate codeinfo memory from the heap as we need to keep them. */

	jd->code  = code_codeinfo_new(m); /* XXX check allocation */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp)
# endif
		/* initialize the register allocator */
		reg_setup(jd);
#endif

	/* setup the codegendata memory */

	codegen_setup(jd);

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

return_r:
#if defined(USE_THREADS)
	/* leave the monitor */
	builtin_monitorexit((java_objectheader *) m );
#endif

	return r;
}

static void insert_inline_node(inline_node *parent,inline_node *child)
{
	inline_node *first;
	inline_node *succ;

	assert(parent && child);

	child->parent = parent;

	first = parent->children;
	if (!first) {
		/* insert as only node */
		parent->children = child;
		child->next = child;
		child->prev = child;
		return;
	}

	/* {there is at least one child already there} */

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

static stackptr relocate_stack_ptr_intern(inline_node *iln,stackptr o_link,ptrint curreloc)
{
	inline_stack_translation *tr;
	
	if (o_link) {
		/* XXX should limit range in both directions */
		if (o_link < iln->ctx->o_translationlimit) {
			/* this stack slot is from an earlier chunk, we must look it up */
			tr = iln->ctx->stacktranslationstart;
			while (tr >= iln->ctx->stacktranslation) {
				if (o_link == tr->o_sp) {
					DOLOG(printf("\t\t\ttable lookup %p -> %d\n",(void*)o_link,DEBUG_SLOT(tr->n_sp)));
					return tr->n_sp;
				}
				tr--;
			}
			DOLOG(debug_dump_inline_context(iln));
			DOLOG(printf("\t\tFAILED TO TRANSLATE: %p\n",(void*)o_link));
			assert(false);
		}
		else {
			/* this stack slot it in the most recent chunk */
			assert(curreloc);
			DOLOG( printf("\t\t\toffset %d\n",(int)curreloc) );
			return (stackptr) ((u1*)o_link + curreloc);
		}
	}
	return iln->n_callerstack;
}

/* XXX for debugging */
static stackptr relocate_stack_ptr(inline_node *iln,stackptr o_link,ptrint curreloc)
{
	stackptr new;

	new = relocate_stack_ptr_intern(iln,o_link,curreloc);
	DOLOG(
		printf("\t\treloc %p -> %d (%p)\t(translimit=%p)\n",
				(void*)o_link,DEBUG_SLOT(new),(void*)new,(void*)iln->ctx->o_translationlimit)
	);
	return new;
}

static void emit_instruction(inline_node *iln,instruction *ins,ptrint curreloc,stackptr o_curstack)
{
	char indent[100];
	int i;
	instruction *n_ins;
	inline_target_ref *ref;

	assert(iln && ins);

	for (i=0; i<iln->depth; ++i)
		indent[i] = '\t';
	indent[i] = 0;

	n_ins = (iln->inlined_iinstr_cursor++);
	assert((n_ins - iln->inlined_iinstr) < iln->cumul_instructioncount);
	
	*n_ins = *ins;

	switch (n_ins[0].opc) {
				/****************************************/
				/* VARIABLE ACCESS                      */

			case ICMD_ILOAD:
			case ICMD_IINC:
			case ICMD_FLOAD:
			case ICMD_LLOAD:
			case ICMD_DLOAD:
			case ICMD_ISTORE:
			case ICMD_FSTORE:
			case ICMD_LSTORE:
			case ICMD_DSTORE:
			case ICMD_ALOAD:
			case ICMD_ASTORE:
		    case ICMD_RET:
			n_ins[0].op1 += iln->localsoffset;
			break;

			case ICMD_GOTO:
			case ICMD_IFNULL:
			case ICMD_IFNONNULL:
			case ICMD_IFEQ:
			case ICMD_IFNE:
			case ICMD_IFLT:
			case ICMD_IFGE:
			case ICMD_IFGT:
			case ICMD_IFLE:
			case ICMD_IF_ICMPEQ:
			case ICMD_IF_ICMPNE:
			case ICMD_IF_ICMPLT:
			case ICMD_IF_ICMPGE:
			case ICMD_IF_ICMPGT:
			case ICMD_IF_ICMPLE:
			case ICMD_IF_ACMPEQ:
			case ICMD_IF_ACMPNE:
			case ICMD_IF_LEQ:
			case ICMD_IF_LNE:
			case ICMD_IF_LLT:
			case ICMD_IF_LGE:
			case ICMD_IF_LGT:
			case ICMD_IF_LLE:
			case ICMD_IF_LCMPEQ:
			case ICMD_IF_LCMPNE:
			case ICMD_IF_LCMPLT:
			case ICMD_IF_LCMPGE:
			case ICMD_IF_LCMPGT:
			case ICMD_IF_LCMPLE:
			case ICMD_JSR:
				ref = DNEW(inline_target_ref);
				ref->ref = (basicblock **) &(n_ins[0].target);
				ref->next = iln->refs;
				iln->refs = ref;
				break;

				/****************************************/
				/* RETURNS                              */

			case ICMD_RETURN:
				if (iln->parent) {
					n_ins[0].opc = ICMD_GOTO;
					n_ins[0].dst = NULL;
					goto return_tail;
				}
				break;

			case ICMD_ARETURN:
			case ICMD_IRETURN:
			case ICMD_LRETURN:
			case ICMD_FRETURN:
			case ICMD_DRETURN:
			if (iln->parent) {
				n_ins[0].opc = ICMD_INLINE_GOTO;
				n_ins[0].dst = o_curstack;
return_tail:
				n_ins[0].target = (void *) (ptrint) (iln->depth + 0x333); /* XXX */
				ref = DNEW(inline_target_ref);
				ref->ref = (basicblock **) &(n_ins[0].target);
				ref->next = iln->refs;
				iln->refs = ref;
			}
			break;
	}

	n_ins[0].dst = relocate_stack_ptr(iln,n_ins[0].dst,curreloc);
}

static stackptr emit_inlining_prolog(inline_node *iln,inline_node *callee,stackptr n_curstack,instruction *o_iptr)
{
	methodinfo *calleem;
	methoddesc *md;
	int i;
	int localindex;
	int depth;
	int type;
	instruction *n_ins;

	assert(iln && callee && o_iptr && o_iptr->method == iln->m);

	calleem = callee->m;
	md = calleem->parseddesc;

	localindex = callee->localsoffset + md->paramslots;
	depth = stack_depth(n_curstack) - 1; /* XXX inefficient */
	for (i=md->paramcount-1; i>=0; --i) {
		assert(iln);

		n_ins = (iln->inlined_iinstr_cursor++);
		assert((n_ins - iln->inlined_iinstr) < iln->cumul_instructioncount);

		type = md->paramtypes[i].type;

		localindex -= IS_2_WORD_TYPE(type) ? 2 : 1;
		assert(callee->regdata);

		DOLOG( printf("prologlocal %d type=%d lofs=%d in ",
			   localindex - callee->localsoffset,
			   callee->regdata->locals[localindex - callee->localsoffset][type].type,callee->localsoffset);
				method_println(callee->m); );

		if (callee->regdata->locals[localindex - callee->localsoffset][type].type >= 0) {
			n_ins->opc = ICMD_ISTORE + type;
			n_ins->op1 = localindex;
		}
		else {
			n_ins->opc = IS_2_WORD_TYPE(type) ? ICMD_POP2 : ICMD_POP;
		}
		n_ins->method = iln->m;
		n_ins->line = o_iptr->line;
		assert(n_curstack);
		if (n_curstack->varkind == ARGVAR) {
			n_curstack->varkind = TEMPVAR;
			n_curstack->varnum = depth;
			n_curstack->flags &= ~INMEMORY;
		}
		n_curstack = n_curstack->prev;
		n_ins->dst = n_curstack;
		depth--;
	}

	n_ins = (iln->inlined_iinstr_cursor++);
	assert((n_ins - iln->inlined_iinstr) < iln->cumul_instructioncount);

	n_ins->opc = ICMD_INLINE_START;
	n_ins->method = callee->m;
	n_ins->dst = n_curstack;
	n_ins->line = o_iptr->line;
	n_ins->target = NULL; /* ease debugging */
	iln->inline_start_instruction = n_ins;

	return n_curstack;
}

static void emit_inlining_epilog(inline_node *iln,inline_node *callee,stackptr n_curstack,instruction *o_iptr)
{
	instruction *n_ins;
	
	assert(iln && callee && o_iptr);
	assert(iln->inline_start_instruction);

	n_ins = (iln->inlined_iinstr_cursor++);
	assert((n_ins - iln->inlined_iinstr) < iln->cumul_instructioncount);

	n_ins->opc = ICMD_INLINE_END;
	n_ins->method = callee->m;
	n_ins->dst = n_curstack;
	n_ins->line = o_iptr->line;
	n_ins->target = iln->inline_start_instruction; /* needed for line number table creation */
}

static void rewrite_stack(inline_node *iln,stackptr o_first,stackptr o_last,ptrint curreloc)
{
	int n;
	stackptr o_sp;
	stackptr n_sp;
	
	assert(iln);

	if (!o_first) {
		assert(!o_last);
		DOLOG(printf("rewrite_stack: no stack slots\n"));
		return;
	}

	assert(o_first);
	assert(o_last);
	assert(o_first <= o_last);

	n = o_last - o_first + 1;
	assert(n >= 0);

	o_sp = o_first;
	n_sp = iln->n_inlined_stack_cursor;
	
	DOLOG(
	printf("rewrite_stack: rewriting %d stack slots (%p,%p) -> (%d,%d)\n",
			n,(void*)o_first,(void*)o_last,DEBUG_SLOT(n_sp),
			DEBUG_SLOT(n_sp+n-1))
	);

	DOLOG( printf("o_first = "); debug_dump_stack(o_first); printf("\n") );
	DOLOG( printf("o_last = "); debug_dump_stack(o_last); printf("\n") );
	
	while (o_sp <= o_last) {
		*n_sp = *o_sp;

		n_sp->prev = relocate_stack_ptr(iln,n_sp->prev,curreloc);
		switch (n_sp->varkind) {
			case STACKVAR: n_sp->varnum += iln->n_callerstackdepth; break;
			case LOCALVAR: n_sp->varnum += iln->localsoffset; break;
		}
		
		o_sp++;
		n_sp++;
	}
	DOLOG( printf("n_sp = "); debug_dump_stack(n_sp-1); printf("\n") );
	
	iln->n_inlined_stack_cursor = n_sp;
}

static void inline_resolve_block_refs(inline_target_ref **refs,basicblock *o_bptr,basicblock *n_bptr)
{
	inline_target_ref *ref;
	inline_target_ref *prev;

	ref = *refs;
	prev = NULL;
	while (ref) {
		if (*(ref->ref) == o_bptr) {
			DOLOG(
				if ((ptrint) o_bptr < (0x333+100)) { /* XXX */
					printf("resolving RETURN block reference %p -> new L%03d (%p)\n",
							(void*)o_bptr,n_bptr->debug_nr,(void*)n_bptr);
				}
				else {
					printf("resolving block reference old L%03d (%p) -> new L%03d (%p)\n",
							o_bptr->debug_nr,(void*)o_bptr,n_bptr->debug_nr,(void*)n_bptr);
				}
			);
			
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

static basicblock * create_block(inline_node *iln,basicblock *o_bptr,inline_target_ref **refs,int indepth)
{
	basicblock *n_bptr;
	stackptr n_sp;
	int i;

	assert(iln && o_bptr && refs);
	
	n_bptr = iln->inlined_basicblocks_cursor++;
	assert(n_bptr);
	
	memset(n_bptr,0,sizeof(basicblock));
	n_bptr->mpc = -1;
	
	n_bptr->type = BBTYPE_STD; /* XXX not necessary */
	n_bptr->iinstr = iln->inlined_iinstr_cursor;
	n_bptr->next = n_bptr+1;
	n_bptr->debug_nr = iln->ctx->next_block_number++;
	n_bptr->indepth = indepth;

	if (indepth) {
		/* allocate stackslots */
		iln->n_inlined_stack_cursor += indepth;
		n_sp = iln->n_inlined_stack_cursor - 1;
		n_bptr->instack = n_sp;

		/* link the stack elements */
		for (i=indepth-1; i>=0; --i) {
			n_sp->varkind = STACKVAR;
			n_sp->varnum = i;
			n_sp->prev = (i) ? n_sp-1 : NULL;
			n_sp->flags = 0; /* XXX */
			n_sp--;
		}
	}

	inline_resolve_block_refs(refs,o_bptr,n_bptr);
	
	return n_bptr;
}

static void fill_translation_table(inline_node *iln,stackptr o_sp,stackptr n_sp,int n_depth)
{
	int i;

	DOLOG(
	printf("fill_translation_table (newdepth=%d):\n",n_depth);
	printf("\tos_sp = "); debug_dump_stack(o_sp); printf("\n");
	printf("\tns_sp = "); debug_dump_stack(n_sp); printf("\n");
	);

	/* we must translate all stack slots that were present before the call XXX  */
	/* and the instack of the block */
	iln->ctx->stacktranslationstart = iln->ctx->stacktranslation + (n_depth - 1);

	/* fill the translation table */
	if (n_depth) {
		i = n_depth-1;

		while (o_sp) {
			assert(i >= 0);
			assert(n_sp);
			iln->ctx->stacktranslation[i].o_sp = o_sp;
			iln->ctx->stacktranslation[i].n_sp = n_sp;
			n_sp->flags |= (o_sp->flags & SAVEDVAR); /* XXX correct? */
			n_sp->type = o_sp->type; /* XXX we overwrite this anyway with STACKVAR, right? */
			o_sp = o_sp->prev;
			n_sp = n_sp->prev;
			i--;
		}

		while (n_sp) {
			assert(i >= 0);
			assert(iln->ctx->stacktranslation[i].o_sp);
			iln->ctx->stacktranslation[i].n_sp = n_sp;
			n_sp->flags |= SAVEDVAR; /* XXX this is too conservative */
			n_sp = n_sp->prev;
			i--;
		}

		assert(i == -1);
	}
}

static void rewrite_method(inline_node *iln)
{
	basicblock *o_bptr;
	s4 len;
	instruction *o_iptr;
	instruction *n_iptr;
	stackptr o_dst;
	stackptr n_sp;
	stackptr o_sp;
	stackptr o_curstack;
	stackptr o_nexttorewrite;
	stackptr o_lasttorewrite;
	inline_node *nextcall;
	ptrint curreloc;
	basicblock *n_bptr;
	inline_block_map *bm;
	int i;
	int icount;

	assert(iln);

	n_bptr = NULL;
	nextcall = iln->children;

	/* set memory cursors */
	iln->inlined_iinstr_cursor = iln->inlined_iinstr;
	iln->n_inlined_stack_cursor = iln->n_inlined_stack;
	iln->inlined_basicblocks_cursor = iln->inlined_basicblocks;

	/* loop over basic blocks */
	o_bptr = iln->m->basicblocks;
	for (; o_bptr; o_bptr = o_bptr->next) {

		if (o_bptr->flags < BBREACHED) {
			DOLOG(
			printf("skipping old L%03d (flags=%d,type=%d,os=%p,oid=%d,ois=%p,cursor=%d,callerstack=%d) of ",
					o_bptr->debug_nr,o_bptr->flags,o_bptr->type,
					(void*)o_bptr->stack,o_bptr->indepth,(void*)o_bptr->instack,
					DEBUG_SLOT(iln->n_inlined_stack_cursor),
					DEBUG_SLOT(iln->n_callerstack));
			method_println(iln->m);
			);

			n_bptr = create_block(iln,o_bptr,&(iln->refs),o_bptr->indepth + iln->n_callerstackdepth);
			n_bptr->type = o_bptr->type;
			/* enter it in the blockmap */
			iln->ctx->blockmap[iln->ctx->blockmap_index].iln = iln;
			iln->ctx->blockmap[iln->ctx->blockmap_index].o_block = o_bptr;
			iln->ctx->blockmap[iln->ctx->blockmap_index++].n_block = n_bptr;
			n_bptr->flags = o_bptr->flags;
			continue;
		}

		assert(o_bptr->stack);
		
		len = o_bptr->icount;
		o_iptr = o_bptr->iinstr;

		DOLOG(
		printf("rewriting old L%03d (flags=%d,type=%d,os=%p,oid=%d,ois=%p,cursor=%d,callerstack=%d) of ",
				o_bptr->debug_nr,o_bptr->flags,o_bptr->type,
				(void*)o_bptr->stack,o_bptr->indepth,(void*)o_bptr->instack,
				DEBUG_SLOT(iln->n_inlined_stack_cursor),
				DEBUG_SLOT(iln->n_callerstack));
		method_println(iln->m);
		printf("o_instack: ");debug_dump_stack(o_bptr->instack);printf("\n");
		printf("o_callerstack: ");debug_dump_stack(iln->o_callerstack);printf("\n");
		);

		o_curstack = o_bptr->instack;

		/* create an inlined clone of this block */
		n_bptr = create_block(iln,o_bptr,&(iln->refs),o_bptr->indepth + iln->n_callerstackdepth);
		n_bptr->type = o_bptr->type;
		n_bptr->flags = o_bptr->flags;

		/* enter it in the blockmap */
		iln->ctx->blockmap[iln->ctx->blockmap_index].iln = iln;
		iln->ctx->blockmap[iln->ctx->blockmap_index].o_block = o_bptr;
		iln->ctx->blockmap[iln->ctx->blockmap_index++].n_block = n_bptr;

		DOLOG( debug_dump_inline_context(iln) );

		if (iln->n_callerstackdepth)
			iln->n_callerstack = n_bptr->instack-o_bptr->indepth;
		else
			iln->n_callerstack = NULL;
		fill_translation_table(iln,iln->o_callerstack,iln->n_callerstack,iln->n_callerstackdepth);
		fill_translation_table(iln,o_bptr->instack,n_bptr->instack,n_bptr->indepth);
		iln->ctx->o_translationlimit = o_bptr->stack;

		DOLOG( debug_dump_inline_context(iln) );

		/* calculate the stack element relocation */
		curreloc = (u1*)iln->n_inlined_stack_cursor - (u1*)o_bptr->stack;
		DOLOG( printf("curreloc <- %d = %p - %p\n",(int)curreloc,(void*)iln->n_inlined_stack_cursor,(void*)(u1*)o_bptr->stack) );

		o_nexttorewrite = o_bptr->stack;
		o_lasttorewrite = o_bptr->stack-1;
		assert(o_nexttorewrite);
			
		icount = 0;

		while (--len >= 0) {
			o_dst = o_iptr->dst;

			DOLOG( printf("o_curstack = "); debug_dump_stack(o_curstack); stack_show_icmd(o_iptr,false); printf(", dst = "); debug_dump_stack(o_dst); printf("\n") );

			if (nextcall && o_iptr == nextcall->callerins) {

				/* rewrite stack elements produced so far in this block */
				if (o_nexttorewrite <= o_lasttorewrite) {
					rewrite_stack(iln, o_nexttorewrite, o_lasttorewrite, curreloc);
				}
				
				/* write the inlining prolog */
				n_sp = emit_inlining_prolog(iln,nextcall,relocate_stack_ptr(iln,o_curstack,curreloc),o_iptr);
				icount += nextcall->m->parseddesc->paramcount + 1; /* XXX prolog instructions */

				/* find the first stack slot under the arguments of the invocation */
				o_sp = o_curstack;
				for (i=0; i < nextcall->m->parseddesc->paramcount; ++i) {
					assert(o_sp);
					o_sp = o_sp->prev;
				}
				nextcall->o_callerstack = o_sp;

				/* see how deep the new stack is after the arguments have been removed */
				i = stack_depth(n_sp);
				assert(i == stack_depth(nextcall->o_callerstack) + iln->n_callerstackdepth);

				/* end current block */
				n_bptr->icount = icount;
				n_bptr->outstack = n_sp;
				n_bptr->outdepth = i;
				
				/* caller stack depth for the callee */
				assert(nextcall->n_callerstackdepth == i);
				
				/* set memory pointers in the callee */
				nextcall->inlined_iinstr = iln->inlined_iinstr_cursor;
				nextcall->n_inlined_stack = iln->n_inlined_stack_cursor;
				nextcall->inlined_basicblocks = iln->inlined_basicblocks_cursor;
				
				/* recurse */
				DOLOG( printf("entering inline "); stack_show_icmd(o_iptr,false); printf("\n") );
				rewrite_method(nextcall);
				DOLOG( printf("leaving inline "); stack_show_icmd(o_iptr,false); printf("\n") );

				/* skip stack slots used by the inlined callee */
				curreloc += (u1*)nextcall->n_inlined_stack_cursor - (u1*)iln->n_inlined_stack_cursor;
				
				/* update memory cursors */
				assert(nextcall->inlined_iinstr_cursor == iln->inlined_iinstr_cursor + nextcall->cumul_instructioncount);
				/*assert(nextcall->n_inlined_stack_cursor == iln->n_inlined_stack_cursor + nextcall->cumul_stackcount);*/
				assert(nextcall->inlined_basicblocks_cursor == iln->inlined_basicblocks_cursor + nextcall->cumul_basicblockcount);
				iln->inlined_iinstr_cursor = nextcall->inlined_iinstr_cursor;
				iln->n_inlined_stack_cursor = nextcall->n_inlined_stack_cursor;
				iln->inlined_basicblocks_cursor = nextcall->inlined_basicblocks_cursor;

				/* start new block */
				i = (nextcall->m->parseddesc->returntype.type == TYPE_VOID) ? 0 : 1; /* number of return slots */
				assert(i == 0 || i == 1);
				n_bptr = create_block(iln,(void*) (ptrint) (nextcall->depth + 0x333) /*XXX*/,
						&(nextcall->refs),nextcall->n_callerstackdepth + i);
				n_bptr->flags = o_bptr->flags;
				icount = 0;

				/* skip allocated stack slots */
				curreloc += sizeof(stackelement) * (n_bptr->indepth - i);
				
				/* fill the translation table for the slots present before the call */
				n_sp = n_bptr->instack;
				fill_translation_table(iln,nextcall->o_callerstack,(i) ? n_sp->prev : n_sp,nextcall->n_callerstackdepth);

				/* the return slot */
				if (i) {
					assert(o_dst);
					assert(n_sp);
					fill_translation_table(iln,o_dst,n_sp,nextcall->n_callerstackdepth + 1);

					o_nexttorewrite = o_dst + 1;
					o_lasttorewrite = o_dst;
				}
				else {
					/* the next chunk of stack slots start with (including) the slots produced */
					/* by the invocation */
					o_nexttorewrite = o_lasttorewrite + 1;
					o_lasttorewrite = o_nexttorewrite - 1;
				}
				
				DOLOG( debug_dump_inline_context(iln) );
				iln->ctx->o_translationlimit = o_nexttorewrite;
					
				/* emit inlining epilog */
				emit_inlining_epilog(iln,nextcall,n_sp,o_iptr);
				icount++; /* XXX epilog instructions */

				/* proceed to next call */
				nextcall = nextcall->next;

				DOLOG(
				printf("resuming old L%03d (flags=%d,type=%d,os=%p,oid=%d,ois=%p,cursor=%d,curreloc=%d,callerstack=%d) of ",
						o_bptr->debug_nr,o_bptr->flags,o_bptr->type,
						(void*)o_bptr->stack,o_bptr->indepth,(void*)o_bptr->instack,
						DEBUG_SLOT(iln->n_inlined_stack_cursor),(int)curreloc,
						DEBUG_SLOT(iln->n_callerstack));
				method_println(iln->m);
				);
			}
			else {
				emit_instruction(iln,o_iptr,curreloc,o_curstack);
				icount++;

				if (o_dst > o_lasttorewrite)
					o_lasttorewrite = o_dst;
			}

			DOLOG( printf("o_dst = %p\n",(void*)o_dst) );
			o_curstack = o_dst;
			o_iptr++;
		}

		/* end of basic block */
		/* rewrite stack after last call */
		if (o_nexttorewrite <= o_lasttorewrite) {
			rewrite_stack(iln,o_nexttorewrite,o_lasttorewrite,curreloc);
		}
		n_bptr->outstack = relocate_stack_ptr(iln,o_bptr->outstack,curreloc);
		n_bptr->outdepth = iln->n_callerstackdepth + o_bptr->outdepth;
		assert(n_bptr->outdepth == stack_depth(n_bptr->outstack));
#if 0
		if (n_bptr->outstack) {
			assert(curreloc);
			n_bptr->outstack += curreloc;
		}
#endif
		n_bptr->icount = icount;

		n_iptr = iln->inlined_iinstr_cursor - 1;
		if (n_iptr->opc == ICMD_INLINE_GOTO) {
			DOLOG( printf("creating stack slot for ICMD_INLINE_GOTO\n") );
			n_sp = iln->n_inlined_stack_cursor++;
			assert(n_iptr->dst);
			*n_sp = *n_iptr->dst;
			n_sp->prev = iln->n_callerstack;
			n_iptr->dst = n_sp;

			n_bptr->outdepth = iln->n_callerstackdepth + 1;
			n_bptr->outstack = n_sp;
		}
	}

	/* end of basic blocks */
	if (!iln->depth && n_bptr) {
		n_bptr->next = NULL;
	}

	bm = iln->ctx->blockmap;
	for (i=0; i<iln->ctx->blockmap_index; ++i, ++bm) {
		assert(bm->iln && bm->o_block && bm->n_block);
		if (bm->iln != iln)
			continue;
		inline_resolve_block_refs(&(iln->refs),iln->ctx->blockmap[i].o_block,iln->ctx->blockmap[i].n_block);
	}

#ifndef NDEBUG
	if (iln->refs) {
		inline_target_ref *ref;
		ref = iln->refs;
		while (ref) {
			if (!iln->depth || *(ref->ref) != (void*) (ptrint) (0x333 + iln->depth) /* XXX */) {
				DOLOG( printf("XXX REMAINING REF at depth %d: %p\n",iln->depth,(void*)*(ref->ref)) );
				assert(false);
			}
			ref = ref->next;
		}
	}
#endif
}

static basicblock * inline_map_block(inline_node *iln,basicblock *o_block,inline_node *targetiln)
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

static exceptiontable * inline_exception_tables(inline_node *iln,exceptiontable *n_extable,exceptiontable **prevextable)
{
	inline_node *child;
	exceptiontable *et;
	int i;
	
	assert(iln);
	assert(n_extable);
	assert(prevextable);

	child = iln->children;
	if (child) {
		do {
			n_extable = inline_exception_tables(child,n_extable,prevextable);
			child = child->next;
		} while (child != iln->children);
	}

	et = iln->m->exceptiontable;
	for (i=0; i<iln->m->exceptiontablelength; ++i) {
		assert(et);
		memset(n_extable,0,sizeof(exceptiontable));
		n_extable->startpc = et->startpc;
		n_extable->endpc = et->endpc;
		n_extable->handlerpc = et->handlerpc;
		n_extable->start = inline_map_block(iln,et->start,iln);
		n_extable->end = inline_map_block(iln,et->end,iln);
		n_extable->handler = inline_map_block(iln,et->handler,iln);
		n_extable->catchtype = et->catchtype;

		if (*prevextable) {
			(*prevextable)->down = n_extable;
		}
		*prevextable = n_extable;
		
		n_extable++;
		et++;
	}

	return n_extable;
}

static void inline_locals(inline_node *iln,registerdata *rd)
{
	int i;
	int t;
	inline_node *child;

	assert(iln);
	assert(rd);

	child = iln->children;
	if (child) {
		do {
			inline_locals(child,rd);
			child = child->next;
		} while (child != iln->children);
	}

	assert(iln->regdata);

	for (i=0; i<iln->m->maxlocals; ++i) {
		for (t=TYPE_INT; t<=TYPE_ADR; ++t) {
			DOLOG( printf("local %d type=%d in ",i,iln->regdata->locals[i][t].type); method_println(iln->m); );
			if (iln->regdata->locals[i][t].type >= 0) {
				rd->locals[iln->localsoffset + i][t].type = iln->regdata->locals[i][t].type;
				rd->locals[iln->localsoffset + i][t].flags |= iln->regdata->locals[i][t].flags;
			}
		}
	}

	if (iln->regdata->memuse > rd->memuse)
		rd->memuse = iln->regdata->memuse;
	if (iln->regdata->argintreguse > rd->argintreguse)
		rd->argintreguse = iln->regdata->argintreguse;
	if (iln->regdata->argfltreguse > rd->argfltreguse)
		rd->argfltreguse = iln->regdata->argfltreguse;
}

static void inline_stack_interfaces(inline_node *iln,registerdata *rd)
{
	int i;
	int d;
	basicblock *bptr;
	stackptr sp;

	assert(iln);
	assert(rd);
	assert(rd->interfaces);

	bptr = iln->inlined_basicblocks;
	for (i=0; i<iln->cumul_basicblockcount; ++i, ++bptr) {
		DOLOG( printf("INLINE STACK INTERFACE block L%03d\n",bptr->debug_nr) );
		DOLOG( printf("\toutstack = ");debug_dump_stack(bptr->outstack);printf("\n") );
		DOLOG( printf("\tinstack = ");debug_dump_stack(bptr->outstack);printf("\n") );

		assert(bptr->outdepth == stack_depth(bptr->outstack));
		assert(bptr->indepth == stack_depth(bptr->instack));
		
		sp = bptr->outstack;
		d = bptr->outdepth - 1;
		while (sp) {
			if ((sp->varkind == STACKVAR) && (sp->varnum > d)) {
				sp->varkind = TEMPVAR;
			}
			else {
				sp->varkind = STACKVAR;
				sp->varnum = d;
			}
			DOLOG( printf("INLINE STACK INTERFACE L%03d outstack d=%d varkind=%d varnum=%d type=%d flags=%01x\n",
					bptr->debug_nr,d,sp->varkind,sp->varnum,sp->type,sp->flags) );
			rd->interfaces[d][sp->type].type = sp->type;
			rd->interfaces[d][sp->type].flags |= sp->flags;
			d--;
			sp = sp->prev;
		}

		sp = bptr->instack;
		d = bptr->indepth - 1;
		while (sp) {
			rd->interfaces[d][sp->type].type = sp->type;
			if (sp->varkind == STACKVAR && (sp->flags & SAVEDVAR)) {
				rd->interfaces[d][sp->type].flags |= SAVEDVAR;
			}
			DOLOG( printf("INLINE STACK INTERFACE L%03d instack d=%d varkind=%d varnum=%d type=%d flags=%01x\n",
					bptr->debug_nr,d,sp->varkind,sp->varnum,sp->type,sp->flags) );
			d--;
			sp = sp->prev;
		}
	}
}

static bool inline_inline_intern(methodinfo *m,jitdata *jd, inline_node *iln)
{
	basicblock *bptr;
	s4 len;
	instruction *iptr;
	stackptr o_dst;
	stackptr o_curstack;
	int opcode;                                   /* invocation opcode */
	methodinfo *callee;
	inline_node *calleenode;
	inline_node *active;
	stackptr sp;
	int i;

	assert(jd);
	assert(iln);

	iln->cumul_maxstack = iln->n_callerstackdepth + m->maxstack + 1 /* XXX builtins */;
	iln->cumul_maxlocals = iln->localsoffset + m->maxlocals;
	iln->cumul_exceptiontablelength += m->exceptiontablelength;

	bptr = m->basicblocks;
	for (; bptr; bptr = bptr->next) {

		iln->cumul_basicblockcount++;

		if (bptr->flags < BBREACHED)
			continue;

		assert(bptr->stack);
		
		len = bptr->icount;
		iptr = bptr->iinstr;
		o_curstack = bptr->instack;

		iln->instructioncount += len;
		iln->cumul_instructioncount += len;

#if 0
		printf("ADD INSTRUCTIONS [%d]: %d, count=%d, cumulcount=%d\n",
				iln->depth,len,iln->instructioncount,iln->cumul_instructioncount);
#endif

		while (--len >= 0) {

			opcode = iptr->opc;
			o_dst = iptr->dst;

			switch (opcode) {
				case ICMD_IINC:
					/* XXX we cannot deal with IINC's stack hacking */
					return false;

				case ICMD_LOOKUPSWITCH:
				case ICMD_TABLESWITCH:
					/* XXX these are not implemented, yet. */
					return false;
				
				/****************************************/
				/* INVOKATIONS                          */

				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKEINTERFACE:
					callee = (methodinfo *) iptr[0].val.a;

					if (callee) {
						if ((callee->flags & (ACC_STATIC | ACC_FINAL | ACC_PRIVATE) || opcode == ICMD_INVOKESPECIAL)
						    && !(callee->flags & (ACC_NATIVE | ACC_SYNCHRONIZED))) 
						{
							if (iln->depth < 3) {
								for (active = iln; active; active = active->parent) {
									if (callee == active->m) {
										DOLOG( printf("RECURSIVE!\n") );
										goto dont_inline;
									}
								}
						
								calleenode = DNEW(inline_node);
								memset(calleenode,0,sizeof(inline_node));
								
								calleenode->ctx = iln->ctx;
								calleenode->m = callee;

								if (!inline_jit_compile(calleenode))
									return false;
								
								calleenode->depth = iln->depth+1;
								calleenode->callerblock = bptr;
								calleenode->callerins = iptr;
								calleenode->callerpc = iptr - m->basicblocks->iinstr;
								
								calleenode->localsoffset = iln->localsoffset + m->maxlocals;
								calleenode->prolog_instructioncount = callee->parseddesc->paramcount;

								calleenode->stackcount = callee->stackcount;
								calleenode->cumul_stackcount = callee->stackcount;

								/* see how deep the stack is below the arguments */
								sp = o_curstack;
								for (i=0; sp; sp = sp->prev)
									i++;
								calleenode->n_callerstackdepth = iln->n_callerstackdepth + i - callee->parseddesc->paramcount;

								insert_inline_node(iln,calleenode);

								if (!inline_inline_intern(callee,jd,calleenode))
									return false;

								iln->cumul_instructioncount += calleenode->prolog_instructioncount;
								iln->cumul_instructioncount += calleenode->cumul_instructioncount - 1/*invoke*/ + 2 /*INLINE_START|END*/;
								iln->cumul_stackcount += calleenode->cumul_stackcount;
								iln->cumul_basicblockcount += calleenode->cumul_basicblockcount + 1/*XXX*/;
								iln->cumul_exceptiontablelength += calleenode->cumul_exceptiontablelength;
								if (calleenode->cumul_maxstack > iln->cumul_maxstack)
									iln->cumul_maxstack = calleenode->cumul_maxstack;
								if (calleenode->cumul_maxlocals > iln->cumul_maxlocals)
									iln->cumul_maxlocals = calleenode->cumul_maxlocals;
							}
						}
					}
dont_inline:

					break;
			}

			o_curstack = o_dst;
			++iptr;
		}

		/* end of basic block */
	}	

	return true;
}

static bool test_inlining(inline_node *iln,jitdata *jd,
		methodinfo **resultmethod, jitdata **resultjd)
{
	instruction *n_ins;
	stackptr n_stack;
	basicblock *n_bb;
	methodinfo *n_method;
	exceptiontable *n_ext;
	exceptiontable *prevext;
	codegendata *n_cd;
	registerdata *n_rd;
	jitdata *n_jd;
	

	static int debug_verify_inlined_code = 1;
#ifndef NDEBUG
	static int debug_compile_inlined_code_counter = 0;
#endif

	assert(iln && jd && resultmethod && resultjd);

	*resultmethod = iln->m;
	*resultjd = jd;

#if 0
	if (debug_compile_inlined_code_counter >5)
		return false;
#endif

	n_ins = DMNEW(instruction,iln->cumul_instructioncount);
	iln->inlined_iinstr = n_ins;

	n_stack = DMNEW(stackelement,iln->cumul_stackcount + 1000 /* XXX */);
	iln->n_inlined_stack = n_stack;
	iln->ctx->n_debug_stackbase = n_stack;

	n_bb = DMNEW(basicblock,iln->cumul_basicblockcount);
	iln->inlined_basicblocks = n_bb;

	iln->ctx->blockmap = DMNEW(inline_block_map,iln->cumul_basicblockcount);

	rewrite_method(iln);

	if (iln->cumul_exceptiontablelength) {
		n_ext = DMNEW(exceptiontable,iln->cumul_exceptiontablelength);
		prevext = NULL;
		inline_exception_tables(iln,n_ext,&prevext);
		if (prevext)
			prevext->down = NULL;
	}
	else {
		n_ext = NULL;
	}

	/*******************************************************************************/

	n_method = NEW(methodinfo);
	memcpy(n_method,iln->m,sizeof(methodinfo));
	n_method->maxstack = iln->cumul_maxstack; /* XXX put into cd,rd */
	n_method->maxlocals = iln->cumul_maxlocals;
	n_method->basicblockcount = iln->cumul_basicblockcount;
	n_method->basicblocks = iln->inlined_basicblocks;
	n_method->basicblockindex = NULL;
	n_method->instructioncount = iln->cumul_instructioncount;
	n_method->instructions = iln->inlined_iinstr;
	n_method->stackcount = iln->cumul_stackcount + 1000 /* XXX */;
	n_method->stack = iln->n_inlined_stack;

	n_method->exceptiontablelength = iln->cumul_exceptiontablelength;
	n_method->exceptiontable = n_ext;
	n_method->linenumbercount = 0;

	n_jd = DNEW(jitdata);
	n_jd->flags = 0;
	n_jd->m = n_method;

	n_cd = DNEW(codegendata);
	n_jd->cd = n_cd;
	memcpy(n_cd,jd->cd,sizeof(codegendata));
	n_cd->method = n_method;
	n_cd->maxstack = n_method->maxstack;
	n_cd->maxlocals = n_method->maxlocals;
	n_cd->exceptiontablelength = n_method->exceptiontablelength;
	n_cd->exceptiontable = n_method->exceptiontable;

	n_rd = DNEW(registerdata);
	n_jd->rd = n_rd;
	reg_setup(n_jd);

	iln->regdata = jd->rd;
	inline_locals(iln,n_rd);
	DOLOG( printf("INLINING STACK INTERFACES FOR "); method_println(iln->m) );
	inline_stack_interfaces(iln,n_rd);
	
	if (debug_verify_inlined_code) {
		debug_verify_inlined_code = 0;
		DOLOG( printf("VERIFYING INLINED RESULT...\n") );
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

#ifndef NDEBUG
#if 1
	if (n_method->instructioncount >= inline_debug_min_size && n_method->instructioncount <= inline_debug_max_size) {
	   if (debug_compile_inlined_code_counter >= inline_debug_start_counter 
			   && debug_compile_inlined_code_counter <= inline_debug_end_counter) 
#else
	if (
		(strcmp(n_method->class->name->text,"java/lang/reflect/Array") == 0 &&
		strcmp(n_method->name->text,"<clinit>") == 0 &&
		strcmp(n_method->descriptor->text,"()V") == 0)
		||
		(strcmp(n_method->class->name->text,"java/lang/VMClassLoader") == 0 &&
		strcmp(n_method->name->text,"getSystemClassLoader") == 0 &&
		strcmp(n_method->descriptor->text,"()Ljava/lang/ClassLoader;") == 0)
		) 
	
	{
#endif
#endif /* NDEBUG */
	   {
			*resultmethod = n_method;
			*resultjd = n_jd;

#ifndef NDEBUG
			inline_count_methods++;
			if (inline_debug_log_names)
				method_println(n_method);

			DOLOG(
			printf("==== %d.INLINE ==================================================================\n",debug_compile_inlined_code_counter);
			method_println(n_method);
			stack_show_method(jd);
			dump_inline_tree(iln);
			stack_show_method(n_jd);
			debug_dump_inlined_code(iln,n_method,n_cd,n_rd);
			printf("-------- DONE -----------------------------------------------------------\n");
			fflush(stdout);
			);
#endif
	   }

#ifndef NDEBUG
		debug_compile_inlined_code_counter++;
	}
#endif
	return true;
}

bool inline_inline(jitdata *jd, methodinfo **resultmethod, 
				   jitdata **resultjd)
{
	inline_node *iln;
	methodinfo *m;

	m = jd->m;

#if 0
	printf("==== INLINE ==================================================================\n");
	method_println(m);
#endif
	
	iln = DNEW(inline_node);
	memset(iln,0,sizeof(inline_node));

	iln->ctx = (inline_context *) DMNEW(u1,sizeof(inline_context) + sizeof(inline_stack_translation) * 1000 /* XXX */);
	memset(iln->ctx,0,sizeof(inline_context));
	iln->ctx->stacktranslationstart = iln->ctx->stacktranslation - 1;
	iln->m = m;		

	/* we cannot use m->instructioncount because it may be greater than 
	 * the actual number of instructions in the basic blocks. */
	iln->instructioncount = 0;
	iln->cumul_instructioncount = 0;

	iln->stackcount = m->stackcount;
	iln->cumul_stackcount = m->stackcount;

	if (inline_inline_intern(m,jd,iln)) {
	
#if 0
		printf("==== TEST INLINE =============================================================\n");
		method_println(m);
#endif

		if (iln->children)
			test_inlining(iln,jd,resultmethod,resultjd);
	}

#if 0
	printf("-------- DONE -----------------------------------------------------------\n");
	fflush(stdout);
#endif

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
