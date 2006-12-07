/* vm/jit/replace.c - on-stack replacement of methods

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

   $Id$

*/

#include "config.h"
#include "vm/types.h"

#include <assert.h>
#include <stdlib.h>

#include "arch.h"

#include "mm/memory.h"
#include "toolbox/logging.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/abi.h"
#include "vm/jit/jit.h"
#include "vm/jit/replace.h"
#include "vm/jit/stack.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/disass.h"
#include "vm/jit/show.h"
#include "vm/jit/methodheader.h"

#include "native/include/java_lang_String.h"

#define REPLACE_PATCH_DYNAMIC_CALL
/*#define REPLACE_PATCH_ALL*/


/*** configuration of native stack slot size **********************************/

/* XXX this should be in md-abi.h files, probably */

#if defined(HAS_4BYTE_STACKSLOT)
#define SIZE_OF_STACKSLOT      4
#define STACK_SLOTS_PER_FLOAT  2
typedef u4 stackslot_t;
#else
#define SIZE_OF_STACKSLOT      8
#define STACK_SLOTS_PER_FLOAT  1
typedef u8 stackslot_t;
#endif


/*** debugging ****************************************************************/

/*#define REPLACE_VERBOSE*/

#if !defined(NDEBUG)
static void java_value_print(s4 type, u8 value);
#endif /* !defined(NDEBUG) */

#if !defined(NDEBUG) && defined(REPLACE_VERBOSE)
#define DOLOG(code) do{ if (1) { code; } } while(0)
#define DOLOG_SHORT(code) do{ if (1) { code; } } while(0)
#else
#define DOLOG(code)
#define DOLOG_SHORT(code)
#endif


/*** statistics ***************************************************************/

#define REPLACE_STATISTICS

#if defined(REPLACE_STATISTICS)

static int stat_replacements = 0;
static int stat_frames = 0;
static int stat_recompile = 0;
static int stat_staticpatch = 0;
static int stat_unroll_inline = 0;
static int stat_unroll_call = 0;
static int stat_dist_frames[20] = { 0 };
static int stat_dist_locals[20] = { 0 };
static int stat_dist_locals_adr[10] = { 0 };
static int stat_dist_locals_prim[10] = { 0 };
static int stat_dist_locals_ret[10] = { 0 };
static int stat_dist_locals_void[10] = { 0 };
static int stat_dist_stack[10] = { 0 };
static int stat_dist_stack_adr[10] = { 0 };
static int stat_dist_stack_prim[10] = { 0 };
static int stat_dist_stack_ret[10] = { 0 };
static int stat_methods = 0;
static int stat_rploints = 0;
static int stat_regallocs = 0;
static int stat_dist_method_rplpoints[20] = { 0 };

#define REPLACE_COUNT(cnt)  (cnt)++
#define REPLACE_COUNT_IF(cnt, cond)  do{ if(cond) (cnt)++; } while(0)
#define REPLACE_COUNT_INC(cnt, inc)  ((cnt) += (inc))

#define REPLACE_COUNT_DIST(array, val)                               \
    do {                                                             \
        int limit = (sizeof(array) / sizeof(int)) - 1;               \
        if ((val) < (limit)) (array)[val]++;                         \
        else (array)[limit]++;                                       \
    } while (0)

static void replace_statistics_source_frame(sourceframe_t *frame);

#else

#define REPLACE_COUNT(cnt)
#define REPLACE_COUNT_IF(cnt, cond)
#define REPLACE_COUNT_INC(cnt, inc)
#define REPLACE_COUNT_DIST(array, val)

#endif /* defined(REPLACE_STATISTICS) */


/*** constants used internally ************************************************/

#define TOP_IS_NORMAL    0
#define TOP_IS_ON_STACK  1
#define TOP_IS_IN_ITMP1  2
#define TOP_IS_VOID      3


/******************************************************************************/
/* PART I: Creating / freeing replacement points                              */
/******************************************************************************/


/* replace_create_replacement_point ********************************************
 
   Create a replacement point.
  
   IN:
       jd...............current jitdata
	   iinfo............inlining info for the current position
	   rp...............pre-allocated (uninitialized) rplpoint
	   type.............RPLPOINT_TYPE constant
	   iptr.............current instruction
	   *pra.............current rplalloc pointer
	   javalocals.......the javalocals at the current point
	   stackvars........the stack variables at the current point
	   stackdepth.......the stack depth at the current point
	   paramcount.......number of parameters at the start of stackvars
  
   OUT:
       *rpa.............points to the next free rplalloc
  
*******************************************************************************/

static void replace_create_replacement_point(jitdata *jd,
											 insinfo_inline *iinfo,
											 rplpoint *rp,
											 s4 type,
											 instruction *iptr,
											 rplalloc **pra,
											 s4 *javalocals,
											 s4 *stackvars,
											 s4 stackdepth,
											 s4 paramcount)
{
	rplalloc *ra;
	s4        i;
	varinfo  *v;
	s4        index;

	ra = *pra;

	REPLACE_COUNT(stat_rploints);

	rp->method = (iinfo) ? iinfo->method : jd->m;
	rp->pc = NULL;        /* set by codegen */
	rp->callsize = 0;     /* set by codegen */
	rp->regalloc = ra;
	rp->flags = 0;
	rp->type = type;
	rp->id = iptr->flags.bits >> INS_FLAG_ID_SHIFT;

	/* XXX unify these two fields */
	rp->parent = (iinfo) ? iinfo->rp : NULL;

	/* store local allocation info of javalocals */

	if (javalocals) {
		for (i = 0; i < rp->method->maxlocals; ++i) {
			index = javalocals[i];
			if (index == UNUSED)
				continue;

			ra->index = i;
			if (index < UNUSED) {
				ra->regoff = (UNUSED - index) - 1;
				ra->type = TYPE_RET;
				ra->flags = 0;
			}
			else {
				v = VAR(index);
				ra->flags = v->flags & (INMEMORY);
				ra->regoff = v->vv.regoff;
				ra->type = v->type;
			}
			ra++;
		}
	}

	/* store allocation info of java stack vars */

	for (i = 0; i < stackdepth; ++i) {
		v = VAR(stackvars[i]);
		ra->flags = v->flags & (INMEMORY);
		ra->index = (i < paramcount) ? RPLALLOC_PARAM : RPLALLOC_STACK;
		ra->type  = v->type;
		/* XXX how to handle locals on the stack containing returnAddresses? */
		if (v->type == TYPE_RET) {
			assert(stackvars[i] >= jd->localcount);
			ra->regoff = v->vv.retaddr->nr;
		}
		else
			ra->regoff = v->vv.regoff;
		ra++;
	}

	/* total number of allocations */

	rp->regalloccount = ra - rp->regalloc;

	*pra = ra;
}


/* replace_create_inline_start_replacement_point *******************************

   Create an INLINE_START replacement point.

   IN:
       jd...............current jitdata
	   rp...............pre-allocated (uninitialized) rplpoint
	   iptr.............current instruction
	   *pra.............current rplalloc pointer
	   javalocals.......the javalocals at the current point

   OUT:
       *rpa.............points to the next free rplalloc

   RETURN VALUE:
       the insinfo_inline * for the following inlined body

*******************************************************************************/

static insinfo_inline * replace_create_inline_start_replacement_point(
											 jitdata *jd,
											 rplpoint *rp,
											 instruction *iptr,
											 rplalloc **pra,
											 s4 *javalocals)
{
	insinfo_inline *calleeinfo;
	rplalloc       *ra;

	calleeinfo = iptr->sx.s23.s3.inlineinfo;

	calleeinfo->rp = rp;

	replace_create_replacement_point(jd, calleeinfo->parent, rp,
			RPLPOINT_TYPE_INLINE, iptr, pra,
			javalocals,
			calleeinfo->stackvars, calleeinfo->stackvarscount,
			calleeinfo->paramcount);

	if (calleeinfo->synclocal != UNUSED) {
		ra = (*pra)++;
		ra->index  = RPLALLOC_SYNC;
		ra->regoff = jd->var[calleeinfo->synclocal].vv.regoff;
		ra->flags  = jd->var[calleeinfo->synclocal].flags & INMEMORY;
		ra->type   = TYPE_ADR;

		rp->regalloccount++;
	}

	return calleeinfo;
}


/* replace_create_replacement_points *******************************************
 
   Create the replacement points for the given code.
  
   IN:
       jd...............current jitdata, must not have any replacement points
  
   OUT:
       code->rplpoints.......set to the list of replacement points
	   code->rplpointcount...number of replacement points
	   code->regalloc........list of allocation info
	   code->regalloccount...total length of allocation info list
	   code->globalcount.....number of global allocations at the
	                         start of code->regalloc
  
   RETURN VALUE:
       true.............everything ok 
       false............an exception has been thrown
   
*******************************************************************************/

#define CLEAR_javalocals(array, method)                              \
    do {                                                             \
        for (i=0; i<(method)->maxlocals; ++i)                        \
            (array)[i] = UNUSED;                                     \
    } while (0)

#define COPY_OR_CLEAR_javalocals(dest, array, method)                \
    do {                                                             \
        if ((array) != NULL)                                         \
            MCOPY((dest), (array), s4, (method)->maxlocals);         \
        else                                                         \
            CLEAR_javalocals((dest), (method));                      \
    } while (0)

