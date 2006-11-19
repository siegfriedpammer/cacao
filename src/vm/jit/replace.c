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

   Changes:

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
#include "vm/jit/asmpart.h"
#include "vm/jit/disass.h"
#include "vm/jit/show.h"
#include "vm/jit/methodheader.h"

#include "native/include/java_lang_String.h"


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

#if !defined(NDEBUG) && defined(REPLACE_VERBOSE)
#define DOLOG(code) do{ if (1) { code; } } while(0)
#else
#define DOLOG(code)
#endif


/*** debugging ****************************************************************/

#define REPLACE_STATISTICS

#if defined(REPLACE_STATISTICS)

static int stat_replacements = 0;
static int stat_frames = 0;
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
#define REPLACE_COUNT_INC(cnt, inc)  ((cnt) += (inc))

#define REPLACE_COUNT_DIST(array, val)                               \
    do {                                                             \
        int limit = (sizeof(array) / sizeof(int)) - 1;               \
        if ((val) < (limit)) (array)[val]++;                         \
        else (array)[limit]++;                                       \
    } while (0)

#define REPLACE_PRINT_DIST(name, array) \
	printf("    " name " distribution:\n"); \
	print_freq(stdout, (array), sizeof(array)/sizeof(int) - 1);

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

void replace_print_statistics(void)
{
	printf("replacement statistics:\n");
	printf("    # of replacements: %d\n", stat_replacements);
	printf("    # of frames:       %d\n", stat_frames);
	printf("    unrolled inlines:  %d\n", stat_unroll_inline);
	printf("    unrolled calls:    %d\n", stat_unroll_call);
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

#else

#define REPLACE_COUNT(cnt)
#define REPLACE_COUNT_INC(cnt, inc)
#define REPLACE_COUNT_DIST(array, val)

#endif /* defined(REPLACE_STATISTICS) */

/*** constants used internally ************************************************/

#define TOP_IS_NORMAL    0
#define TOP_IS_ON_STACK  1
#define TOP_IS_IN_ITMP1  2
#define TOP_IS_VOID      3


/* replace_create_replacement_point ********************************************
 
   Create a replacement point.
  
   IN:
       jd...............current jitdata
	   iinfo............inlining info for the current position
	   rp...............pre-allocated (uninitialized) rplpoint
	   type.............RPLPOINT_TYPE constant
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
	rp->outcode = NULL;   /* set by codegen */
	rp->callsize = 0;     /* set by codegen */
	rp->target = NULL;
	rp->regalloc = ra;
	rp->flags = 0;
	rp->type = type;
	rp->id = iptr->flags.bits >> INS_FLAG_ID_SHIFT;

	/* XXX unify these two fields */
	rp->code = jd->code;
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
	methoddesc      *md;
	s4               j;
	insinfo_inline  *iinfo;
	insinfo_inline  *calleeinfo;
	s4               startcount;

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

	/* iterate over the basic block list to find replacement points */

	m = code->m;

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

		if (bptr->javalocals)
			MCOPY(javalocals, bptr->javalocals, s4, m->maxlocals);
		else
			for (i=0; i<m->maxlocals; ++i)
				javalocals[i] = UNUSED;

		/* iterate over the instructions */

		iptr = bptr->iinstr;
		iend = iptr + bptr->icount;
		startcount = count;

		for (; iptr != iend; ++iptr) {
			switch (iptr->opc) {
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKEINTERFACE:
					INSTRUCTION_GET_METHODDESC(iptr, md);
					count++;
					for (i=0; i<m->maxlocals; ++i)
						if (javalocals[i] != UNUSED)
							alloccount++;
					alloccount += iptr->s1.argcount;
					if (iinfo)
						alloccount -= iinfo->throughcount;
					break;

				case ICMD_ISTORE:
				case ICMD_LSTORE:
				case ICMD_FSTORE:
				case ICMD_DSTORE:
				case ICMD_ASTORE:
					/* XXX share code with stack.c */
					j = iptr->dst.varindex;
					i = iptr->sx.s23.s3.javaindex;
					if (i != UNUSED) {
						if (iptr->flags.bits & INS_FLAG_RETADDR)
							javalocals[i] = iptr->sx.s23.s2.retaddrnr;
						else
							javalocals[i] = j;
						if (iptr->flags.bits & INS_FLAG_KILL_PREV)
							javalocals[i-1] = UNUSED;
						if (iptr->flags.bits & INS_FLAG_KILL_NEXT)
							javalocals[i+1] = UNUSED;
					}
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
					for (i=0; i<m->maxlocals; ++i)
						if (javalocals[i] != UNUSED)
							alloccount++;
					alloccount += iinfo->stackvarscount;
					if (iinfo->synclocal != UNUSED)
						alloccount++;

					m = iinfo->method;
					if (iinfo->javalocals_start)
						MCOPY(javalocals, iinfo->javalocals_start, s4, m->maxlocals);
					break;

				case ICMD_INLINE_END:
					iinfo = iptr->sx.s23.s3.inlineinfo;
					m = iinfo->outer;
					if (iinfo->javalocals_end)
						MCOPY(javalocals, iinfo->javalocals_end, s4, m->maxlocals);
					iinfo = iinfo->parent;
					break;
			}
		} /* end instruction loop */

		/* create replacement points at targets of backward branches */
		/* We only need the replacement point there, if there is no  */
		/* replacement point inside the block.                       */

		if (bptr->bitflags & BBFLAG_REPLACEMENT) {
			if (count > startcount) {
				/* we don't need it */
				bptr->bitflags &= ~BBFLAG_REPLACEMENT;
			}
			else {
				count++;
				alloccount += bptr->indepth;

				for (i=0; i<m->maxlocals; ++i)
					if (bptr->javalocals[i] != UNUSED)
						alloccount++;
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

		if (bptr->javalocals)
			MCOPY(javalocals, bptr->javalocals, s4, m->maxlocals);
		else
			for (i=0; i<m->maxlocals; ++i)
				javalocals[i] = UNUSED;

		/* create replacement points at targets of backward branches */

		if (bptr->bitflags & BBFLAG_REPLACEMENT) {

			replace_create_replacement_point(jd, iinfo, rp++,
					bptr->type, bptr->iinstr, &ra,
					bptr->javalocals, bptr->invars, bptr->indepth, 0);
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
					/* XXX share code with stack.c */
					j = iptr->dst.varindex;
					i = iptr->sx.s23.s3.javaindex;
					if (i != UNUSED) {
						if (iptr->flags.bits & INS_FLAG_RETADDR)
							javalocals[i] = iptr->sx.s23.s2.retaddrnr;
						else
							javalocals[i] = j;
						if (iptr->flags.bits & INS_FLAG_KILL_PREV)
							javalocals[i-1] = UNUSED;
						if (iptr->flags.bits & INS_FLAG_KILL_NEXT)
							javalocals[i+1] = UNUSED;
					}
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
					calleeinfo = iptr->sx.s23.s3.inlineinfo;

					calleeinfo->rp = rp;
					replace_create_replacement_point(jd, iinfo, rp++,
							RPLPOINT_TYPE_INLINE, iptr, &ra,
							javalocals,
							calleeinfo->stackvars, calleeinfo->stackvarscount,
							calleeinfo->paramcount);

					if (calleeinfo->synclocal != UNUSED) {
						ra->index = RPLALLOC_SYNC;
						ra->regoff = jd->var[calleeinfo->synclocal].vv.regoff;
						ra->flags  = jd->var[calleeinfo->synclocal].flags & INMEMORY;
						ra->type = TYPE_ADR;
						ra++;
						rp[-1].regalloccount++;
					}

					iinfo = calleeinfo;
					m = iinfo->method;
					if (iinfo->javalocals_start)
						MCOPY(javalocals, iinfo->javalocals_start, s4, m->maxlocals);
					break;

				case ICMD_INLINE_END:
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


/* replace_activate_replacement_point ******************************************
 
   Activate a replacement point. When this function returns, the
   replacement point is "armed", that is each thread reaching this point
   will be replace to `target`.
   
   IN:
       rp...............replacement point to activate
	   target...........target of replacement
  
*******************************************************************************/

void replace_activate_replacement_point(rplpoint *rp,rplpoint *target)
{
	assert(rp->target == NULL);

	DOLOG( printf("activate replacement point:\n");
		   replace_replacement_point_println(rp, 1); fflush(stdout); );

	rp->target = target;

#if (defined(__I386__) || defined(__X86_64__) || defined(__ALPHA__) || defined(__POWERPC__) || defined(__MIPS__)) && defined(ENABLE_JIT)
	md_patch_replacement_point(rp);
#endif
}


/* replace_deactivate_replacement_point ****************************************
 
   Deactivate a replacement point. When this function returns, the
   replacement point is "un-armed", that is a each thread reaching this point
   will just continue normally.
   
   IN:
       rp...............replacement point to deactivate
  
*******************************************************************************/

void replace_deactivate_replacement_point(rplpoint *rp)
{
	assert(rp->target);

	DOLOG( printf("deactivate replacement point:\n");
		   replace_replacement_point_println(rp, 1); fflush(stdout); );

	rp->target = NULL;

#if (defined(__I386__) || defined(__X86_64__) || defined(__ALPHA__) || defined(__POWERPC__) || defined(__MIPS__)) && defined(ENABLE_JIT)
	md_patch_replacement_point(rp);
#endif
}


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

	code = rp->code;
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
	frame->up = ss->frames;
	frame->method = rp->method;
	frame->id = rp->id;
	frame->syncslotcount = 0;
	frame->syncslots = NULL;
#if !defined(NDEBUG)
	frame->debug_rp = rp;
#endif

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
				sourceframe_t *calleeframe = frame->up;
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

	code = rp->code;
	m = rp->method;
	topslot = TOP_IS_NORMAL;

	/* pop a source frame */

	frame = ss->frames;
	assert(frame);
	ss->frames = frame->up;

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
				assert(frame->up);
				assert(frame->up->syncslotcount == 1); /* XXX need to understand more cases */
				assert(frame->up->syncslots != NULL);

				replace_write_value(es,sp,ra,frame->up->syncslots);
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

bool replace_pop_activation_record(executionstate_t *es,
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
		return false;

	code = *(codeinfo **)(pv + CodeinfoPointer);

	DOLOG( printf("CODE = %p\n", (void*) code); );

	if (code == NULL)
		return false;

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

	return true;
}


/* replace_push_activation_record **********************************************

   Push a stack frame onto the execution state.
   
   *** This function imitates the effects of a call and the ***
   *** method prolog of the callee.                         ***

   IN:
	   es...............execution state
	   rpcall...........the replacement point at the call site
	   calleecode.......the codeinfo of the callee
	   frame............source frame, only the synch. slots are used

   OUT:
       *es..............the execution state after pushing the stack frame
  
*******************************************************************************/

void replace_push_activation_record(executionstate_t *es,
									rplpoint *rpcall,
									codeinfo *calleecode,
									sourceframe_t *frame)
{
	s4 reg;
	s4 i;
	s4 count;
	stackslot_t *basesp;
	stackslot_t *sp;

	assert(es);
	assert(rpcall && rpcall->type == RPLPOINT_TYPE_CALL);
	assert(calleecode);
	assert(frame);

	/* write the return address */

	es->sp -= SIZE_OF_STACKSLOT;

	DOLOG( printf("writing return address %p to %p\n",
				(void*) (rpcall->pc + rpcall->callsize),
				(void*) es->sp); );

	*((stackslot_t *)es->sp) = (stackslot_t) (rpcall->pc + rpcall->callsize);

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
	assert(count == frame->syncslotcount);
	for (i=0; i<count; ++i) {
		sp[es->code->memuse + i] = frame->syncslots[i];
	}

	/* set the PV */

	es->pv = es->code->entrypoint;
}


/* replace_find_replacement_point **********************************************

   Find the replacement point in the given code corresponding to the
   position given in the source frame.
   
   IN:
	   code.............the codeinfo in which to search the rplpoint
	   ss...............the source state defining the position to look for

   RETURN VALUE:
       the replacement point
  
*******************************************************************************/

rplpoint * replace_find_replacement_point(codeinfo *code, sourcestate_t *ss)
{
	sourceframe_t *frame;
	methodinfo *m;
	rplpoint *rp;
	s4        i;
	s4        j;
	s4        stacki;
	rplalloc *ra;

	assert(ss);

	frame = ss->frames;
	assert(frame);

	DOLOG( printf("searching replacement point for:\n");
		   replace_source_frame_println(frame); );

	m = frame->method;

	DOLOG( printf("code = %p\n", (void*)code); );

	rp = code->rplpoints;
	i = code->rplpointcount;
	while (i--) {
		if (rp->id == frame->id && rp->method == frame->method) {
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

	assert(0);
	return NULL; /* NOT REACHED */
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
	rplpoint     *target;
	sourcestate_t ss;
	s4            dumpsize;
	rplpoint     *candidate;
	codeinfo     *code;
	s4            i;
	s4            depth;
	rplpoint     *origrp;

	origrp = rp;

	es->code = rp->code;

	DOLOG( printf("REPLACING: "); method_println(es->code->m); );
	REPLACE_COUNT(stat_replacements);

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fetch the target of the replacement */

	target = rp->target;

	DOLOG( printf("replace_me(%p,%p)\n",(void*)rp,(void*)es); fflush(stdout);
		   replace_replacement_point_println(rp, 1);
		   replace_executionstate_println(es); );

	/* read execution state of old code */

	ss.frames = NULL;

	/* XXX testing */

	depth = 0;
	candidate = rp;
	do {
		DOLOG( printf("recovering source state for%s:\n",
					(ss.frames == NULL) ? " TOPFRAME" : "");
			   replace_replacement_point_println(candidate, 1); );

		replace_read_executionstate(candidate, es, &ss, ss.frames == NULL);
		REPLACE_COUNT(stat_frames);
		depth++;

#if defined(REPLACE_STATISTICS)
		int adr = 0; int ret = 0; int prim = 0; int vd = 0; int n = 0;
		for (i=0; i<ss.frames->javalocalcount; ++i) {
			switch (ss.frames->javalocaltype[i]) {
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
		for (i=0; i<ss.frames->javastackdepth; ++i) {
			switch (ss.frames->javastacktype[i]) {
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
#endif /* defined(REPLACE_STATISTICS) */

		if (candidate->parent) {
			DOLOG( printf("INLINED!\n"); );
			candidate = candidate->parent;
			assert(candidate->type == RPLPOINT_TYPE_INLINE);
			REPLACE_COUNT(stat_unroll_inline);
		}
		else {
			DOLOG( printf("UNWIND\n"); );
			REPLACE_COUNT(stat_unroll_call);
			if (!replace_pop_activation_record(es, ss.frames)) {
				DOLOG( printf("BREAKING\n"); );
				break;
			}
			DOLOG( replace_executionstate_println(es); );
			candidate = NULL;
			rp = es->code->rplpoints;
			for (i=0; i<es->code->rplpointcount; ++i, ++rp)
				if (rp->pc <= es->pc)
					candidate = rp;
			if (!candidate)
				DOLOG( printf("NO CANDIDATE!\n"); );
			else {
				DOLOG( printf("found replacement point.\n");
					   replace_replacement_point_println(candidate, 1); );
				assert(candidate->type == RPLPOINT_TYPE_CALL);
			}
		}
	} while (candidate);

	REPLACE_COUNT_DIST(stat_dist_frames, depth);
	DOLOG( replace_sourcestate_println(&ss); );

	/* write execution state of new code */

	DOLOG( replace_executionstate_println(es); );

	code = es->code;

	/* XXX get new code */

	while (ss.frames) {

		candidate = replace_find_replacement_point(code, &ss);

		DOLOG( printf("creating execution state for%s:\n",
				(ss.frames->up == NULL) ? " TOPFRAME" : "");
			   replace_replacement_point_println(ss.frames->debug_rp, 1);
			   replace_replacement_point_println(candidate, 1); );

		replace_write_executionstate(candidate, es, &ss, ss.frames->up == NULL);
		if (ss.frames == NULL)
			break;
		DOLOG( replace_executionstate_println(es); );

		if (candidate->type == RPLPOINT_TYPE_CALL) {
			jit_recompile(ss.frames->method);
			code = ss.frames->method->code;
			assert(code);
			DOLOG( printf("pushing activation record for:\n");
				   replace_replacement_point_println(candidate, 1); );
			replace_push_activation_record(es, candidate, code, ss.frames);
		}
		DOLOG( replace_executionstate_println(es); );
	}

	DOLOG( replace_executionstate_println(es); );

	assert(candidate);
	if (candidate == origrp) {
		printf("WARNING: identity replacement, turning off rp to avoid infinite loop");
		replace_deactivate_replacement_point(origrp);
	}

	/* release dump area */

	dump_release(dumpsize);

	/* enter new code */

	DOLOG( printf("JUMPING IN!\n"); fflush(stdout); );

#if (defined(__I386__) || defined(__X86_64__) || defined(__ALPHA__) || defined(__POWERPC__) || defined(__MIPS__)) && defined(ENABLE_JIT)
	asm_replacement_in(es);
#endif
	abort(); /* NOT REACHED */
}


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
	"RETURN"
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

	printf("rplpoint (id %d) %p pc:%p+%d out:%p target:%p mcode:%016llx type:%s flags:%01x parent:%p\n",
			rp->id, (void*)rp,rp->pc,rp->callsize,rp->outcode,(void*)rp->target,
			(unsigned long long)rp->mcode,replace_type_str[rp->type],rp->flags,
			(void*)rp->parent);
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
#endif


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

		assert(rp->code == code);

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
void java_value_print(s4 type, u8 value)
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
	printf("\n");

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

	for (i=0, frame = ss->frames; frame != NULL; frame = frame->up, ++i) {
		printf("    frame %d:\n", i);
		replace_source_frame_println(frame);
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
