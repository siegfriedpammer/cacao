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


/*** constants used internally ************************************************/

#define TOP_IS_NORMAL    0
#define TOP_IS_ON_STACK  1
#define TOP_IS_IN_ITMP1  2


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
  
   OUT:
       *rpa.............points to the next free rplalloc
  
*******************************************************************************/

static void replace_create_replacement_point(jitdata *jd,
											 insinfo_inline *iinfo,
											 rplpoint *rp,
											 s4 type,
											 rplalloc **pra,
											 s4 *javalocals,
											 s4 *stackvars,
											 s4 stackdepth)
{
	rplalloc *ra;
	s4        i;
	varinfo  *v;
	s4        index;

	static s4 fake_id = 0;

	ra = *pra;

	/* there will be a replacement point at the start of this block */

	rp->method = (iinfo) ? iinfo->method : jd->m;
	rp->pc = NULL;        /* set by codegen */
	rp->outcode = NULL;   /* set by codegen */
	rp->callsize = 0;     /* set by codegen */
	rp->target = NULL;
	rp->regalloc = ra;
	rp->flags = 0;
	rp->type = type;
	rp->id = ++fake_id; /* XXX need a real invariant id */

	/* XXX unify these two fields */
	rp->code = jd->code;
	rp->parent = (iinfo) ? iinfo->rp : NULL;

	/* store local allocation info of javalocals */

	if (javalocals) {
		for (i = 0; i < rp->method->maxlocals; ++i) {
			index = javalocals[i];
			if (index == UNUSED)
				continue;

			v = VAR(index);
			ra->flags = v->flags & (INMEMORY);
			ra->index = i;
			ra->regoff = v->vv.regoff;
			ra->type = v->type;
			ra++;
		}
	}

	/* store allocation info of java stack vars */

	for (i = 0; i < stackdepth; ++i) {
		v = VAR(stackvars[i]);
		ra->flags = v->flags & (INMEMORY);
		ra->index = -1;
		ra->regoff = v->vv.regoff;
		ra->type  = v->type;
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

		/* create replacement points at targets of backward branches */

		if (bptr->bitflags & BBFLAG_REPLACEMENT) {
			count++;
			alloccount += bptr->indepth;

			for (i=0; i<m->maxlocals; ++i)
				if (bptr->javalocals[i] != UNUSED)
					alloccount++;
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
					count++;
					for (i=0; i<m->maxlocals; ++i)
						if (javalocals[i] != UNUSED)
							alloccount++;
					alloccount += iptr->s1.argcount - md->paramcount;
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
					if (iptr->flags.bits & INS_FLAG_RETADDR)
						javalocals[i] = UNUSED;
					else
						javalocals[i] = j;
					if (iptr->flags.bits & INS_FLAG_KILL_PREV)
						javalocals[i-1] = UNUSED;
					if (iptr->flags.bits & INS_FLAG_KILL_NEXT)
						javalocals[i+1] = UNUSED;
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
		}
	}

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
					bptr->type, &ra,
					bptr->javalocals, bptr->invars, bptr->indepth);
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
							RPLPOINT_TYPE_CALL, &ra,
							javalocals, iptr->sx.s23.s2.args + md->paramcount,
							iptr->s1.argcount - md->paramcount - i);
					break;

				case ICMD_ISTORE:
				case ICMD_LSTORE:
				case ICMD_FSTORE:
				case ICMD_DSTORE:
				case ICMD_ASTORE:
					/* XXX share code with stack.c */
					j = iptr->dst.varindex;
					i = iptr->sx.s23.s3.javaindex;
					if (iptr->flags.bits & INS_FLAG_RETADDR)
						javalocals[i] = UNUSED;
					else
						javalocals[i] = j;
					if (iptr->flags.bits & INS_FLAG_KILL_PREV)
						javalocals[i-1] = UNUSED;
					if (iptr->flags.bits & INS_FLAG_KILL_NEXT)
						javalocals[i+1] = UNUSED;
					break;

				case ICMD_IRETURN:
				case ICMD_LRETURN:
				case ICMD_FRETURN:
				case ICMD_DRETURN:
				case ICMD_ARETURN:
					replace_create_replacement_point(jd, iinfo, rp++,
							RPLPOINT_TYPE_RETURN, &ra,
							NULL, &(iptr->s1.varindex), 1);
					break;

				case ICMD_RETURN:
					replace_create_replacement_point(jd, iinfo, rp++,
							RPLPOINT_TYPE_RETURN, &ra,
							NULL, NULL, 0);
					break;

				case ICMD_INLINE_START:
					calleeinfo = iptr->sx.s23.s3.inlineinfo;

					calleeinfo->rp = rp;
					replace_create_replacement_point(jd, iinfo, rp++,
							RPLPOINT_TYPE_INLINE, &ra,
							javalocals,
							calleeinfo->stackvars, calleeinfo->stackvarscount);

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
		}
	}

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