#define COUNT_javalocals(array, method, counter)                     \
    do {                                                             \
        for (i=0; i<(method)->maxlocals; ++i)                        \
            if ((array)[i] != UNUSED)                                \
				(counter)++;                                         \
    } while (0)

bool replace_create_replacement_points(jitdata *jd)
{
	codeinfo        *code;
	registerdata    *rd;
	basicblock      *bptr;
	int              count;
	methodinfo      *m;
	rplpoint        *rplpoints;
	rplpoint        *rp;
	int              alloccount;
	rplalloc        *regalloc;
	rplalloc        *ra;
	int              i;
	instruction     *iptr;
	instruction     *iend;
	s4              *javalocals;
	s4              *jl;
	methoddesc      *md;
	insinfo_inline  *iinfo;
	s4               startcount;
	s4               firstcount;
#if defined(REPLACE_PATCH_DYNAMIC_CALL)
	bool             needentry;
#endif

	REPLACE_COUNT(stat_methods);

	/* get required compiler data */

	code = jd->code;
	rd   = jd->rd;

	/* assert that we wont overwrite already allocated data */

	assert(code);
	assert(code->m);
	assert(code->rplpoints == NULL);
	assert(code->rplpointcount == 0);
	assert(code->regalloc == NULL);
	assert(code->regalloccount == 0);
	assert(code->globalcount == 0);

	m = code->m;

	/* in instance methods, we may need a rplpoint at the method entry */

#if defined(REPLACE_PATCH_DYNAMIC_CALL)
	if (!(m->flags & ACC_STATIC)) {
		jd->basicblocks[0].bitflags |= BBFLAG_REPLACEMENT;
		needentry = true;
	}
	else {
		needentry = false;
	}
#endif /* defined(REPLACE_PATCH_DYNAMIC_CALL) */

	/* iterate over the basic block list to find replacement points */

	count = 0;
	alloccount = 0;

	javalocals = DMNEW(s4, jd->maxlocals);

	for (bptr = jd->basicblocks; bptr; bptr = bptr->next) {

		/* skip dead code */

		if (bptr->flags < BBFINISHED)
			continue;

		/* get info about this block */

		m = bptr->method;
		iinfo = bptr->inlineinfo;

		/* initialize javalocals at the start of this block */

		COPY_OR_CLEAR_javalocals(javalocals, bptr->javalocals, m);

		/* iterate over the instructions */

		iptr = bptr->iinstr;
		iend = iptr + bptr->icount;
		startcount = count;
		firstcount = count;

		for (; iptr != iend; ++iptr) {
			switch (iptr->opc) {
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKEINTERFACE:
					INSTRUCTION_GET_METHODDESC(iptr, md);
					count++;
					COUNT_javalocals(javalocals, m, alloccount);
					alloccount += iptr->s1.argcount;
					if (iinfo)
						alloccount -= iinfo->throughcount;
					break;

				case ICMD_ISTORE:
				case ICMD_LSTORE:
				case ICMD_FSTORE:
				case ICMD_DSTORE:
				case ICMD_ASTORE:
					stack_javalocals_store(iptr, javalocals);
					break;

				case ICMD_IRETURN:
				case ICMD_LRETURN:
				case ICMD_FRETURN:
				case ICMD_DRETURN:
				case ICMD_ARETURN:
					alloccount += 1;
					/* FALLTHROUGH! */
				case ICMD_RETURN:
					count++;
					break;

				case ICMD_INLINE_START:
					iinfo = iptr->sx.s23.s3.inlineinfo;

					count++;
					COUNT_javalocals(javalocals, m, alloccount);
					alloccount += iinfo->stackvarscount;
					if (iinfo->synclocal != UNUSED)
						alloccount++;

					m = iinfo->method;
					/* javalocals may be set at next block start, or now */
					COPY_OR_CLEAR_javalocals(javalocals, iinfo->javalocals_start, m);
					break;

				case ICMD_INLINE_BODY:
					assert(iinfo == iptr->sx.s23.s3.inlineinfo);

					jl = iinfo->javalocals_start;
					if (jl == NULL) {
						/* get the javalocals from the following block start */
						assert(bptr->next);
						jl = bptr->next->javalocals;
					}
					count++;
					COUNT_javalocals(jl, m, alloccount);
					break;

				case ICMD_INLINE_END:
					assert(iinfo == iptr->sx.s23.s3.inlineinfo ||
						   iinfo == iptr->sx.s23.s3.inlineinfo->parent);
					iinfo = iptr->sx.s23.s3.inlineinfo;
					m = iinfo->outer;
					if (iinfo->javalocals_end)
						MCOPY(javalocals, iinfo->javalocals_end, s4, m->maxlocals);
					iinfo = iinfo->parent;
					break;
			}

			if (iptr == bptr->iinstr)
				firstcount = count;
		} /* end instruction loop */

		/* create replacement points at targets of backward branches */
		/* We only need the replacement point there, if there is no  */
		/* replacement point inside the block.                       */

		if (bptr->bitflags & BBFLAG_REPLACEMENT) {
#if defined(REPLACE_PATCH_DYNAMIC_CALL)
			int test = (needentry && bptr == jd->basicblocks) ? firstcount : count;
#else
			int test = count;
#endif
			if (test > startcount) {
				/* we don't need an extra rplpoint */
				bptr->bitflags &= ~BBFLAG_REPLACEMENT;
			}
			else {
				count++;
				alloccount += bptr->indepth;
				if (bptr->inlineinfo)
					alloccount -= bptr->inlineinfo->throughcount;

				COUNT_javalocals(bptr->javalocals, bptr->method, alloccount);
			}
		}

	} /* end basicblock loop */

	/* if no points were found, there's nothing to do */

	if (!count)
		return true;

	/* allocate replacement point array and allocation array */

	rplpoints = MNEW(rplpoint, count);
	regalloc = MNEW(rplalloc, alloccount);
	ra = regalloc;

	/* initialize replacement point structs */

	rp = rplpoints;

	/* XXX try to share code with the counting loop! */

	for (bptr = jd->basicblocks; bptr; bptr = bptr->next) {
		/* skip dead code */

		if (bptr->flags < BBFINISHED)
			continue;

		/* get info about this block */

		m = bptr->method;
		iinfo = bptr->inlineinfo;

		/* initialize javalocals at the start of this block */

		COPY_OR_CLEAR_javalocals(javalocals, bptr->javalocals, m);

		/* create replacement points at targets of backward branches */

		if (bptr->bitflags & BBFLAG_REPLACEMENT) {

			i = (iinfo) ? iinfo->throughcount : 0;
			replace_create_replacement_point(jd, iinfo, rp++,
					bptr->type, bptr->iinstr, &ra,
					bptr->javalocals, bptr->invars + i, bptr->indepth - i, 0);
		}

		/* iterate over the instructions */

		iptr = bptr->iinstr;
		iend = iptr + bptr->icount;

		for (; iptr != iend; ++iptr) {
			switch (iptr->opc) {
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKEINTERFACE:
					INSTRUCTION_GET_METHODDESC(iptr, md);

					i = (iinfo) ? iinfo->throughcount : 0;
					replace_create_replacement_point(jd, iinfo, rp++,
							RPLPOINT_TYPE_CALL, iptr, &ra,
							javalocals, iptr->sx.s23.s2.args,
							iptr->s1.argcount - i,
							md->paramcount);
					break;

				case ICMD_ISTORE:
				case ICMD_LSTORE:
				case ICMD_FSTORE:
				case ICMD_DSTORE:
				case ICMD_ASTORE:
					stack_javalocals_store(iptr, javalocals);
					break;

				case ICMD_IRETURN:
				case ICMD_LRETURN:
				case ICMD_FRETURN:
				case ICMD_DRETURN:
				case ICMD_ARETURN:
					replace_create_replacement_point(jd, iinfo, rp++,
							RPLPOINT_TYPE_RETURN, iptr, &ra,
							NULL, &(iptr->s1.varindex), 1, 0);
					break;

				case ICMD_RETURN:
					replace_create_replacement_point(jd, iinfo, rp++,
							RPLPOINT_TYPE_RETURN, iptr, &ra,
							NULL, NULL, 0, 0);
					break;

				case ICMD_INLINE_START:
					iinfo = replace_create_inline_start_replacement_point(
								jd, rp++, iptr, &ra, javalocals);
					m = iinfo->method;
					/* javalocals may be set at next block start, or now */
					COPY_OR_CLEAR_javalocals(javalocals, iinfo->javalocals_start, m);
					break;

				case ICMD_INLINE_BODY:
					assert(iinfo == iptr->sx.s23.s3.inlineinfo);

					jl = iinfo->javalocals_start;
					if (jl == NULL) {
						/* get the javalocals from the following block start */
						assert(bptr->next);
						jl = bptr->next->javalocals;
					}
					/* create a non-trappable rplpoint */
					replace_create_replacement_point(jd, iinfo, rp++,
							RPLPOINT_TYPE_BODY, iptr, &ra,
							jl, NULL, 0, 0);
					rp[-1].flags |= RPLPOINT_FLAG_NOTRAP;
					break;

				case ICMD_INLINE_END:
					assert(iinfo == iptr->sx.s23.s3.inlineinfo ||
						   iinfo == iptr->sx.s23.s3.inlineinfo->parent);
					iinfo = iptr->sx.s23.s3.inlineinfo;
					m = iinfo->outer;
					if (iinfo->javalocals_end)
						MCOPY(javalocals, iinfo->javalocals_end, s4, m->maxlocals);
					iinfo = iinfo->parent;
					break;
			}
		} /* end instruction loop */
	} /* end basicblock loop */

	assert((rp - rplpoints) == count);
	assert((ra - regalloc) == alloccount);

	/* store the data in the codeinfo */

	code->rplpoints     = rplpoints;
	code->rplpointcount = count;
	code->regalloc      = regalloc;
	code->regalloccount = alloccount;
	code->globalcount   = 0;
	code->savedintcount = INT_SAV_CNT - rd->savintreguse;
	code->savedfltcount = FLT_SAV_CNT - rd->savfltreguse;
	code->memuse        = rd->memuse;
	code->stackframesize = jd->cd->stackframesize;

	REPLACE_COUNT_DIST(stat_dist_method_rplpoints, count);
	REPLACE_COUNT_INC(stat_regallocs, alloccount);

	/* everything alright */

	return true;
}