#if !defined(NDEBUG)
	printf("activate replacement point: ");
	replace_replacement_point_println(rp, 0);
	fflush(stdout);
#endif

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

#if !defined(NDEBUG)
	printf("deactivate replacement point: ");
	replace_replacement_point_println(rp, 0);
	fflush(stdout);
#endif

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

   Read the given executions state and translate it to a source state.
   
   IN:
       rp...............replacement point at which `es` was taken
	   es...............execution state
	   ss...............where to put the source state

   OUT:
       *ss..............the source state derived from the execution state
  
*******************************************************************************/

static void replace_read_executionstate(rplpoint *rp,executionstate_t *es,
									 sourcestate_t *ss)
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

	/* on some architectures the returnAddress is passed on the stack by JSR */

#if defined(__I386__) || defined(__X86_64__)
	if (rp->type == BBTYPE_SBR) {
		sp++;
		topslot = TOP_IS_ON_STACK; /* XXX */
	}
#endif

	/* in some cases the top stack slot is passed in REG_ITMP1 */

	if (  (rp->type == BBTYPE_EXH)
#if defined(__ALPHA__) || defined(__POWERPC__) || defined(__MIPS__)
	   || (rp->type == BBTYPE_SBR) /* XXX */
#endif
	   )
	{
		topslot = TOP_IS_IN_ITMP1;
	}

	/* calculate base stack pointer */

	basesp = sp + code_get_stack_frame_size(code);

	/* create the source frame */

	frame = DNEW(sourceframe_t);
	frame->up = ss->frames;
	frame->method = rp->method;
	frame->id = rp->id;

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

		frame->javastack[i] = sp[-1];
		frame->javastacktype[i] = TYPE_ADR; /* XXX RET */
		count--;
		i++;
		ra++;
	}
	else if (topslot == TOP_IS_IN_ITMP1) {
		assert(count);

		frame->javastack[i] = es->intregs[REG_ITMP1];
		frame->javastacktype[i] = TYPE_ADR; /* XXX RET */
		count--;
		i++;
		ra++;
	}

	/* read remaining stack slots */

	for (; count--; ra++, i++) {
		assert(ra->index == -1);

		replace_read_value(es,sp,ra,frame->javastack + i);
		frame->javastacktype[i] = ra->type;
	}

	/* read slots used for synchronization */

	count = code_get_sync_slot_count(code);
	frame->syncslotcount = count;
	frame->syncslots = DMNEW(u8,count);
	for (i=0; i<count; ++i) {
		frame->syncslots[i] = sp[code->memuse + i];
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
										 sourcestate_t *ss)
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

	/* on some architectures the returnAddress is passed on the stack by JSR */

#if defined(__I386__) || defined(__X86_64__)
	if (rp->type == BBTYPE_SBR) {
		topslot = TOP_IS_ON_STACK; /* XXX */
	}