/* replace_free_replacement_points *********************************************
 
   Free memory used by replacement points.
  
   IN:
       code.............codeinfo whose replacement points should be freed.
  
*******************************************************************************/

void replace_free_replacement_points(codeinfo *code)
{
	assert(code);

	if (code->rplpoints)
		MFREE(code->rplpoints,rplpoint,code->rplpointcount);

	if (code->regalloc)
		MFREE(code->regalloc,rplalloc,code->regalloccount);

	code->rplpoints = NULL;
	code->rplpointcount = 0;
	code->regalloc = NULL;
	code->regalloccount = 0;
	code->globalcount = 0;
}


/******************************************************************************/
/* PART II: Activating / deactivating replacement points                      */
/******************************************************************************/


/* replace_activate_replacement_points *****************************************
 
   Activate the replacement points of the given compilation unit. When this
   function returns, the replacement points are "armed", so each thread
   reaching one of the points will enter the replacement mechanism.
   
   IN:
       code.............codeinfo of which replacement points should be
						activated
	   mappable.........if true, only mappable replacement points are
						activated
  
*******************************************************************************/

void replace_activate_replacement_points(codeinfo *code, bool mappable)
{
	rplpoint *rp;
	s4        i;
	s4        count;
	s4        index;
	u1       *savedmcode;

	assert(code->savedmcode == NULL);

	/* count trappable replacement points */

	count = 0;
	index = 0;
	i = code->rplpointcount;
	rp = code->rplpoints;
	for (; i--; rp++) {
		if (rp->flags & RPLPOINT_FLAG_NOTRAP)
			continue;

		index++;

		if (mappable && (rp->type == RPLPOINT_TYPE_RETURN))
			continue;

		count++;
	}

	/* allocate buffer for saved machine code */

	savedmcode = MNEW(u1, count * REPLACEMENT_PATCH_SIZE);
	code->savedmcode = savedmcode;
	savedmcode += count * REPLACEMENT_PATCH_SIZE;

	/* activate trappable replacement points */
	/* (in reverse order to handle overlapping points within basic blocks) */

	i = code->rplpointcount;
	rp = code->rplpoints + i;
	while (rp--, i--) {
		assert(!(rp->flags & RPLPOINT_FLAG_ACTIVE));

		if (rp->flags & RPLPOINT_FLAG_NOTRAP)
			continue;

		index--;

		if (mappable && (rp->type == RPLPOINT_TYPE_RETURN))
			continue;

		DOLOG( printf("activate replacement point:\n");
			   replace_replacement_point_println(rp, 1); fflush(stdout); );

		savedmcode -= REPLACEMENT_PATCH_SIZE;

#if (defined(__I386__) || defined(__X86_64__) || defined(__ALPHA__) || defined(__POWERPC__) || defined(__MIPS__)) && defined(ENABLE_JIT)
		md_patch_replacement_point(code, index, rp, savedmcode);
#endif
		rp->flags |= RPLPOINT_FLAG_ACTIVE;
	}

	assert(savedmcode == code->savedmcode);
}


/* replace_deactivate_replacement_points ***************************************
 
   Deactivate a replacement points in the given compilation unit.
   When this function returns, the replacement points will be "un-armed",
   that is a each thread reaching a point will just continue normally.
   
   IN:
       code.............the compilation unit
  
*******************************************************************************/

void replace_deactivate_replacement_points(codeinfo *code)
{
	rplpoint *rp;
	s4        i;
	s4        count;
	u1       *savedmcode;

	assert(code->savedmcode != NULL);
	savedmcode = code->savedmcode;

	/* de-activate each trappable replacement point */

	i = code->rplpointcount;
	rp = code->rplpoints;
	count = 0;
	for (; i--; rp++) {
		if (!(rp->flags & RPLPOINT_FLAG_ACTIVE))
			continue;

		count++;

		DOLOG( printf("deactivate replacement point:\n");
			   replace_replacement_point_println(rp, 1); fflush(stdout); );

#if (defined(__I386__) || defined(__X86_64__) || defined(__ALPHA__) || defined(__POWERPC__) || defined(__MIPS__)) && defined(ENABLE_JIT)
		md_patch_replacement_point(code, -1, rp, savedmcode);
#endif

		rp->flags &= ~RPLPOINT_FLAG_ACTIVE;

		savedmcode += REPLACEMENT_PATCH_SIZE;
	}

	assert(savedmcode == code->savedmcode + count * REPLACEMENT_PATCH_SIZE);

	/* free saved machine code */

	MFREE(code->savedmcode, u1, count * REPLACEMENT_PATCH_SIZE);
	code->savedmcode = NULL;
}


/******************************************************************************/
/* PART III: The replacement mechanism                                        */
/******************************************************************************/


/* replace_read_value **********************************************************

   Read a value with the given allocation from the execution state.
   
   IN:
	   es...............execution state
	   sp...............stack pointer of the execution state (XXX eliminate?)
	   ra...............allocation
	   javaval..........where to put the value

   OUT:
       *javaval.........the value
  
*******************************************************************************/

static void replace_read_value(executionstate_t *es,
							   stackslot_t *sp,
							   rplalloc *ra,
							   u8 *javaval)
{
	if (ra->flags & INMEMORY) {
		/* XXX HAS_4BYTE_STACKSLOT may not be the right discriminant here */
#ifdef HAS_4BYTE_STACKSLOT
		if (IS_2_WORD_TYPE(ra->type)) {
			*javaval = *(u8*)(sp + ra->regoff);
		}
		else {
#endif
			*javaval = sp[ra->regoff];
#ifdef HAS_4BYTE_STACKSLOT
		}
#endif
	}
	else {
		/* allocated register */
		if (IS_FLT_DBL_TYPE(ra->type)) {
			*javaval = es->fltregs[ra->regoff];
		}
		else {
			*javaval = es->intregs[ra->regoff];
		}
	}
}


/* replace_write_value *********************************************************

   Write a value to the given allocation in the execution state.
   
   IN:
	   es...............execution state
	   sp...............stack pointer of the execution state (XXX eliminate?)
	   ra...............allocation
	   *javaval.........the value

*******************************************************************************/

static void replace_write_value(executionstate_t *es,
							    stackslot_t *sp,
							    rplalloc *ra,
							    u8 *javaval)
{
	if (ra->flags & INMEMORY) {
		/* XXX HAS_4BYTE_STACKSLOT may not be the right discriminant here */
#ifdef HAS_4BYTE_STACKSLOT
		if (IS_2_WORD_TYPE(ra->type)) {
			*(u8*)(sp + ra->regoff) = *javaval;
		}
		else {
#endif
			sp[ra->regoff] = *javaval;
#ifdef HAS_4BYTE_STACKSLOT
		}
#endif
	}
	else {
		/* allocated register */
		if (IS_FLT_DBL_TYPE(ra->type)) {
			es->fltregs[ra->regoff] = *javaval;
		}
		else {
			es->intregs[ra->regoff] = *javaval;
		}
	}
}


/* replace_read_executionstate *************************************************

   Read the given executions state and translate it to a source frame.
   
   IN:
       rp...............replacement point at which `es` was taken
	   es...............execution state
	   ss...............where to put the source state

   OUT:
       *ss..............the source state derived from the execution state
  
*******************************************************************************/

static s4 replace_normalize_type_map[] = {
/* RPLPOINT_TYPE_STD    |--> */ RPLPOINT_TYPE_STD,
/* RPLPOINT_TYPE_EXH    |--> */ RPLPOINT_TYPE_STD,
/* RPLPOINT_TYPE_SBR    |--> */ RPLPOINT_TYPE_STD,
/* RPLPOINT_TYPE_CALL   |--> */ RPLPOINT_TYPE_CALL,
/* RPLPOINT_TYPE_INLINE |--> */ RPLPOINT_TYPE_CALL,
/* RPLPOINT_TYPE_RETURN |--> */ RPLPOINT_TYPE_RETURN,
/* RPLPOINT_TYPE_BODY   |--> */ RPLPOINT_TYPE_STD
};

static void replace_read_executionstate(rplpoint *rp,
										executionstate_t *es,
										sourcestate_t *ss,
										bool topframe)
{
	methodinfo    *m;
	codeinfo      *code;
	int            count;
	int            i;
	rplalloc      *ra;
	sourceframe_t *frame;
	int            topslot;
	stackslot_t   *sp;
	stackslot_t   *basesp;

	code = code_find_codeinfo_for_pc(rp->pc);
	m = rp->method;
	topslot = TOP_IS_NORMAL;

	/* stack pointer */

	sp = (stackslot_t *) es->sp;

	/* in some cases the top stack slot is passed in REG_ITMP1 */

	if (rp->type == BBTYPE_EXH) {
		topslot = TOP_IS_IN_ITMP1;
	}

	/* calculate base stack pointer */

	basesp = sp + code_get_stack_frame_size(code);

	/* create the source frame */

	frame = DNEW(sourceframe_t);
	frame->down = ss->frames;
	frame->method = rp->method;
	frame->id = rp->id;
	assert(rp->type >= 0 && rp->type < sizeof(replace_normalize_type_map)/sizeof(s4));
	frame->type = replace_normalize_type_map[rp->type];
	frame->instance = 0;
	frame->syncslotcount = 0;
	frame->syncslots = NULL;
	frame->fromrp = rp;
	frame->fromcode = code;
	frame->torp = NULL;
	frame->tocode = NULL;

	ss->frames = frame;

	/* read local variables */

	count = m->maxlocals;
	frame->javalocalcount = count;
	frame->javalocals = DMNEW(u8, count);
	frame->javalocaltype = DMNEW(u1, count);

#if !defined(NDEBUG)
	/* mark values as undefined */
	for (i=0; i<count; ++i) {
		frame->javalocals[i] = (u8) 0x00dead0000dead00ULL;
		frame->javalocaltype[i] = TYPE_VOID;
	}

	/* some entries in the intregs array are not meaningful */
	/*es->intregs[REG_ITMP3] = (u8) 0x11dead1111dead11ULL;*/
	es->intregs[REG_SP   ] = (u8) 0x11dead1111dead11ULL;
#ifdef REG_PV
	es->intregs[REG_PV   ] = (u8) 0x11dead1111dead11ULL;
#endif
#endif /* !defined(NDEBUG) */

	/* read javalocals */

	count = rp->regalloccount;
	ra = rp->regalloc;

	while (count && (i = ra->index) >= 0) {
		assert(i < m->maxlocals);
		frame->javalocaltype[i] = ra->type;
		if (ra->type == TYPE_RET)
			frame->javalocals[i] = ra->regoff;
		else
			replace_read_value(es, sp, ra, frame->javalocals + i);
		ra++;
		count--;
	}

	/* read instance, if this is the first rplpoint */

#if defined(REPLACE_PATCH_DYNAMIC_CALL)
	if (topframe && !(rp->method->flags & ACC_STATIC) && rp == code->rplpoints) {
		rplalloc instra;
		methoddesc *md;

		md = rp->method->parseddesc;
		assert(md->params);
		assert(md->paramcount >= 1);
		instra.type = TYPE_ADR;
		instra.regoff = md->params[0].regoff;
		if (md->params[0].inmemory) {
			instra.flags = INMEMORY;
			instra.regoff += (1 + code->stackframesize);
		}
		else {
			instra.flags = 0;
		}
		replace_read_value(es, sp, &instra, &(frame->instance));
	}
#endif /* defined(REPLACE_PATCH_DYNAMIC_CALL) */

	/* read stack slots */

	frame->javastackdepth = count;
	frame->javastack = DMNEW(u8, count);
	frame->javastacktype = DMNEW(u1, count);

#if !defined(NDEBUG)
	/* mark values as undefined */
	for (i=0; i<count; ++i) {
		frame->javastack[i] = (u8) 0x00dead0000dead00ULL;
		frame->javastacktype[i] = TYPE_VOID;
	}
#endif /* !defined(NDEBUG) */

	i = 0;

	/* the first stack slot is special in SBR and EXH blocks */

	if (topslot == TOP_IS_ON_STACK) {
		assert(count);

		assert(ra->index == RPLALLOC_STACK);
		frame->javastack[i] = sp[-1];
		frame->javastacktype[i] = TYPE_ADR; /* XXX RET */
		count--;
		i++;
		ra++;
	}
	else if (topslot == TOP_IS_IN_ITMP1) {
		assert(count);

		assert(ra->index == RPLALLOC_STACK);
		frame->javastack[i] = es->intregs[REG_ITMP1];
		frame->javastacktype[i] = TYPE_ADR; /* XXX RET */
		count--;
		i++;
		ra++;
	}
	else if (topslot == TOP_IS_VOID) {
		assert(count);

		assert(ra->index == RPLALLOC_STACK);
		frame->javastack[i] = 0;
		frame->javastacktype[i] = TYPE_VOID;
		count--;
		i++;
		ra++;
	}

	/* read remaining stack slots */

	for (; count--; ra++) {
		if (ra->index == RPLALLOC_SYNC) {
			assert(rp->type == RPLPOINT_TYPE_INLINE);

			/* only read synchronization slots when traversing an inline point */

			if (!topframe) {
				sourceframe_t *calleeframe = frame->down;
				assert(calleeframe);
				assert(calleeframe->syncslotcount == 0);
				assert(calleeframe->syncslots == NULL);

				calleeframe->syncslotcount = 1;
				calleeframe->syncslots = DMNEW(u8, 1);
				replace_read_value(es,sp,ra,calleeframe->syncslots);
			}

			frame->javastackdepth--;
			continue;
		}

		assert(ra->index == RPLALLOC_STACK || ra->index == RPLALLOC_PARAM);

		/* do not read parameters of calls down the call chain */

		if (!topframe && ra->index == RPLALLOC_PARAM) {
			frame->javastackdepth--;
		}
		else {
			if (ra->type == TYPE_RET)
				frame->javastack[i] = ra->regoff;
			else
				replace_read_value(es,sp,ra,frame->javastack + i);
			frame->javastacktype[i] = ra->type;
			i++;
		}
	}
}


/* replace_write_executionstate ************************************************

   Translate the given source state into an execution state.
   
   IN:
       rp...............replacement point for which execution state should be
	                    creates
	   es...............where to put the execution state
	   ss...............the given source state

   OUT:
       *es..............the execution state derived from the source state
  
*******************************************************************************/

static void replace_write_executionstate(rplpoint *rp,
										 executionstate_t *es,
										 sourcestate_t *ss,
										 bool topframe)
{
	methodinfo     *m;
	codeinfo       *code;
	int             count;
	int             i;
	rplalloc       *ra;
	sourceframe_t  *frame;
	int             topslot;
	stackslot_t    *sp;
	stackslot_t    *basesp;

	code = code_find_codeinfo_for_pc(rp->pc);
	m = rp->method;
	topslot = TOP_IS_NORMAL;

	/* pop a source frame */

	frame = ss->frames;
	assert(frame);
	ss->frames = frame->down;

	/* calculate stack pointer */

	sp = (stackslot_t *) es->sp;

	basesp = sp + code_get_stack_frame_size(code);

	/* in some cases the top stack slot is passed in REG_ITMP1 */

	if (rp->type == BBTYPE_EXH) {
		topslot = TOP_IS_IN_ITMP1;
	}

	/* write javalocals */

	ra = rp->regalloc;
	count = rp->regalloccount;

	while (count && (i = ra->index) >= 0) {
		assert(i < m->maxlocals);
		assert(i < frame->javalocalcount);
		assert(ra->type == frame->javalocaltype[i]);
		if (ra->type == TYPE_RET) {
			/* XXX assert that it matches this rplpoint */
		}
		else
			replace_write_value(es, sp, ra, frame->javalocals + i);
		count--;
		ra++;
	}

	/* write stack slots */

	i = 0;

	/* the first stack slot is special in SBR and EXH blocks */

	if (topslot == TOP_IS_ON_STACK) {
		assert(count);

		assert(ra->index == RPLALLOC_STACK);
		assert(i < frame->javastackdepth);
		assert(frame->javastacktype[i] == TYPE_ADR);
		sp[-1] = frame->javastack[i];
		count--;
		i++;
		ra++;
	}
	else if (topslot == TOP_IS_IN_ITMP1) {
		assert(count);

		assert(ra->index == RPLALLOC_STACK);
		assert(i < frame->javastackdepth);
		assert(frame->javastacktype[i] == TYPE_ADR);
		es->intregs[REG_ITMP1] = frame->javastack[i];
		count--;
		i++;
		ra++;
	}
	else if (topslot == TOP_IS_VOID) {
		assert(count);

		assert(ra->index == RPLALLOC_STACK);
		assert(i < frame->javastackdepth);
		assert(frame->javastacktype[i] == TYPE_VOID);
		count--;
		i++;
		ra++;
	}

	/* write remaining stack slots */

	for (; count--; ra++) {
		if (ra->index == RPLALLOC_SYNC) {
			assert(rp->type == RPLPOINT_TYPE_INLINE);

			/* only write synchronization slots when traversing an inline point */

			if (!topframe) {
				assert(frame->down);
				assert(frame->down->syncslotcount == 1); /* XXX need to understand more cases */
				assert(frame->down->syncslots != NULL);

				replace_write_value(es,sp,ra,frame->down->syncslots);
			}
			continue;
		}

		assert(ra->index == RPLALLOC_STACK || ra->index == RPLALLOC_PARAM);

		/* do not write parameters of calls down the call chain */

		if (!topframe && ra->index == RPLALLOC_PARAM) {
			/* skip it */
		}
		else {
			assert(i < frame->javastackdepth);
			assert(ra->type == frame->javastacktype[i]);
			if (ra->type == TYPE_RET) {
				/* XXX assert that it matches this rplpoint */
			}
			else {
				replace_write_value(es,sp,ra,frame->javastack + i);
			}
			i++;
		}
	}

	/* set new pc */

	es->pc = rp->pc;
}