#endif

	/* in some cases the top stack slot is passed in REG_ITMP1 */

	if (  (rp->type == BBTYPE_EXH)
#if defined(__ALPHA__) || defined(__POWERPC__) || defined(__MIPS__)
	   || (rp->type == BBTYPE_SBR) /* XXX */
#endif
	   )
	{
		topslot = TOP_IS_IN_ITMP1;
	}

	/* write javalocals */

	ra = rp->regalloc;
	count = rp->regalloccount;

	while (count && (i = ra->index) >= 0) {
		assert(i < m->maxlocals);
		assert(ra->type == frame->javalocaltype[i]);
		replace_write_value(es, sp, ra, frame->javalocals + i);
		count--;
		ra++;
	}

	/* write stack slots */

	i = 0;

	/* the first stack slot is special in SBR and EXH blocks */

	if (topslot == TOP_IS_ON_STACK) {
		assert(count);

		assert(frame->javastacktype[i] == TYPE_ADR);
		sp[-1] = frame->javastack[i];
		count--;
		i++;
		ra++;
	}
	else if (topslot == TOP_IS_IN_ITMP1) {
		assert(count);

		assert(frame->javastacktype[i] == TYPE_ADR);
		assert(frame->javastacktype[i] == TYPE_ADR);
		es->intregs[REG_ITMP1] = frame->javastack[i];
		count--;
		i++;
		ra++;
	}

	/* write remaining stack slots */

	for (; count--; ra++, i++) {
		assert(ra->index == -1);

		assert(ra->type == frame->javastacktype[i]);
		replace_write_value(es,sp,ra,frame->javastack + i);
	}

	/* write slots used for synchronization */

	count = code_get_sync_slot_count(code);
	assert(count == frame->syncslotcount);
	for (i=0; i<count; ++i) {
		sp[code->memuse + i] = frame->syncslots[i];
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

   OUT:
       *es..............the execution state after popping the stack frame
  
*******************************************************************************/

bool replace_pop_activation_record(executionstate_t *es)
{
	u1 *ra;
	u1 *pv;
	s4 reg;
	s4 i;
	codeinfo *code;
	stackslot_t *basesp;

	assert(es->code);

	/* read the return address */

	ra = md_stacktrace_get_returnaddress(es->sp,
			SIZE_OF_STACKSLOT * es->code->stackframesize);

	printf("return address: %p\n", (void*)ra);

	/* find the new codeinfo */

	pv = md_codegen_get_pv_from_pc(ra);

	printf("PV = %p\n", (void*) pv);

	if (pv == NULL)
		return false;

	code = *(codeinfo **)(pv + CodeinfoPointer);

	printf("CODE = %p\n", (void*) code);

	if (code == NULL)
		return false;

	/* calculate the base of the stack frame */

	basesp = (stackslot_t *)es->sp + es->code->stackframesize;

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

	/* set the new pc */

	es->pc = ra;

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

   OUT:
       *es..............the execution state after pushing the stack frame
  
*******************************************************************************/

void replace_push_activation_record(executionstate_t *es,
									rplpoint *rpcall,
									codeinfo *calleecode)
{
	s4 reg;
	s4 i;
	stackslot_t *basesp;

	/* write the return address */

	*((stackslot_t *)es->sp) = (stackslot_t) (rpcall->pc + rpcall->callsize);

	es->sp -= SIZE_OF_STACKSLOT;

	/* we move into a new code unit */

	es->code = calleecode;

	/* set the new pc XXX not needed */

	es->pc = es->code->entrypoint;

	/* build the stackframe */

	basesp = (stackslot_t *) es->sp;
	es->sp -= SIZE_OF_STACKSLOT * es->code->stackframesize;

	/* in debug mode, invalidate stack frame first */

#if !defined(NDEBUG)
	{
		stackslot_t *sp = (stackslot_t *) es->sp;
		for (i=0; i<(basesp - sp); ++i) {
			sp[i] = 0xdeaddeadU;
		}
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

	assert(ss);

	frame = ss->frames;
	assert(frame);

#if !defined(NDEBUG)
	printf("searching replacement point for:\n");
	replace_source_frame_println(frame);
#endif

	m = frame->method;

#if !defined(NDEBUG)
	printf("code = %p\n", (void*)code);
#endif

	rp = code->rplpoints;
	i = code->rplpointcount;
	while (i--) {
		if (rp->id == frame->id)
			return rp;
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

	es->code = rp->code;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fetch the target of the replacement */

	target = rp->target;

	/* XXX DEBUG turn off self-replacement */
	if (target == rp)
		replace_deactivate_replacement_point(rp);

#if !defined(NDEBUG)
	printf("replace_me(%p,%p)\n",(void*)rp,(void*)es);
	fflush(stdout);
	replace_replacement_point_println(rp, 0);
	replace_executionstate_println(es);
#endif

	/* read execution state of old code */

	ss.frames = NULL;

	/* XXX testing */

	candidate = rp;
	do {
#if !defined(NDEBUG)
		printf("recovering source state for:\n");
		replace_replacement_point_println(candidate, 1);
#endif

		replace_read_executionstate(candidate,es,&ss);

		if (candidate->parent) {
			printf("INLINED!\n");
			candidate = candidate->parent;
			assert(candidate->type == RPLPOINT_TYPE_INLINE);
		}
		else {
			printf("UNWIND\n");
			if (!replace_pop_activation_record(es)) {
				printf("BREAKING\n");
				break;
			}
#if !defined(NDEBUG)
			replace_executionstate_println(es);
#endif
			candidate = NULL;
			rp = es->code->rplpoints;
			for (i=0; i<es->code->rplpointcount; ++i, ++rp)
				if (rp->pc <= es->pc)
					candidate = rp;
			if (!candidate)
				printf("NO CANDIDATE!\n");
			else {
				printf("found replacement point.\n");
				assert(candidate->type == RPLPOINT_TYPE_CALL);
			}
		}
	} while (candidate);

#if !defined(NDEBUG)
	replace_sourcestate_println(&ss);
#endif

	/* write execution state of new code */

#if !defined(NDEBUG)
	replace_executionstate_println(es);
#endif

	code = es->code;


	while (ss.frames) {

		candidate = replace_find_replacement_point(code, &ss);

#if !defined(NDEBUG)
		printf("creating execution state for:\n");
		replace_replacement_point_println(candidate, 1);
#endif

		replace_write_executionstate(candidate, es, &ss);
		if (ss.frames == NULL)
			break;

		if (candidate->type == RPLPOINT_TYPE_CALL) {
			code = ss.frames->method->code;
			assert(code);
			replace_push_activation_record(es, candidate, code);
		}
#if !defined(NDEBUG)
		replace_executionstate_println(es);
#endif
	}

#if !defined(NDEBUG)
	replace_executionstate_println(es);
#endif

	/* release dump area */

	dump_release(dumpsize);

	/* enter new code */

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
static const char *type_char = "IJFDA";

#define TYPECHAR(t)  (((t) >= 0 && (t) <= 4) ? type_char[t] : '?')

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

	if (!rp) {
		printf("(rplpoint *)NULL\n");
		return;
	}

	for (j=0; j<depth; ++j)
		putchar('\t');

	printf("rplpoint %p pc:%p+%d out:%p target:%p mcode:%016llx type:%s flags:%01x parent:%p\n",
			(void*)rp,rp->pc,rp->callsize,rp->outcode,(void*)rp->target,
			(unsigned long long)rp->mcode,replace_type_str[rp->type],rp->flags,
			(void*)rp->parent);
	for (j=0; j<depth; ++j)
		putchar('\t');
	printf("ra:%d = [",	rp->regalloccount);

	for (j=0; j<rp->regalloccount; ++j) {
		if (j)
			putchar(' ');
		printf("%d:%1c:",
				rp->regalloc[j].index,
				/* (rp->regalloc[j].next) ? '^' : ' ', */
				TYPECHAR(rp->regalloc[j].type));
		show_allocation(rp->regalloc[j].type, rp->regalloc[j].flags, rp->regalloc[j].regoff);
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

	if (es->code)
		slots = code_get_stack_frame_size(es->code);
	else
		slots = 0;

	if (slots) {
		printf("\tstack slots(+1) at sp:");
		for (i=0; i<slots+1; ++i) {
			if (i%4 == 0)
				printf("\n\t\t");
			else
				printf(" ");
			if (i >= slots)
				putchar('(');
#ifdef HAS_4BYTE_STACKSLOT
			printf("%08lx",(unsigned long)*sp++);
#else
			printf("%016llx",(unsigned long long)*sp++);
#endif
			if (i >= slots)
				putchar(')');
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
			printf("\tstack[%2d] = ",i);
			java_value_print(frame->javastacktype[i], frame->javastack[i]);
			printf("\n");
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