/* replace_pop_activation_record ***********************************************

   Peel a stack frame from the execution state.
   
   *** This function imitates the effects of the method epilog ***
   *** and returning from the method call.                     ***

   IN:
	   es...............execution state
	   frame............source frame, receives synchronization slots

   OUT:
       *es..............the execution state after popping the stack frame
  
*******************************************************************************/

u1* replace_pop_activation_record(executionstate_t *es,
								  sourceframe_t *frame)
{
	u1 *ra;
	u1 *pv;
	s4 reg;
	s4 i;
	s4 count;
	codeinfo *code;
	stackslot_t *basesp;
	stackslot_t *sp;

	assert(es->code);
	assert(frame);

	/* read the return address */

	ra = md_stacktrace_get_returnaddress(es->sp,
			SIZE_OF_STACKSLOT * es->code->stackframesize);

	DOLOG( printf("return address: %p\n", (void*)ra); );

	/* find the new codeinfo */

	pv = md_codegen_get_pv_from_pc(ra);

	DOLOG( printf("PV = %p\n", (void*) pv); );

	if (pv == NULL)
		return NULL;

	code = *(codeinfo **)(pv + CodeinfoPointer);

	DOLOG( printf("CODE = %p\n", (void*) code); );

	if (code == NULL)
		return NULL;

	/* calculate the base of the stack frame */

	sp = (stackslot_t *) es->sp;
	basesp = sp + es->code->stackframesize;

	/* read slots used for synchronization */

	assert(frame->syncslotcount == 0);
	assert(frame->syncslots == NULL);
	count = code_get_sync_slot_count(es->code);
	frame->syncslotcount = count;
	frame->syncslots = DMNEW(u8, count);
	for (i=0; i<count; ++i) {
		frame->syncslots[i] = sp[es->code->memuse + i];
	}

	/* restore saved int registers */

	reg = INT_REG_CNT;
	for (i=0; i<es->code->savedintcount; ++i) {
		while (nregdescint[--reg] != REG_SAV)
			;
		es->intregs[reg] = *--basesp;
	}

	/* restore saved flt registers */

	/* XXX align? */
	reg = FLT_REG_CNT;
	for (i=0; i<es->code->savedfltcount; ++i) {
		while (nregdescfloat[--reg] != REG_SAV)
			;
		basesp -= STACK_SLOTS_PER_FLOAT;
		es->fltregs[reg] = *(u8*)basesp;
	}

	/* Set the new pc. Subtract one so we do not hit the replacement point */
	/* of the instruction following the call, if there is one.             */

	es->pc = ra - 1;

	/* adjust the stackpointer */

	es->sp += SIZE_OF_STACKSLOT * es->code->stackframesize;
	es->sp += SIZE_OF_STACKSLOT; /* skip return address */

	es->pv = pv;
	es->code = code;

#if !defined(NDEBUG)
	/* for debugging */
	for (i=0; i<INT_REG_CNT; ++i)
		if (nregdescint[i] != REG_SAV)
			es->intregs[i] = 0x33dead3333dead33ULL;
	for (i=0; i<FLT_REG_CNT; ++i)
		if (nregdescfloat[i] != REG_SAV)
			es->fltregs[i] = 0x33dead3333dead33ULL;
#endif /* !defined(NDEBUG) */

	return ra;
}


/* replace_patch_future_calls **************************************************

   Analyse a call site and depending on the kind of call patch the call, the
   virtual function table, or the interface table.

   IN:
	   ra...............return address pointing after the call site
	   calleeframe......source frame of the callee
	   calleecode.......the codeinfo of the callee

*******************************************************************************/

void replace_patch_future_calls(u1 *ra, sourceframe_t *calleeframe, codeinfo *calleecode)
{
	methodptr *mpp;
	bool       atentry;
#if !defined(NDEBUG)
	codeinfo  *oldcode;
	codeinfo  *newcode;
#endif
#if defined(REPLACE_VERBOSE)
	s4         i;
	char      *logkind;
	int        disas = 0;
#endif

	assert(ra);
	assert(calleecode);

	mpp = NULL;

	atentry = (calleeframe->down == NULL)
			&& !(calleecode->m->flags & ACC_STATIC)
			&& (calleeframe->fromrp->id == 0); /* XXX */

	DOLOG( printf("bytes at patch position:");
		   for (i=0; i<16; ++i)
			   printf(" %02x", ra[-16+i]);
		   printf("\n"); );

	if (ra[-2] == 0xff && ra[-1] == 0xd1
			&& ra[-7] == 0xb9)
	{
		DOLOG_SHORT( logkind = "static   "; );
		DOLOG( printf("PATCHING static call to "); method_println(calleecode->m); disas = 7; );
		REPLACE_COUNT(stat_staticpatch);
		mpp = (methodptr*)(ra - 6);
	}
#if defined(REPLACE_PATCH_DYNAMIC_CALL)
	else if (ra[-2] == 0xff && ra[-1] == 0xd2
				&& ra[-8] == 0x8b && ra[-7] == 0x91
				&& atentry)
	{
		java_objectheader *obj;
		u1 *table;
		u4 offset;

		DOLOG_SHORT( printf("\tinstance: "); java_value_print(TYPE_ADR, calleeframe->instance);
					 printf("\n"); );

		assert(calleeframe->instance != 0);

		obj = (java_objectheader *) (ptrint) calleeframe->instance;
		table = (u1*) obj->vftbl;
		offset = *(u4*)(ra - 6);

		if (ra[-10] == 0x8b && ra[-9] == 0x08) {
			mpp = (methodptr *) (table + offset);
			DOLOG_SHORT( logkind = "virtual  "; );
			DOLOG( printf("updating virtual call at %p\n", (void*) ra); disas = 8);
		}
		else {
			u4 ioffset = *(u4*)(ra - 12);
			u1 *itable = *(u1**)(table + ioffset);

			assert(ra[-14] == 0x8b && ra[-13] == 0x89);
			mpp = (methodptr *) (itable + offset);
			DOLOG_SHORT( logkind = "interface"; );
			DOLOG( printf("updating interface call at %p\n", (void*) ra); disas = 14);
		}
	}
#endif /* defined(REPLACE_PATCH_DYNAMIC_CALL) */

	if (mpp == NULL)
		return;

	DOLOG(
		u1* u1ptr = ra - disas;
		DISASSINSTR(u1ptr);
		DISASSINSTR(u1ptr);
		if (disas > 8)
			DISASSINSTR(u1ptr);
		fflush(stdout);
	);

	DOLOG( printf("patch method pointer from: %p to %p\n",
				  (void*) *mpp, (void*)calleecode->entrypoint); );

#if !defined(NDEBUG)
	oldcode = *(codeinfo **)((u1*)(*mpp) + CodeinfoPointer);
	newcode = *(codeinfo **)((u1*)(calleecode->entrypoint) + CodeinfoPointer);

	DOLOG_SHORT( printf("\tpatch %s %p ", logkind, (void*) oldcode->entrypoint);
				 method_println(oldcode->m);
				 printf("\t      with      %p ", (void*) newcode->entrypoint);
				 method_println(newcode->m); );

	assert(oldcode->m == newcode->m);
#endif

	/* write the new entrypoint */

	*mpp = (methodptr) calleecode->entrypoint;
}


/* replace_push_activation_record **********************************************

   Push a stack frame onto the execution state.
   
   *** This function imitates the effects of a call and the ***
   *** method prolog of the callee.                         ***

   IN:
	   es...............execution state
	   rpcall...........the replacement point at the call site
	   callerframe......source frame of the caller
	   calleecode.......the codeinfo of the callee
	   calleeframe......source frame of the callee

   OUT:
       *es..............the execution state after pushing the stack frame
  
*******************************************************************************/

void replace_push_activation_record(executionstate_t *es,
									rplpoint *rpcall,
									sourceframe_t *callerframe,
									codeinfo *calleecode,
									sourceframe_t *calleeframe)
{
	s4           reg;
	s4           i;
	s4           count;
	stackslot_t *basesp;
	stackslot_t *sp;
	u1          *ra;

	assert(es);
	assert(rpcall && rpcall->type == RPLPOINT_TYPE_CALL);
	assert(calleecode);
	assert(callerframe);
	assert(calleeframe);
	assert(calleeframe == callerframe->down);

	/* write the return address */

	es->sp -= SIZE_OF_STACKSLOT;

	ra = rpcall->pc + rpcall->callsize;

	DOLOG( printf("writing return address %p to %p\n",
				(void*) ra, (void*) es->sp); );

	*((stackslot_t *)es->sp) = (stackslot_t) ra;

	/* we move into a new code unit */

	es->code = calleecode;

	/* set the new pc XXX not needed */

	es->pc = es->code->entrypoint;

	/* build the stackframe */

	DOLOG( printf("building stackframe of %d words at %p\n",
				es->code->stackframesize, (void*)es->sp); );

	sp = (stackslot_t *) es->sp;
	basesp = sp;

	sp -= es->code->stackframesize;
	es->sp = (u1*) sp;

	/* in debug mode, invalidate stack frame first */

#if !defined(NDEBUG)
	for (i=0; i<(basesp - sp); ++i) {
		sp[i] = 0xdeaddeadU;
	}
#endif

	/* save int registers */

	reg = INT_REG_CNT;
	for (i=0; i<es->code->savedintcount; ++i) {
		while (nregdescint[--reg] != REG_SAV)
			;
		*--basesp = es->intregs[reg];

#if !defined(NDEBUG)
		es->intregs[reg] = 0x44dead4444dead44ULL;
#endif
	}

	/* save flt registers */

	/* XXX align? */
	reg = FLT_REG_CNT;
	for (i=0; i<es->code->savedfltcount; ++i) {
		while (nregdescfloat[--reg] != REG_SAV)
			;
		basesp -= STACK_SLOTS_PER_FLOAT;
		*(u8*)basesp = es->fltregs[reg];

#if !defined(NDEBUG)
		es->fltregs[reg] = 0x44dead4444dead44ULL;
#endif
	}

	/* write slots used for synchronization */

	count = code_get_sync_slot_count(es->code);
	assert(count == calleeframe->syncslotcount);
	for (i=0; i<count; ++i) {
		sp[es->code->memuse + i] = calleeframe->syncslots[i];
	}

	/* set the PV */

	es->pv = es->code->entrypoint;

	/* redirect future invocations */

#if defined(REPLACE_PATCH_ALL)
	if (rpcall->type == callerframe->fromrp->type)
#else
	if (rpcall == callerframe->fromrp)
#endif
		replace_patch_future_calls(ra, calleeframe, calleecode);
}


/* replace_find_replacement_point **********************************************

   Find the replacement point in the given code corresponding to the
   position given in the source frame.
   
   IN:
	   code.............the codeinfo in which to search the rplpoint
	   frame............the source frame defining the position to look for
	   parent...........parent replacement point to match

   RETURN VALUE:
       the replacement point
  
*******************************************************************************/

rplpoint * replace_find_replacement_point(codeinfo *code,
										  sourceframe_t *frame,
										  rplpoint *parent)
{
	methodinfo *m;
	rplpoint *rp;
	s4        i;
	s4        j;
	s4        stacki;
	rplalloc *ra;

	assert(frame);

	DOLOG( printf("searching replacement point for:\n");
		   replace_source_frame_println(frame); );

	m = frame->method;

	DOLOG( printf("code = %p\n", (void*)code); );

	rp = code->rplpoints;
	i = code->rplpointcount;
	while (i--) {
		if (rp->id == frame->id && rp->method == frame->method
				&& rp->parent == parent
				&& replace_normalize_type_map[rp->type] == frame->type)
		{
			/* check if returnAddresses match */
			/* XXX optimize: only do this if JSRs in method */
			DOLOG( printf("checking match for:");
				   replace_replacement_point_println(rp, 1); fflush(stdout); );
			ra = rp->regalloc;
			stacki = 0;
			for (j = rp->regalloccount; j--; ++ra) {
				if (ra->type == TYPE_RET) {
					if (ra->index == RPLALLOC_STACK) {
						assert(stacki < frame->javastackdepth);
						if (frame->javastack[stacki] != ra->regoff)
							goto no_match;
						stacki++;
					}
					else {
						assert(ra->index >= 0 && ra->index < frame->javalocalcount);
						if (frame->javalocals[ra->index] != ra->regoff)
							goto no_match;
					}
				}
			}

			/* found */
			return rp;
		}
no_match:
		rp++;
	}

#if !defined(NDEBUG)
	printf("candidate replacement points were:\n");
	rp = code->rplpoints;
	i = code->rplpointcount;
	for (; i--; ++rp) {
		replace_replacement_point_println(rp, 1);
	}
#endif

	vm_abort("no matching replacement point found");
	return NULL; /* NOT REACHED */
}


/* replace_find_replacement_point_for_pc ***************************************

   Find the nearest replacement point at or before the given PC.

   IN:
       code.............compilation unit the PC is in
	   pc...............the machine code PC

   RETURN VALUE:
       the replacement point found, or
	   NULL if no replacement point was found

*******************************************************************************/

rplpoint *replace_find_replacement_point_for_pc(codeinfo *code, u1 *pc)
{
	rplpoint *found;
	rplpoint *rp;
	s4        i;

	found = NULL;

	rp = code->rplpoints;
	for (i=0; i<code->rplpointcount; ++i, ++rp)
		if (rp->pc <= pc)
			found = rp;

	return found;
}


/* replace_recover_source_state ************************************************

   Recover the source state from the given replacement point and execution
   state.

   IN:
       rp...............replacement point that has been reached
	   es...............execution state at the replacement point rp

   RETURN VALUE:
       the source state

*******************************************************************************/

sourcestate_t *replace_recover_source_state(rplpoint *rp,
										    executionstate_t *es)
{
	sourcestate_t *ss;
	u1            *ra;
#if defined(REPLACE_STATISTICS)
	s4             depth;
#endif

	/* create the source frame structure in dump memory */

	ss = DNEW(sourcestate_t);
	ss->frames = NULL;

	/* each iteration of the loop recovers one source frame */

	depth = 0;

	while (true) {

		/* read the values for this source frame from the execution state */

		DOLOG( printf("recovering source state for%s:\n",
					(ss->frames == NULL) ? " TOPFRAME" : "");
			   replace_replacement_point_println(rp, 1); );

		replace_read_executionstate(rp, es, ss, ss->frames == NULL);

#if defined(REPLACE_STATISTICS)
		REPLACE_COUNT(stat_frames);
		depth++;
		replace_statistics_source_frame(ss->frames);
#endif

		/* unroll to the next (outer) frame */

		if (rp->parent) {
			/* this frame is in inlined code */

			DOLOG( printf("INLINED!\n"); );

			rp = rp->parent;

			assert(rp->type == RPLPOINT_TYPE_INLINE);
			REPLACE_COUNT(stat_unroll_inline);
		}
		else {
			/* this frame had been called at machine-level */

			DOLOG( printf("UNWIND\n"); );

			ra = replace_pop_activation_record(es, ss->frames);
			if (ra == NULL) {
				DOLOG( printf("BREAKING\n"); );
				break;
			}

			rp = replace_find_replacement_point_for_pc(es->code, es->pc);

			if (rp == NULL)
				vm_abort("could not find replacement point while unrolling call");

			DOLOG( printf("found replacement point.\n");
					replace_replacement_point_println(rp, 1); );

			assert(rp->type == RPLPOINT_TYPE_CALL);
			REPLACE_COUNT(stat_unroll_call);
		}
	} /* end loop over source frames */

	REPLACE_COUNT_DIST(stat_dist_frames, depth);

	return ss;
}


/* replace_map_source_state ****************************************************

   Map each source frame in the given source state to a target replacement
   point and compilation unit. If no valid code is available for a source
   frame, it is (re)compiled.

   IN:
       firstcode........XXX temporary hack, will be removed
       ss...............the source state

   OUT:
       ss...............the source state, modified: The `torp` and `tocode`
	                    fields of each source frame are set.

   RETURN VALUE:
       true.............everything went ok
	   false............an exception has been thrown

*******************************************************************************/

static bool replace_map_source_state(codeinfo *firstcode, sourcestate_t *ss)
{
	sourceframe_t *frame;
	codeinfo      *code;
	rplpoint      *rp;
	rplpoint      *parent; /* parent of inlined rplpoint */
#if defined(REPLACE_STATISTICS)
	codeinfo      *oldcode;
#endif

	parent = NULL;

	/* iterate over the source frames from outermost to innermost */

	code = firstcode; /* XXX should get code for first frame */

	frame = ss->frames;
	while (true) {

		/* map this frame */

		rp = replace_find_replacement_point(code, frame, parent);

		frame->tocode = code;
		frame->torp = rp;

		/* go down one frame */

		frame = frame->down;
		if (frame == NULL)
			break;

		if (rp->type == RPLPOINT_TYPE_CALL) {
#if defined(REPLACE_STATISTICS)
			oldcode = frame->method->code;
#endif
			code = jit_get_current_code(frame->method);

			if (code == NULL)
				return false; /* exception */

			REPLACE_COUNT_IF(stat_recompile, code != oldcode);

			parent = NULL;
		}
		else {
			parent = rp;
		}
	}

	return true;
}


/* replace_build_execution_state ***********************************************

   Build an execution state for the given (mapped) source state.

   !!! CAUTION: This function rewrites the machine stack !!!

   IN:
       ss...............the source state. Must have been mapped by
						replace_map_source_state before.
	   es...............the base execution state on which to build

   OUT:
       *es..............the new execution state

*******************************************************************************/

static void replace_build_execution_state(sourcestate_t *ss,
										  executionstate_t *es)
{
	rplpoint      *rp;
	sourceframe_t *prevframe;
	codeinfo      *code;

	while (true) {

		code = ss->frames->tocode;
		rp = ss->frames->torp;

		assert(code);
		assert(rp);
		assert(es->code == code);

		DOLOG( printf("creating execution state for%s:\n",
				(ss->frames->down == NULL) ? " TOPFRAME" : "");
			   replace_replacement_point_println(ss->frames->fromrp, 1);
			   replace_replacement_point_println(rp, 1); );

		prevframe = ss->frames;

		replace_write_executionstate(rp, es, ss, ss->frames->down == NULL);

		if (ss->frames == NULL)
			break;

		DOLOG( replace_executionstate_println(es); );

		if (rp->type == RPLPOINT_TYPE_CALL) {

			DOLOG( printf("pushing activation record for:\n");
				   replace_replacement_point_println(rp, 1); );

			code = ss->frames->tocode;
			replace_push_activation_record(es, rp, prevframe, code, ss->frames);
		}

		DOLOG( replace_executionstate_println(es); );
	}
}


/* replace_me ******************************************************************
 
   This function is called by asm_replacement_out when a thread reaches
   a replacement point. `replace_me` must map the execution state to the
   target replacement point and let execution continue there.

   This function never returns!
  
   IN:
       rp...............replacement point that has been reached
	   es...............execution state read by asm_replacement_out
  
*******************************************************************************/

void replace_me(rplpoint *rp, executionstate_t *es)
{
	sourcestate_t *ss;
	sourceframe_t *frame;
	s4             dumpsize;
	rplpoint      *origrp;

	origrp = rp;
	es->code = code_find_codeinfo_for_pc(rp->pc);

	DOLOG_SHORT( printf("REPLACING(%d %p): (id %d %p) ",
				 stat_replacements, (void*)THREADOBJECT,
				 rp->id, (void*)rp);
				 method_println(es->code->m); );

	DOLOG( replace_replacement_point_println(rp, 1);
		   replace_executionstate_println(es); );

	REPLACE_COUNT(stat_replacements);

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* recover source state */

	ss = replace_recover_source_state(rp, es);

	/* map the source state */

	if (!replace_map_source_state(es->code, ss))
		vm_abort("exception during method replacement");

	DOLOG( replace_sourcestate_println(ss); );

	DOLOG_SHORT( replace_sourcestate_println_short(ss); );

	/* avoid infinite loops by self-replacement */

	frame = ss->frames;
	while (frame->down)
		frame = frame->down;

	if (frame->torp == origrp) {
		DOLOG_SHORT(
			printf("WARNING: identity replacement, turning off rps to avoid infinite loop\n");
		);
		replace_deactivate_replacement_points(frame->tocode);
	}

	/* write execution state of new code */

	DOLOG( replace_executionstate_println(es); );

	replace_build_execution_state(ss, es);

	DOLOG( replace_executionstate_println(es); );

	/* release dump area */

	dump_release(dumpsize);

	/* enter new code */

	DOLOG( printf("JUMPING IN!\n"); fflush(stdout); );

#if (defined(__I386__) || defined(__X86_64__) || defined(__ALPHA__) || defined(__POWERPC__) || defined(__MIPS__)) && defined(ENABLE_JIT)
	asm_replacement_in(es);
#endif
	abort(); /* NOT REACHED */
}


/******************************************************************************/
/* NOTE: No important code below.                                             */
/******************************************************************************/


/* statistics *****************************************************************/

#if defined(REPLACE_STATISTICS)
static void print_freq(FILE *file,int *array,int limit)
{
	int i;
	int sum = 0;
	int cum = 0;
	for (i=0; i<limit; ++i)
		sum += array[i];
	sum += array[limit];
	for (i=0; i<limit; ++i) {
		cum += array[i];
		fprintf(file,"      %3d: %8d (cum %3d%%)\n",
				i, array[i], (sum) ? ((100*cum)/sum) : 0);
	}
	fprintf(file,"    >=%3d: %8d\n",limit,array[limit]);
}
#endif /* defined(REPLACE_STATISTICS) */


#if defined(REPLACE_STATISTICS)

#define REPLACE_PRINT_DIST(name, array)                              \
    printf("    " name " distribution:\n");                          \
    print_freq(stdout, (array), sizeof(array)/sizeof(int) - 1);

void replace_print_statistics(void)
{
	printf("replacement statistics:\n");
	printf("    # of replacements:   %d\n", stat_replacements);
	printf("    # of frames:         %d\n", stat_frames);
	printf("    # of recompilations: %d\n", stat_recompile);
	printf("    patched static calls:%d\n", stat_staticpatch);
	printf("    unrolled inlines:    %d\n", stat_unroll_inline);
	printf("    unrolled calls:      %d\n", stat_unroll_call);
	REPLACE_PRINT_DIST("frame depth", stat_dist_frames);
	REPLACE_PRINT_DIST("locals per frame", stat_dist_locals);
	REPLACE_PRINT_DIST("ADR locals per frame", stat_dist_locals_adr);
	REPLACE_PRINT_DIST("primitive locals per frame", stat_dist_locals_prim);
	REPLACE_PRINT_DIST("RET locals per frame", stat_dist_locals_ret);
	REPLACE_PRINT_DIST("void locals per frame", stat_dist_locals_void);
	REPLACE_PRINT_DIST("stack slots per frame", stat_dist_stack);
	REPLACE_PRINT_DIST("ADR stack slots per frame", stat_dist_stack_adr);
	REPLACE_PRINT_DIST("primitive stack slots per frame", stat_dist_stack_prim);
	REPLACE_PRINT_DIST("RET stack slots per frame", stat_dist_stack_ret);
	printf("\n");
	printf("    # of methods:            %d\n", stat_methods);
	printf("    # of replacement points: %d\n", stat_rploints);
	printf("    # of regallocs:          %d\n", stat_regallocs);
	printf("        per rplpoint:        %f\n", (double)stat_regallocs / stat_rploints);
	printf("        per method:          %f\n", (double)stat_regallocs / stat_methods);
	REPLACE_PRINT_DIST("replacement points per method", stat_dist_method_rplpoints);
	printf("\n");

}
#endif /* defined(REPLACE_STATISTICS) */


#if defined(REPLACE_STATISTICS)
static void replace_statistics_source_frame(sourceframe_t *frame)
{
	int adr = 0;
	int ret = 0;
	int prim = 0;
	int vd = 0;
	int n = 0;
	int i;

	for (i=0; i<frame->javalocalcount; ++i) {
		switch (frame->javalocaltype[i]) {
			case TYPE_ADR: adr++; break;
			case TYPE_RET: ret++; break;
			case TYPE_INT: case TYPE_LNG: case TYPE_FLT: case TYPE_DBL: prim++; break;
			case TYPE_VOID: vd++; break;
			default: assert(0);
		}
		n++;
	}
	REPLACE_COUNT_DIST(stat_dist_locals, n);
	REPLACE_COUNT_DIST(stat_dist_locals_adr, adr);
	REPLACE_COUNT_DIST(stat_dist_locals_void, vd);
	REPLACE_COUNT_DIST(stat_dist_locals_ret, ret);
	REPLACE_COUNT_DIST(stat_dist_locals_prim, prim);
	adr = ret = prim = n = 0;
	for (i=0; i<frame->javastackdepth; ++i) {
		switch (frame->javastacktype[i]) {
			case TYPE_ADR: adr++; break;
			case TYPE_RET: ret++; break;
			case TYPE_INT: case TYPE_LNG: case TYPE_FLT: case TYPE_DBL: prim++; break;
		}
		n++;
	}
	REPLACE_COUNT_DIST(stat_dist_stack, n);
	REPLACE_COUNT_DIST(stat_dist_stack_adr, adr);
	REPLACE_COUNT_DIST(stat_dist_stack_ret, ret);
	REPLACE_COUNT_DIST(stat_dist_stack_prim, prim);
}
#endif /* defined(REPLACE_STATISTICS) */


/* debugging helpers **********************************************************/

/* replace_replacement_point_println *******************************************
 
   Print replacement point info.
  
   IN:
       rp...............the replacement point to print
  
*******************************************************************************/

#if !defined(NDEBUG)

#define TYPECHAR(t)  (((t) >= 0 && (t) <= TYPE_RET) ? show_jit_type_letters[t] : '?')

static char *replace_type_str[] = {
	"STD",
	"EXH",
	"SBR",
	"CALL",
	"INLINE",
	"RETURN",
	"BODY"
};

void replace_replacement_point_println(rplpoint *rp, int depth)
{
	int j;
	int index;

	if (!rp) {
		printf("(rplpoint *)NULL\n");
		return;
	}

	for (j=0; j<depth; ++j)
		putchar('\t');

	printf("rplpoint (id %d) %p pc:%p+%d type:%s",
			rp->id, (void*)rp,rp->pc,rp->callsize,
			replace_type_str[rp->type]);
	if (rp->flags & RPLPOINT_FLAG_NOTRAP)
		printf(" NOTRAP");
	if (rp->flags & RPLPOINT_FLAG_ACTIVE)
		printf(" ACTIVE");
	printf(" parent:%p\n", (void*)rp->parent);
	for (j=0; j<depth; ++j)
		putchar('\t');
	printf("ra:%d = [",	rp->regalloccount);

	for (j=0; j<rp->regalloccount; ++j) {
		if (j)
			putchar(' ');
		index = rp->regalloc[j].index;
		switch (index) {
			case RPLALLOC_STACK: printf("S"); break;
			case RPLALLOC_PARAM: printf("P"); break;
			case RPLALLOC_SYNC : printf("Y"); break;
			default: printf("%d", index);
		}
		printf(":%1c:", TYPECHAR(rp->regalloc[j].type));
		if (rp->regalloc[j].type == TYPE_RET) {
			printf("ret(L%03d)", rp->regalloc[j].regoff);
		}
		else {
			show_allocation(rp->regalloc[j].type, rp->regalloc[j].flags, rp->regalloc[j].regoff);
		}
	}

	printf("]\n");
	for (j=0; j<depth; ++j)
		putchar('\t');
	printf("method: ");
	method_print(rp->method);

	printf("\n");
}
#endif /* !defined(NDEBUG) */


/* replace_show_replacement_points *********************************************
 
   Print replacement point info.
  
   IN:
       code.............codeinfo whose replacement points should be printed.
  
*******************************************************************************/

#if !defined(NDEBUG)
void replace_show_replacement_points(codeinfo *code)
{
	int i;
	int depth;
	rplpoint *rp;
	rplpoint *parent;

	if (!code) {
		printf("(codeinfo *)NULL\n");
		return;
	}

	printf("\treplacement points: %d\n",code->rplpointcount);

	printf("\ttotal allocations : %d\n",code->regalloccount);
	printf("\tsaved int regs    : %d\n",code->savedintcount);
	printf("\tsaved flt regs    : %d\n",code->savedfltcount);
	printf("\tmemuse            : %d\n",code->memuse);

	printf("\n");

	for (i=0; i<code->rplpointcount; ++i) {
		rp = code->rplpoints + i;

		depth = 1;
		parent = rp->parent;
		while (parent) {
			depth++;
			parent = parent->parent;
		}
		replace_replacement_point_println(rp, depth);
	}
}
#endif


/* replace_executionstate_println **********************************************
 
   Print execution state
  
   IN:
       es...............the execution state to print
  
*******************************************************************************/

#if !defined(NDEBUG)
void replace_executionstate_println(executionstate_t *es)
{
	int i;
	int slots;
	stackslot_t *sp;
	int extraslots;

	if (!es) {
		printf("(executionstate_t *)NULL\n");
		return;
	}

	printf("executionstate_t:\n");
	printf("\tpc = %p",(void*)es->pc);
	printf("  sp = %p",(void*)es->sp);
	printf("  pv = %p\n",(void*)es->pv);
#if defined(ENABLE_DISASSEMBLER)
	for (i=0; i<INT_REG_CNT; ++i) {
		if (i%4 == 0)
			printf("\t");
		else
			printf(" ");
		printf("%-3s = %016llx",regs[i],(unsigned long long)es->intregs[i]);
		if (i%4 == 3)
			printf("\n");
	}
	for (i=0; i<FLT_REG_CNT; ++i) {
		if (i%4 == 0)
			printf("\t");
		else
			printf(" ");
		printf("F%02d = %016llx",i,(unsigned long long)es->fltregs[i]);
		if (i%4 == 3)
			printf("\n");
	}
#endif

	sp = (stackslot_t *) es->sp;

	extraslots = 2;

	if (es->code) {
		methoddesc *md = es->code->m->parseddesc;
		slots = code_get_stack_frame_size(es->code);
		extraslots = 1 + md->memuse;
	}
	else
		slots = 0;


	if (slots) {
		printf("\tstack slots(+%d) at sp:", extraslots);
		for (i=0; i<slots+extraslots; ++i) {
			if (i%4 == 0)
				printf("\n\t\t");
			printf("M%02d%c", i, (i >= slots) ? '(' : ' ');
#ifdef HAS_4BYTE_STACKSLOT
			printf("%08lx",(unsigned long)*sp++);
#else
			printf("%016llx",(unsigned long long)*sp++);
#endif
			printf("%c", (i >= slots) ? ')' : ' ');
		}
		printf("\n");
	}

	printf("\tcode: %p", (void*)es->code);
	if (es->code != NULL) {
		printf(" stackframesize=%d ", es->code->stackframesize);
		method_print(es->code->m);
	}
	printf("\n");

	printf("\n");
}
#endif

#if !defined(NDEBUG)
static void java_value_print(s4 type, u8 value)
{
	java_objectheader *obj;
	utf               *u;

	printf("%016llx",(unsigned long long) value);

	if (type < 0 || type > TYPE_RET)
		printf(" <INVALID TYPE:%d>", type);
	else
		printf(" %s", show_jit_type_names[type]);

	if (type == TYPE_ADR && value != 0) {
		obj = (java_objectheader *) (ptrint) value;
		putchar(' ');
		utf_display_printable_ascii_classname(obj->vftbl->class->name);

		if (obj->vftbl->class == class_java_lang_String) {
			printf(" \"");
			u = javastring_toutf((java_lang_String *)obj, false);
			utf_display_printable_ascii(u);
			printf("\"");
		}
	}
	else if (type == TYPE_INT || type == TYPE_LNG) {
		printf(" %lld", (long long) value);
	}
}
#endif /* !defined(NDEBUG) */


#if !defined(NDEBUG)
void replace_source_frame_println(sourceframe_t *frame)
{
	s4 i;
	s4 t;

	printf("\t");
	method_println(frame->method);
	printf("\tid: %d\n", frame->id);
	printf("\ttype: %s\n", replace_type_str[frame->type]);
	printf("\n");

	if (frame->instance) {
		printf("\tinstance: ");
		java_value_print(TYPE_ADR, frame->instance);
		printf("\n");
	}

	if (frame->javalocalcount) {
		printf("\tlocals (%d):\n",frame->javalocalcount);
		for (i=0; i<frame->javalocalcount; ++i) {
			t = frame->javalocaltype[i];
			if (t == TYPE_VOID) {
				printf("\tlocal[ %2d] = void\n",i);
			}
			else {
				printf("\tlocal[%c%2d] = ",TYPECHAR(t),i);
				java_value_print(t, frame->javalocals[i]);
				printf("\n");
			}
		}
		printf("\n");
	}

	if (frame->javastackdepth) {
		printf("\tstack (depth %d):\n",frame->javastackdepth);
		for (i=0; i<frame->javastackdepth; ++i) {
			t = frame->javastacktype[i];
			if (t == TYPE_VOID) {
				printf("\tstack[%2d] = void", i);
			}
			else {
				printf("\tstack[%2d] = ",i);
				java_value_print(frame->javastacktype[i], frame->javastack[i]);
				printf("\n");
			}
		}
		printf("\n");
	}

	if (frame->syncslotcount) {
		printf("\tsynchronization slots (%d):\n",frame->syncslotcount);
		for (i=0; i<frame->syncslotcount; ++i) {
			printf("\tslot[%2d] = ",i);
#ifdef HAS_4BYTE_STACKSLOT
			printf("%08lx\n",(unsigned long) frame->syncslots[i]);
#else
			printf("%016llx\n",(unsigned long long) frame->syncslots[i]);
#endif
		}
		printf("\n");
	}

	if (frame->fromcode) {
		printf("\tfrom %p ", (void*)frame->fromcode);
		method_println(frame->fromcode->m);
	}
	if (frame->tocode) {
		printf("\tto %p ", (void*)frame->tocode);
		method_println(frame->tocode->m);
	}

	if (frame->fromrp) {
		printf("\tfrom replacement point:\n");
		replace_replacement_point_println(frame->fromrp, 2);
	}
	if (frame->torp) {
		printf("\tto replacement point:\n");
		replace_replacement_point_println(frame->torp, 2);
	}

	printf("\n");
}
#endif /* !defined(NDEBUG) */


/* replace_sourcestate_println *************************************************
 
   Print source state
  
   IN:
       ss...............the source state to print
  
*******************************************************************************/

#if !defined(NDEBUG)
void replace_sourcestate_println(sourcestate_t *ss)
{
	int i;
	sourceframe_t *frame;

	if (!ss) {
		printf("(sourcestate_t *)NULL\n");
		return;
	}

	printf("sourcestate_t:\n");

	for (i=0, frame = ss->frames; frame != NULL; frame = frame->down, ++i) {
		printf("    frame %d:\n", i);
		replace_source_frame_println(frame);
	}
}
#endif


/* replace_sourcestate_println_short *******************************************

   Print a compact representation of the given source state.

   IN:
       ss...............the source state to print

*******************************************************************************/

#if !defined(NDEBUG)
void replace_sourcestate_println_short(sourcestate_t *ss)
{
	sourceframe_t *frame;

	for (frame = ss->frames; frame != NULL; frame = frame->down) {
		printf("\t");

		if (frame->torp) {
			printf("%c", (frame->torp == frame->fromrp) ? '=' : '+');
		}

		printf("%s", replace_type_str[frame->fromrp->type]);

		if (frame->torp && frame->torp->type != frame->fromrp->type)
			printf("->%s", replace_type_str[frame->torp->type]);

		if (frame->tocode != frame->fromcode)
			printf(" (%p->%p/%d) ",
				   (void*) frame->fromcode, (void*) frame->tocode,
				   frame->fromrp->id);
		else
			printf(" (%p/%d) ", (void*) frame->fromcode, frame->fromrp->id);

		method_println(frame->method);
	}
}
#endif

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
