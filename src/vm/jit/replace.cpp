/* src/vm/jit/replace.cpp - on-stack replacement of methods

   Copyright (C) 1996-2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

*/


#include "config.h"
#include "vm/types.hpp"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "arch.hpp"
#include "md.hpp"

#include "mm/dumpmemory.hpp"
#include "mm/memory.hpp"

#include "threads/thread.hpp"

#include "toolbox/logging.hpp"

#include "vm/classcache.hpp"
#include "vm/descriptor.hpp"
#include "vm/globals.hpp"
#include "vm/options.hpp"
#include "vm/string.hpp"

#if defined(ENABLE_RT_TIMING)
# include "vm/rt-timing.hpp"
#endif

#include "vm/jit/abi.hpp"
#include "vm/jit/asmpart.hpp"
#include "vm/jit/disass.hpp"
#include "vm/jit/executionstate.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/methodheader.hpp"
#include "vm/jit/replace.hpp"
#include "vm/jit/show.hpp"
#include "vm/jit/stack.hpp"
#include "vm/jit/stacktrace.hpp"

#include "vm/jit/ir/instruction.hpp"

#define DEBUG_NAME "replace"

using namespace cacao;

#define REPLACE_PATCH_DYNAMIC_CALL
/*#define REPLACE_PATCH_ALL*/

/*** constants used internally ************************************************/

#define TOP_IS_NORMAL    0
#define TOP_IS_ON_STACK  1
#define TOP_IS_IN_ITMP1  2
#define TOP_IS_VOID      3

/**
 * Determines whether a given `sourceframe` represents a frame of a
 * native method.
 */
#define REPLACE_IS_NATIVE_FRAME(frame)  ((frame)->sfi != NULL)

/**
 * Determeines whether the given replacement point is at a call site.
 */
#define REPLACE_IS_CALL_SITE(rp) ((rp)->callsize > 0)

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
	u1       *savedmcode;

	assert(code->savedmcode == NULL);

	/* count trappable replacement points */

	count = 0;
	i = code->rplpointcount;
	rp = code->rplpoints;
	for (; i--; rp++) {
		if (!(rp->flags & rplpoint::FLAG_TRAPPABLE))
			continue;

#if 0
		if (mappable && (rp->type == rplpoint::TYPE_RETURN))
			continue;
#endif

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
		assert(!(rp->flags & rplpoint::FLAG_ACTIVE));

		if (!(rp->flags & rplpoint::FLAG_TRAPPABLE))
			continue;

#if 0
		if (mappable && (rp->type == rplpoint::TYPE_RETURN))
			continue;
#endif

		LOG2("activate replacement point: " << rp << nl);

		savedmcode -= REPLACEMENT_PATCH_SIZE;
		md_patch_replacement_point(rp->pc, savedmcode, false);
		rp->flags |= rplpoint::FLAG_ACTIVE;
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
		if (!(rp->flags & rplpoint::FLAG_ACTIVE))
			continue;

		count++;

		LOG2("deactivate replacement point: " << rp << nl);
		md_patch_replacement_point(rp->pc, savedmcode, true);
		rp->flags &= ~rplpoint::FLAG_ACTIVE;
		savedmcode += REPLACEMENT_PATCH_SIZE;
	}

	assert(savedmcode == code->savedmcode + count * REPLACEMENT_PATCH_SIZE);

	/* free saved machine code */

	MFREE(code->savedmcode, u1, count * REPLACEMENT_PATCH_SIZE);
	code->savedmcode = NULL;
}


/* replace_read_value **********************************************************

   Read a value with the given allocation from the execution state.

   IN:
	   es...............execution state
	   ra...............allocation
	   javaval..........where to put the value

   OUT:
       *javaval.........the value

*******************************************************************************/

static void replace_read_value(executionstate_t *es,
							   rplalloc *ra,
							   replace_val_t *javaval)
{
	if (ra->inmemory) {
		/* XXX HAS_4BYTE_STACKSLOT may not be the right discriminant here */
#ifdef HAS_4BYTE_STACKSLOT
		if (IS_2_WORD_TYPE(ra->type)) {
			javaval->l = *(u8*)(es->sp + ra->regoff);
		}
		else {
#endif
			javaval->p = *(ptrint*)(es->sp + ra->regoff);
#ifdef HAS_4BYTE_STACKSLOT
		}
#endif
	}
	else {
		/* allocated register */
		if (IS_FLT_DBL_TYPE(ra->type)) {
			javaval->d = es->fltregs[ra->regoff];

			if (ra->type == TYPE_FLT)
				javaval->f = javaval->d;
		}
		else {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
			if (ra->type == TYPE_LNG) {
				javaval->words.lo = es->intregs[GET_LOW_REG(ra->regoff)];
				javaval->words.hi = es->intregs[GET_HIGH_REG(ra->regoff)];
			}
			else
#endif /* defined(SUPPORT_COMBINE_INTEGER_REGISTERS) */
				javaval->p = es->intregs[ra->regoff];
		}
	}
}


/* replace_write_value *********************************************************

   Write a value to the given allocation in the execution state.

   IN:
	   es...............execution state
	   ra...............allocation
	   *javaval.........the value

*******************************************************************************/

static void replace_write_value(executionstate_t *es,
							    rplalloc *ra,
							    replace_val_t *javaval)
{
	if (ra->inmemory) {
		/* XXX HAS_4BYTE_STACKSLOT may not be the right discriminant here */
#ifdef HAS_4BYTE_STACKSLOT
		if (IS_2_WORD_TYPE(ra->type)) {
			*(u8*)(es->sp + ra->regoff) = javaval->l;
		}
		else {
#endif
			*(ptrint*)(es->sp + ra->regoff) = javaval->p;
#ifdef HAS_4BYTE_STACKSLOT
		}
#endif
	}
	else {
		/* allocated register */
		switch (ra->type) {
			case TYPE_FLT:
				es->fltregs[ra->regoff] = (double) javaval->f;
				break;
			case TYPE_DBL:
				es->fltregs[ra->regoff] = javaval->d;
				break;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
			case TYPE_LNG:
				es->intregs[GET_LOW_REG(ra->regoff)] = javaval->words.lo;
				es->intregs[GET_HIGH_REG(ra->regoff)] = javaval->words.hi;
				break;
#endif
			default:
				es->intregs[ra->regoff] = javaval->p;
		}
	}
}


/* replace_new_sourceframe *****************************************************

   Allocate a new source frame and insert it at the front of the frame list.

   IN:
	   ss...............the source state

   OUT:
	   ss->frames.......set to new frame (the new head of the frame list).

   RETURN VALUE:
       returns the new frame

*******************************************************************************/

static sourceframe_t *replace_new_sourceframe(sourcestate_t *ss)
{
	sourceframe_t *frame;

	frame = (sourceframe_t*) DumpMemory::allocate(sizeof(sourceframe_t));
	MZERO(frame, sourceframe_t, 1);

	frame->down = ss->frames;
	ss->frames = frame;

	return frame;
}


/* replace_read_executionstate *************************************************

   Read a source frame from the given executions state.
   The new source frame is pushed to the front of the frame list of the
   source state.

   IN:
       rp...............replacement point at which `es` was taken
	   es...............execution state
	   ss...............the source state to add the source frame to

   OUT:
       *ss..............the source state with the newly created source frame
	                    added

*******************************************************************************/

static s4 replace_normalize_type_map[] = {
/* rplpoint::TYPE_STD    |--> */ rplpoint::TYPE_STD,
/* rplpoint::TYPE_EXH    |--> */ rplpoint::TYPE_STD,
/* rplpoint::TYPE_SBR    |--> */ rplpoint::TYPE_STD,
/* rplpoint::TYPE_CALL   |--> */ rplpoint::TYPE_CALL,
/* rplpoint::TYPE_INLINE |--> */ rplpoint::TYPE_CALL,
/* rplpoint::TYPE_RETURN |--> */ rplpoint::TYPE_RETURN,
/* rplpoint::TYPE_BODY   |--> */ rplpoint::TYPE_STD
};


static void replace_read_executionstate(rplpoint *rp,
										executionstate_t *es,
										sourcestate_t *ss)
{
	methodinfo    *m;
	codeinfo      *code;
	rplalloc      *ra;
	sourceframe_t *frame;
	int            topslot;
	stackslot_t   *sp;
#if defined(__I386__)
	stackslot_t   *basesp;
#endif
	bool           topframe;

	LOG("read execution state at replacement point " << rp << nl);

	code = code_find_codeinfo_for_pc(rp->pc);
	m = rp->method;
	topslot = TOP_IS_NORMAL;
	topframe = ss->frames == NULL;

	/* stack pointer */

	sp = (stackslot_t *) es->sp;

	/* in some cases the top stack slot is passed in REG_ITMP1 */

#if 0
	if (rp->type == rplpoint::TYPE_EXH) {
		topslot = TOP_IS_IN_ITMP1;
	}
#endif

	/* calculate base stack pointer */

#if defined(__I386__)
	basesp = sp + code->stackframesize;
#endif

	/* create the source frame */

	frame = replace_new_sourceframe(ss);
	frame->method = rp->method;
	frame->id = rp->id;
#if 0
	assert(rp->type >= 0 && rp->type < sizeof(replace_normalize_type_map)/sizeof(s4));
	frame->type = replace_normalize_type_map[rp->type];
#endif
	frame->fromrp = rp;
	frame->fromcode = code;

	/* read local variables */

	frame->javalocalcount = m->maxlocals;
	frame->javalocals = (replace_val_t*) DumpMemory::allocate(sizeof(replace_val_t) * frame->javalocalcount);
	frame->javalocaltype = (u1*) DumpMemory::allocate(sizeof(u1) * frame->javalocalcount);

	/* mark values as undefined */
	for (int javalocal_index = 0; javalocal_index < frame->javalocalcount;
			++javalocal_index) {
#if !defined(NDEBUG)
		frame->javalocals[javalocal_index].l = (u8) 0x00dead0000dead00ULL;
#endif
		frame->javalocaltype[javalocal_index] = TYPE_VOID;
	}

	/* some entries in the intregs array are not meaningful */
	/*es->intregs[REG_ITMP3] = (u8) 0x11dead1111dead11ULL;*/
#if !defined(NDEBUG)
	es->intregs[REG_SP   ] = (ptrint) 0x11dead1111dead11ULL;
#ifdef REG_PV
	es->intregs[REG_PV   ] = (ptrint) 0x11dead1111dead11ULL;
#endif
#endif /* !defined(NDEBUG) */

	/* read javalocals */

	ra = rp->regalloc;
	int remaining_allocations = rp->regalloccount;

	while (remaining_allocations && ra->index >= 0) {
		assert(ra->index < m->maxlocals);
		frame->javalocaltype[ra->index] = ra->type;
		if (ra->type == TYPE_RET)
			frame->javalocals[ra->index].i = ra->regoff;
		else
			replace_read_value(es, ra, frame->javalocals + ra->index);
		ra++;
		remaining_allocations--;
	}

	/* read instance, if this is the first rplpoint */

#if defined(REPLACE_PATCH_DYNAMIC_CALL)
	if (topframe && !(rp->method->flags & ACC_STATIC) && rp == code->rplpoints) {
#if 1
		/* we are at the start of the method body, so if local 0 is set, */
		/* it is the instance.                                           */
		if (frame->javalocaltype[0] == TYPE_ADR)
			frame->instance = frame->javalocals[0];
#else
		rplalloc instra;
		methoddesc *md;

		md = rp->method->parseddesc;
		assert(md->params);
		assert(md->paramcount >= 1);
		instra.type = TYPE_ADR;
		instra.regoff = md->params[0].regoff;
		if (md->params[0].inmemory) {
			instra.flags = INMEMORY;
			instra.regoff += (1 + code->stackframesize) * SIZE_OF_STACKSLOT;
		}
		else {
			instra.flags = 0;
		}
		replace_read_value(es, &instra, &(frame->instance));
#endif
	}
#if defined(__I386__)
	else if (!(rp->method->flags & ACC_STATIC)) {
		/* On i386 we always pass the first argument on stack. */
		frame->instance.a = *(java_object_t **)(basesp + 1);
	}
#endif
#endif /* defined(REPLACE_PATCH_DYNAMIC_CALL) */

	/* read stack slots */

	frame->javastackdepth = remaining_allocations;
	frame->javastack = (replace_val_t*) DumpMemory::allocate(sizeof(replace_val_t) * frame->javastackdepth);
	frame->javastacktype = (u1*) DumpMemory::allocate(sizeof(u1) * frame->javastackdepth);

#if !defined(NDEBUG)
	/* mark values as undefined */
	for (int stack_index = 0; stack_index < frame->javastackdepth; ++stack_index) {
		frame->javastack[stack_index].l = (u8) 0x00dead0000dead00ULL;
		frame->javastacktype[stack_index] = TYPE_VOID;
	}
#endif /* !defined(NDEBUG) */

	int stack_index = 0;

	/* the first stack slot is special in SBR and EXH blocks */

	if (topslot == TOP_IS_ON_STACK) {
		assert(remaining_allocations);

		assert(ra->index == RPLALLOC_STACK);
		assert(ra->type == TYPE_ADR);
		frame->javastack[stack_index].p = sp[-1];
		frame->javastacktype[stack_index] = TYPE_ADR; /* XXX RET */
		remaining_allocations--;
		stack_index++;
		ra++;
	}
	else if (topslot == TOP_IS_IN_ITMP1) {
		assert(remaining_allocations);

		assert(ra->index == RPLALLOC_STACK);
		assert(ra->type == TYPE_ADR);
		frame->javastack[stack_index].p = es->intregs[REG_ITMP1];
		frame->javastacktype[stack_index] = TYPE_ADR; /* XXX RET */
		remaining_allocations--;
		stack_index++;
		ra++;
	}
	else if (topslot == TOP_IS_VOID) {
		assert(remaining_allocations);

		assert(ra->index == RPLALLOC_STACK);
		frame->javastack[stack_index].l = 0;
		frame->javastacktype[stack_index] = TYPE_VOID;
		remaining_allocations--;
		stack_index++;
		ra++;
	}

	/* read remaining stack slots */

	for (; remaining_allocations--; ra++) {
		if (ra->index == RPLALLOC_SYNC) {
			assert(rp->type == rplpoint::TYPE_INLINE);

			/* only read synchronization slots when traversing an inline point */

			if (!topframe) {
				sourceframe_t *calleeframe = frame->down;
				assert(calleeframe);
				assert(calleeframe->syncslotcount == 0);
				assert(calleeframe->syncslots == NULL);

				calleeframe->syncslotcount = 1;
				calleeframe->syncslots = (replace_val_t*) DumpMemory::allocate(sizeof(replace_val_t));
				replace_read_value(es,ra,calleeframe->syncslots);
			}

			frame->javastackdepth--;
			continue;
		}

		assert(ra->index == RPLALLOC_STACK || ra->index == RPLALLOC_PARAM);

		/* do not read parameters of calls down the call chain */

		if (!topframe && ra->index == RPLALLOC_PARAM) {
			frame->javastackdepth--;
		} else {
			if (ra->type == TYPE_RET)
				frame->javastack[stack_index].i = ra->regoff;
			else
				replace_read_value(es,ra,frame->javastack + stack_index);
			frame->javastacktype[stack_index] = ra->type;
			stack_index++;
		}
	}

	LOG("recovered source frame: [" << frame << "]" << nl);
}


/* replace_write_executionstate ************************************************

   Pop a source frame from the front of the frame list of the given source state
   and write its values into the execution state.

   IN:
       rp...............replacement point for which execution state should be
	                    created
	   es...............the execution state to modify
	   ss...............the given source state
	   topframe.........true, if this is the last (top-most) source frame to be
	                    translated

   OUT:
       *es..............the execution state derived from the source state

*******************************************************************************/

static void replace_write_executionstate(rplpoint *rp,
										 executionstate_t *es,
										 sourcestate_t *ss)
{
	methodinfo     *m;
	rplalloc       *ra;
	sourceframe_t  *frame;
	int             topslot;
	stackslot_t    *sp;
	bool            topframe;

	LOG("write execution state for " << rp << nl);

	m = rp->method;
	topslot = TOP_IS_NORMAL;
	topframe = ss->frames->down == NULL;

	/* pop a source frame */

	frame = ss->frames;
	assert(frame);
	ss->frames = frame->down;

	/* calculate stack pointer */

	sp = (stackslot_t *) es->sp;

#if defined(__X86_64__)
	if (code_is_using_frameptr(frame->tocode)) {
		/* The frame pointer has to point to the beginning of the stack
		   frame. */
		es->intregs[RBP] = (uintptr_t) (sp + frame->tocode->stackframesize - 1);
		LOG2("set RBP to " << (uintptr_t*) es->intregs[RBP] << nl);
	}
#endif

	/* in some cases the top stack slot is passed in REG_ITMP1 */

#if 0
	if (rp->type == rplpoint::TYPE_EXH) {
		topslot = TOP_IS_IN_ITMP1;
	}
#endif

	/* write javalocals */

	ra = rp->regalloc;
	int remaining_allocations = rp->regalloccount;

	while (remaining_allocations && ra->index >= 0) {
		assert(ra->index < m->maxlocals);
		assert(ra->index < frame->javalocalcount);
		assert(ra->type == frame->javalocaltype[ra->index]);
		if (ra->type == TYPE_RET) {
			/* XXX assert that it matches this rplpoint */
		} else {
			replace_write_value(es, ra, frame->javalocals + ra->index);
		}
		remaining_allocations--;
		ra++;
	}

	/* write stack slots */

	int stack_index = 0;

	/* the first stack slot is special in SBR and EXH blocks */

	if (topslot == TOP_IS_ON_STACK) {
		assert(remaining_allocations);

		assert(ra->index == RPLALLOC_STACK);
		assert(stack_index < frame->javastackdepth);
		assert(frame->javastacktype[stack_index] == TYPE_ADR);
		sp[-1] = frame->javastack[stack_index].p;
		remaining_allocations--;
		stack_index++;
		ra++;
	}
	else if (topslot == TOP_IS_IN_ITMP1) {
		assert(remaining_allocations);

		assert(ra->index == RPLALLOC_STACK);
		assert(stack_index < frame->javastackdepth);
		assert(frame->javastacktype[stack_index] == TYPE_ADR);
		es->intregs[REG_ITMP1] = frame->javastack[stack_index].p;
		remaining_allocations--;
		stack_index++;
		ra++;
	}
	else if (topslot == TOP_IS_VOID) {
		assert(remaining_allocations);

		assert(ra->index == RPLALLOC_STACK);
		assert(stack_index < frame->javastackdepth);
		assert(frame->javastacktype[stack_index] == TYPE_VOID);
		remaining_allocations--;
		stack_index++;
		ra++;
	}

	/* write remaining stack slots */

	for (; remaining_allocations--; ra++) {
		if (ra->index == RPLALLOC_SYNC) {
			assert(rp->type == rplpoint::TYPE_INLINE);

			/* only write synchronization slots when traversing an inline point */

			if (!topframe) {
				assert(frame->down);
				assert(frame->down->syncslotcount == 1); /* XXX need to understand more cases */
				assert(frame->down->syncslots != NULL);

				replace_write_value(es,ra,frame->down->syncslots);
			}
			continue;
		}

		assert(ra->index == RPLALLOC_STACK || ra->index == RPLALLOC_PARAM);

		/* do not write parameters of calls down the call chain */

		if (!topframe && ra->index == RPLALLOC_PARAM) {
			/* skip it */
			/*
			ra->index = RPLALLOC_PARAM;
			replace_val_t v;
			v.l = 0;
			replace_write_value(es,ra,&v);
			*/
		}
		else {
			assert(stack_index < frame->javastackdepth);
			assert(ra->type == frame->javastacktype[stack_index]);
			if (ra->type == TYPE_RET) {
				/* XXX assert that it matches this rplpoint */
			}
			else {
				replace_write_value(es,ra,frame->javastack + stack_index);
			}
			stack_index++;
		}
	}

	/* set new pc */

	es->pc = rp->pc;
}


/* md_push_stackframe **********************************************************

   Save the given return address, build the new stackframe,
   and store callee-saved registers.

   *** This function imitates the effects of a call and the ***
   *** method prolog of the callee.                         ***

   IN:
       es...............execution state
       calleecode.......the code we are "calling"
       ra...............the return address to save

   OUT:
       *es..............the execution state after pushing the stack frame
                        NOTE: es->pc, es->code, and es->pv are NOT updated.

*******************************************************************************/

void md_push_stackframe(executionstate_t *es, codeinfo *calleecode, u1 *ra)
{
	s4           reg;
	s4           i;
	stackslot_t *basesp;
	stackslot_t *sp;

	assert(es);
	assert(calleecode);

	/* write the return address */

#if STACKFRAME_RA_BETWEEN_FRAMES
	es->sp -= SIZEOF_VOID_P;
	*((void **)es->sp) = (void *) ra;
	if (calleecode->stackframesize)
		es->sp -= (SIZE_OF_STACKSLOT - SIZEOF_VOID_P);
#endif /* STACKFRAME_RA_BETWEEN_FRAMES */

	es->ra = (u1*) (ptrint) ra;

	/* build the stackframe */

	LOG("building stackframe of " << calleecode->stackframesize << " words at "
			<< es->sp << nl);

	sp = (stackslot_t *) es->sp;
	basesp = sp;

	sp -= calleecode->stackframesize;
	es->sp = (u1*) sp;

	/* in debug mode, invalidate stack frame first */

	/* XXX may not invalidate linkage area used by native code! */

#if !defined(NDEBUG) && 0
	for (i=0; i< (basesp - sp) && i < 1; ++i) {
		sp[i] = 0xdeaddeadU;
	}
#endif

#if defined(__I386__)
	/* Stackslot 0 may contain the object instance for vftbl patching.
	   Destroy it, so there's no undefined value used. */
	// XXX Is this also true for x86_64?
	if ((basesp - sp) > 0) {
		sp[0] = 0;
	}
#endif

	/* save the return address register */

#if STACKFRAME_RA_TOP_OF_FRAME
# if STACKFRAME_LEAFMETHODS_RA_REGISTER
	if (!code_is_leafmethod(calleecode))
# endif
		*--basesp = (ptrint) ra;
#endif /* STACKFRAME_RA_TOP_OF_FRAME */

#if STACKFRAME_RA_LINKAGE_AREA
# if STACKFRAME_LEAFMETHODS_RA_REGISTER
	if (!code_is_leafmethod(calleecode))
# endif
		*((uint8_t**) ((intptr_t) basesp + LA_LR_OFFSET)) = ra;
#endif /* STACKFRAME_RA_LINKAGE_AREA */

	/* save int registers */

#if defined(__X86_64__)
	if (code_is_using_frameptr(calleecode)) {
		*((uintptr_t*) --basesp) = es->intregs[RBP];
		LOG2("push rbp onto stack " << (uintptr_t*) es->intregs[RBP] << nl);
	}
#endif

	reg = INT_REG_CNT;
	for (i=0; i<calleecode->savedintcount; ++i) {
		while (nregdescint[--reg] != REG_SAV)
			;
		basesp -= 1;
		*((uintptr_t*) basesp) = es->intregs[reg];

		/* XXX may not clobber saved regs used by native code! */
#if !defined(NDEBUG) && 0
		es->intregs[reg] = (ptrint) 0x44dead4444dead44ULL;
#endif

		LOG2("push " << abi_registers_integer_name[reg] << " onto stack" << nl);
	}

	/* save flt registers */

	/* XXX align? */
	reg = FLT_REG_CNT;
	for (i=0; i<calleecode->savedfltcount; ++i) {
		while (nregdescfloat[--reg] != REG_SAV)
			;
		basesp -= STACK_SLOTS_PER_FLOAT;
		*((double*) basesp) = es->fltregs[reg];

		/* XXX may not clobber saved regs used by native code! */
#if !defined(NDEBUG) && 0
		*(u8*)&(es->fltregs[reg]) = 0x44dead4444dead44ULL;
#endif
	}
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

void replace_pop_activation_record(executionstate_t *es,
								  sourceframe_t *frame)
{
	u1 *ra;
	s4 i;
	s4 count;
	codeinfo *code;
	stackslot_t *sp;

	assert(es->code);
	assert(frame);

	LOG("pop activation record for method [" << frame->method->name << "]" << nl);

	/* calculate the base of the stack frame */

	sp = (stackslot_t *) es->sp;
	assert(frame->syncslotcount == 0);
	assert(frame->syncslots == NULL);
	count = code_get_sync_slot_count(es->code);
	frame->syncslotcount = count;
	frame->syncslots = (replace_val_t*) DumpMemory::allocate(sizeof(replace_val_t) * count);
	for (i=0; i<count; ++i) {
		frame->syncslots[i].p = *((intptr_t*) (sp + es->code->memuse + i)); /* XXX md_ function */
	}

	/* pop the stackframe */

	executionstate_pop_stackframe(es);

	/* Subtract one from the PC so we do not hit the replacement point */
	/* of the instruction following the call, if there is one.         */

	es->pc--;

	ra = es->pc;

	/* find the new codeinfo */

	void* pv = md_codegen_get_pv_from_pc(ra);

	code = code_get_codeinfo_for_pv(pv);

	es->pv   = (uint8_t*) pv;
	es->code = code;
}


/* replace_patch_method_pointer ************************************************

   Patch a method pointer (may be in code, data segment, vftbl, or interface
   table).

   IN:
	   mpp..............address of the method pointer to patch
	   entrypoint.......the new entrypoint of the method
	   kind.............kind of call to patch, used only for debugging

*******************************************************************************/

static void replace_patch_method_pointer(methodptr *mpp,
										 methodptr entrypoint,
										 const char *kind)
{
#if !defined(NDEBUG)
	codeinfo *oldcode = code_get_codeinfo_for_pv(*mpp);
	codeinfo *newcode = code_get_codeinfo_for_pv(entrypoint);

	assert(oldcode->m == newcode->m);
#endif

	LOG("patch method pointer from " << (void *) *mpp << " to "
		<< (void *) entrypoint << nl);

	/* write the new entrypoint */

	*mpp = (methodptr) entrypoint;
}


/* replace_patch_class *********************************************************

   Patch a method in the given class.

   IN:
	   vftbl............vftbl of the class
	   m................the method to patch
	   oldentrypoint....the old entrypoint to replace
	   entrypoint.......the new entrypoint

*******************************************************************************/

void replace_patch_class(vftbl_t *vftbl,
						 methodinfo *m,
						 u1 *oldentrypoint,
						 u1 *entrypoint)
{
	s4                 i;
	methodptr         *mpp;
	methodptr         *mppend;

	LOG("patch class " << vftbl->clazz->name << nl);

	/* patch the vftbl of the class */

	replace_patch_method_pointer(vftbl->table + m->vftblindex,
								 entrypoint,
								 "virtual  ");

	/* patch the interface tables */

	assert(oldentrypoint);

	for (i=0; i < vftbl->interfacetablelength; ++i) {
		mpp = vftbl->interfacetable[-i];
		mppend = mpp + vftbl->interfacevftbllength[i];
		for (; mpp != mppend; ++mpp)
			if (*mpp == oldentrypoint) {
				replace_patch_method_pointer(mpp, entrypoint, "interface");
			}
	}
}


/* replace_patch_class_hierarchy ***********************************************

   Patch a method in all loaded classes.

   IN:
	   m................the method to patch
	   oldentrypoint....the old entrypoint to replace
	   entrypoint.......the new entrypoint

*******************************************************************************/

struct replace_patch_data_t {
	methodinfo *m;
	u1         *oldentrypoint;
	u1         *entrypoint;
};

void replace_patch_callback(classinfo *c, struct replace_patch_data_t *pd)
{
	vftbl_t *vftbl = c->vftbl;

	if (vftbl != NULL
		&& vftbl->vftbllength > pd->m->vftblindex
		// TODO understand
		//&& (void*) vftbl->table[pd->m->vftblindex] != (void*) (uintptr_t) &asm_abstractmethoderror
		&& code_get_methodinfo_for_pv(vftbl->table[pd->m->vftblindex]) == pd->m)
	{
		replace_patch_class(c->vftbl, pd->m, pd->oldentrypoint, pd->entrypoint);
	}
}

void replace_patch_class_hierarchy(methodinfo *m,
								   u1 *oldentrypoint,
								   u1 *entrypoint)
{
	struct replace_patch_data_t pd;

	pd.m = m;
	pd.oldentrypoint = oldentrypoint;
	pd.entrypoint = entrypoint;

	LOG("patching class hierarchy: " << *m << nl);

	classcache_foreach_loaded_class(
			(classcache_foreach_functionptr_t) &replace_patch_callback,
			(void*) &pd);
}


/* replace_patch_future_calls **************************************************

   Analyse a call site and depending on the kind of call patch the call, the
   virtual function table, or the interface table.

   IN:
	   ra...............return address pointing after the call site
	   callerframe......source frame of the caller
	   calleeframe......source frame of the callee, must have been mapped

*******************************************************************************/

void replace_patch_future_calls(u1 *ra,
								sourceframe_t *callerframe,
								sourceframe_t *calleeframe)
{
	u1            *patchpos;
	methodptr      entrypoint;
	methodptr      oldentrypoint;
#if !defined(__I386__)
	bool           atentry;
#endif
	void          *pv;
	codeinfo      *calleecode;
	methodinfo    *calleem;
	java_object_t *obj;
	vftbl_t       *vftbl;

	assert(ra);
	assert(callerframe->down == calleeframe);

	/* get the new codeinfo and the method that shall be entered */

	calleecode = calleeframe->tocode;
	assert(calleecode);

	calleem = calleeframe->method;
	assert(calleem == calleecode->m);

	entrypoint = (methodptr) calleecode->entrypoint;

	/* check if we are at an method entry rplpoint at the innermost frame */

#if !defined(__I386__)
	atentry = (calleeframe->down == NULL)
			&& !(calleem->flags & ACC_STATIC)
			&& (calleeframe->fromrp->id == 0); /* XXX */
#endif

	/* get the position to patch, in case it was a statically bound call   */

	pv = callerframe->fromcode->entrypoint;
	patchpos = (u1*) md_jit_method_patch_address(pv, ra, NULL);

	if (patchpos == NULL) {
		/* the call was dispatched dynamically */

		/* we can only patch such calls if we are at the entry point */

#if !defined(__I386__)
		/* On i386 we always know the instance argument. */
		if (!atentry)
			return;
#endif

		assert((calleem->flags & ACC_STATIC) == 0);

		oldentrypoint = calleeframe->fromcode->entrypoint;

		/* we need to know the instance */

		if (!calleeframe->instance.a) {
			LOG(BoldYellow << "WARNING: " << reset_color << "object instance unknown!" << nl);
			replace_patch_class_hierarchy(calleem, oldentrypoint, entrypoint);
			return;
		}

		/* get the vftbl */

		obj = calleeframe->instance.a;
		vftbl = obj->vftbl;

		assert(vftbl->clazz->vftbl == vftbl);

		LOG("class: " << vftbl->clazz);

		replace_patch_class(vftbl, calleem, oldentrypoint, entrypoint);
	} else {
		/* the call was statically bound */

#if defined(__I386__) || defined(__X86_64__)
		/* It happens that there is a patcher trap. (pm) */
		if (*(u2 *)(patchpos - 1) == 0x0b0f) {
			LOG2("enountered patcher trap, not patching static call");
			return;
		}
#endif
		LOG2("patch static call of [" << *calleem << "] in ["
			<< *callerframe->method << "]" << nl);

		replace_patch_method_pointer((methodptr *) patchpos, entrypoint, "static   ");
	}
}


/* replace_push_activation_record **********************************************

   Push a stack frame onto the execution state.

   *** This function imitates the effects of a call and the ***
   *** method prolog of the callee.                         ***

   IN:
	   es...............execution state
	   rpcall...........the replacement point at the call site
	   callerframe......source frame of the caller, or NULL for creating the
	                    first frame
	   calleeframe......source frame of the callee, must have been mapped

   OUT:
       *es..............the execution state after pushing the stack frame

*******************************************************************************/

void replace_push_activation_record(executionstate_t *es,
									rplpoint *rpcall,
									sourceframe_t *callerframe,
									sourceframe_t *calleeframe)
{
	s4           count;
	stackslot_t *sp;
	u1          *ra;
	codeinfo    *calleecode;

	assert(es);
	assert(!rpcall || callerframe);
#if 0
	assert(!rpcall || rpcall->type == rplpoint::TYPE_CALL);
#endif
	assert(!rpcall || rpcall == callerframe->torp);
	assert(calleeframe);
	assert(!callerframe || calleeframe == callerframe->down);

	LOG("push activation record for " << *calleeframe->method << nl);

	/* the compilation unit we are entering */

	calleecode = calleeframe->tocode;
	assert(calleecode);

	/* calculate the return address */

	// XXX why is ra == es->pc for native frames but not for
	//     non-native ones?
	if (rpcall) {
		ra = rpcall->pc + rpcall->callsize;
	} else {
#if 1
		// XXX we only need to do this if we decrement the pc in
		//     `replace_pop_activation_record`
		ra = es->pc + 1;
#else
		ra = es->pc;
#endif
	}

	/* push the stackframe */

	md_push_stackframe(es, calleecode, ra);

	/* we move into a new code unit, set code, PC, PV */

	es->code = calleecode;
#if 0
	es->pc = calleecode->entrypoint; /* XXX not needed? */
#endif
	es->pv = calleecode->entrypoint;

	/* write slots used for synchronization */

	sp = (stackslot_t *) es->sp;
	count = code_get_sync_slot_count(calleecode);
	assert(count == calleeframe->syncslotcount);
	for (s4 slot_index = 0; slot_index < count; ++slot_index) {
		*((intptr_t*) (sp + calleecode->memuse + slot_index)) = calleeframe->syncslots[slot_index].p;
	}

	/* redirect future invocations */

	if (callerframe && rpcall) {
#if !defined(REPLACE_PATCH_ALL)
		if (rpcall == callerframe->fromrp)
#endif
			replace_patch_future_calls(ra, callerframe, calleeframe);
	}
}

rplpoint *replace_find_replacement_point_at_call_site(codeinfo *code,
													  sourceframe_t *frame)
{
	assert(code);
	assert(frame);

	LOG("searching for call site rp for source id " << frame->id
			<< " in method [" << *frame->method << "]" << nl);

	rplpoint *rp = code->rplpoints;

	for (s4 i = 0; i < code->rplpointcount; i++, rp++) {
		if (rp->id == frame->id
				&& rp->method == frame->method
				&& REPLACE_IS_CALL_SITE(rp)) {
			return rp;
		}
	}

	ABORT_MSG("no matching replacement point found", "");
	return NULL; /* NOT REACHED */
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
										  sourceframe_t *frame)
{
	rplpoint *rp;
	s4        stacki;
	rplalloc *ra;

	assert(code);
	assert(frame);

	LOG("searching for rp for source id " << frame->id
			<< " in method [" << *frame->method << "]" << nl);

	rp = code->rplpoints;
	for (s4 i = 0; i < code->rplpointcount; i++, rp++) {
		if (rp->id == frame->id && rp->method == frame->method) {
			/* check if returnAddresses match */
			/* XXX optimize: only do this if JSRs in method */
			ra = rp->regalloc;
			stacki = 0;
			for (s4 j = rp->regalloccount; j--; ++ra) {
				if (ra->type == TYPE_RET) {
					if (ra->index == RPLALLOC_STACK) {
						assert(stacki < frame->javastackdepth);
						if (frame->javastack[stacki].i != ra->regoff)
							continue;
						stacki++;
					} else {
						assert(ra->index >= 0 && ra->index < frame->javalocalcount);
						if (frame->javalocals[ra->index].i != ra->regoff)
							continue;
					}
				}
			}

			/* found */
			return rp;
		}
	}

	ABORT_MSG("no matching replacement point found", "");
	return NULL; /* NOT REACHED */
}

/* replace_find_replacement_point_at_or_before_pc ******************************

   Find the nearest replacement point at or before the given PC.

   IN:
       code.............compilation unit the PC is in
       pc...............the machine code PC

   RETURN VALUE:
       the replacement point found, or NULL if no replacement point was found

*******************************************************************************/


rplpoint *replace_find_replacement_point_at_or_before_pc(
		codeinfo *code,
		u1 *pc,
		unsigned int desired_flags) {
	LOG("searching for nearest rp before pc " << pc
			<< " in method [" << *code->m << "]" << nl);

	rplpoint *nearest = NULL;
	rplpoint *rp = code->rplpoints;

	for (s4 i = 0; i < code->rplpointcount; ++i, ++rp) {
		if (rp->pc <= pc && (rp->flags & desired_flags)) {
			if (nearest == NULL || nearest->pc < rp->pc) {
				nearest = rp;
			}
		}
	}

	return nearest;
}

/* replace_find_replacement_point_for_pc ***************************************

   Find the nearest replacement point whose PC is between (rp->pc) and
   (rp->pc+rp->callsize).

   IN:
       code.............compilation unit the PC is in
	   pc...............the machine code PC

   RETURN VALUE:
       the replacement point found, or
	   NULL if no replacement point was found

*******************************************************************************/

rplpoint *replace_find_replacement_point_for_pc(codeinfo *code, u1 *pc)
{
	rplpoint *rp;
	s4        i;

	LOG("searching for rp at pc " << pc << " in method [" << *code->m << "]"
			<< nl);

	rp = code->rplpoints;
	for (i=0; i<code->rplpointcount; ++i, ++rp) {
		if (rp->pc <= pc && rp->pc + rp->callsize >= pc) {
			return rp;
		}
	}

	return NULL;
}

/* replace_pop_native_frame ****************************************************

   Unroll a native frame in the execution state and create a source frame
   for it.

   IN:
	   es...............current execution state
	   ss...............the current source state
	   sfi..............stackframeinfo for the native frame

   OUT:
       es...............execution state after unrolling the native frame
	   ss...............gets the added native source frame

*******************************************************************************/

static void replace_pop_native_frame(executionstate_t *es,
									 sourcestate_t *ss,
									 stackframeinfo_t *sfi)
{
	sourceframe_t *frame;
	codeinfo      *code;
	s4             i,j;

	assert(sfi);

	frame = replace_new_sourceframe(ss);

	frame->sfi = sfi;

	/* remember pc and size of native frame */

	frame->nativepc = es->pc;
	frame->nativeframesize = (es->sp != 0) ? (((uintptr_t) sfi->sp) - ((uintptr_t) es->sp)) : 0;
	assert(frame->nativeframesize >= 0);

	/* remember values of saved registers */

	j = 0;
	for (i=0; i<INT_REG_CNT; ++i) {
		if (nregdescint[i] == REG_SAV)
			frame->nativesavint[j++] = es->intregs[i];
	}

	j = 0;
	for (i=0; i<FLT_REG_CNT; ++i) {
		if (nregdescfloat[i] == REG_SAV)
			frame->nativesavflt[j++] = es->fltregs[i];
	}

	/* restore saved registers */

	/* XXX we don't have them, yet, in the sfi, so clear them */

	for (i=0; i<INT_REG_CNT; ++i) {
		if (nregdescint[i] == REG_SAV)
			es->intregs[i] = 0;
	}

	/* XXX we don't have float registers in the sfi, so clear them */

	for (i=0; i<FLT_REG_CNT; ++i) {
		if (nregdescfloat[i] == REG_SAV)
			es->fltregs[i] = 0.0;
	}

	/* restore codeinfo of the native stub */

	code = code_get_codeinfo_for_pv(sfi->pv);

	/* restore sp, pv, pc and codeinfo of the parent method */

	es->sp   = (uint8_t*) (((uintptr_t) sfi->sp) + md_stacktrace_get_framesize(code));
#if STACKFRAME_RA_BETWEEN_FRAMES
	es->sp  += SIZEOF_VOID_P; /* skip return address */
#endif
	es->pv   = (uint8_t*) md_codegen_get_pv_from_pc(sfi->ra);
	es->pc   = (uint8_t*) (((uintptr_t) ((sfi->xpc) ? sfi->xpc : sfi->ra)) - 1);
	es->code = code_get_codeinfo_for_pv(es->pv);
}


/* replace_push_native_frame ***************************************************

   Rebuild a native frame onto the execution state and remove its source frame.

   Note: The native frame is "rebuild" by setting fields like PC and stack
         pointer in the execution state accordingly. Values in the
		 stackframeinfo may be modified, but the actual stack frame of the
		 native code is not touched.

   IN:
	   es...............current execution state
	   ss...............the current source state

   OUT:
       es...............execution state after re-rolling the native frame
	   ss...............the native source frame is removed

*******************************************************************************/

static void replace_push_native_frame(executionstate_t *es, sourcestate_t *ss)
{
	sourceframe_t *frame;
	s4             i,j;

	assert(es);
	assert(ss);

	LOG("push native frame" << nl);

	/* remove the frame from the source state */

	frame = ss->frames;
	assert(frame);
	assert(REPLACE_IS_NATIVE_FRAME(frame));

	ss->frames = frame->down;

	/* skip sp for the native stub */

	es->sp -= md_stacktrace_get_framesize(frame->sfi->code);
#if STACKFRAME_RA_BETWEEN_FRAMES
	es->sp -= SIZEOF_VOID_P; /* skip return address */
#endif

	/* assert that the native frame has not moved */

	assert(es->sp == frame->sfi->sp);

	/* restore saved registers */

	j = 0;
	for (i=0; i<INT_REG_CNT; ++i) {
		if (nregdescint[i] == REG_SAV)
			es->intregs[i] = frame->nativesavint[j++];
	}

	j = 0;
	for (i=0; i<FLT_REG_CNT; ++i) {
		if (nregdescfloat[i] == REG_SAV)
			es->fltregs[i] = frame->nativesavflt[j++];
	}

	/* skip the native frame on the machine stack */

	es->sp -= frame->nativeframesize;

	/* set the pc the next frame must return to */

	es->pc = frame->nativepc;
}

/* replace_recover_source_frame ************************************************

   Recovers a source frame from the given replacement point and execution
   state and pushes is to the front of the source state.

   IN:
       rp...............replacement point that has been reached, if any
	   es...............execution state at the replacement point rp
	   es...............the source state

	OUT:
       es...............the modified execution state
	   ss...............the new source state

   RETURN VALUE:
       the replacement point at the call site within the caller frame or NULL
       if it was called from a native method

*******************************************************************************/

rplpoint *replace_recover_source_frame(rplpoint *rp,
											   executionstate_t *es,
											   sourcestate_t *ss) {
	/* read the values for this source frame from the execution state */

	replace_read_executionstate(rp, es, ss);

	/* unroll to the next (outer) frame */

	rplpoint *next_rp = NULL;
	if (rp->parent) {
		/* this frame is in inlined code */

		LOG("INLINED!" << nl);

		next_rp = rp->parent;

#if 0
		assert(next_rp->type == rplpoint::TYPE_INLINE);
#endif
	} else {
		/* this frame had been called at machine-level. pop it. */

		replace_pop_activation_record(es, ss->frames);
		if (es->code == NULL) {
			LOG("REACHED NATIVE CODE" << nl);
			next_rp = NULL;
		} else {
			/* find the replacement point at the call site */

			next_rp = replace_find_replacement_point_for_pc(es->code, es->pc);

			assert(next_rp);
			assert(REPLACE_IS_CALL_SITE(next_rp));
		}
	}

	return next_rp;
}


/* replace_recover_inlined_source_state *******************************************

   Recover the source state from the given replacement point and execution
   state until the first non-inlined method is reached.

   IN:
       rp...............replacement point that has been reached, if any
       es...............execution state at the replacement point rp

   OUT:
       es...............the modified execution state

   RETURN VALUE:
       the source state

*******************************************************************************/

sourcestate_t *replace_recover_inlined_source_state(rplpoint *rp,
													executionstate_t *es)
{
	sourcestate_t *ss = (sourcestate_t*) DumpMemory::allocate(sizeof(sourcestate_t));
	ss->frames = NULL;

	/* recover source frames of inlined methods, if there are any */

	rplpoint *next_rp = rp;

#if 0
	while (next_rp->parent != NULL) {
		next_rp = replace_recover_source_frame(next_rp, es, ss);
		assert(next_rp);
	}
#endif

	/* recover the source frame of the first non-inlined method */

	next_rp = replace_recover_source_frame(next_rp, es, ss);

	/* recover the frame of the calling method (if it's not native) so that the
	   call can be patched if necessary */

	if (next_rp != NULL) {
		replace_recover_source_frame(next_rp, es, ss);
	}

	return ss;
}


/* replace_map_source_state_to_deoptimized_code ********************************

   Map each source frame in the given source state to a target replacement
   point and compilation unit (considering that the correspondings methods could
   be inlined).

   IN:
       ss...............the source state

   OUT:
       ss...............the source state, modified: The `torp` and `tocode`
	                    fields of each source frame are set.

*******************************************************************************/

static void replace_map_source_state_to_deoptimized_code(sourcestate_t *ss)
{
	codeinfo *code = NULL;

	assert(ss->frames);
	assert(!REPLACE_IS_NATIVE_FRAME(ss->frames));
	assert(ss->frames->down);
	assert(!ss->frames->down->down);

	/* iterate over the source frames from outermost to innermost */

	for (sourceframe_t *frame = ss->frames; frame != NULL; frame = frame->down) {

		assert(!REPLACE_IS_NATIVE_FRAME(frame));

		LOG("map frame " << frame << nl);

		/* find code for this frame */

		if (frame != ss->frames) {
			if (frame->method->deopttarget == NULL) {
				/* reinvoke baseline compiler and save depotimized code for
				   future replacements */

				LOG("reinvoke baseline compiler" << nl);
				jit_recompile_for_deoptimization(frame->method);
				frame->method->deopttarget = frame->method->code;
			}

			code = frame->method->deopttarget;
		} else {
			code = frame->method->code;
		}

		assert(code);

		/* prepare code for usage */

#if 0
		if (code_is_invalid(code)) {
			code_unflag_invalid(code);
			replace_deactivate_replacement_points(code);
		}
#endif

		rplpoint *rp;
		if (frame->down) {
			/* we have not yet reached the topmost frame on the stack, therefore
			   there execution currently stands at a call site */
			rp = replace_find_replacement_point_at_call_site(code, frame);
		} else {
			/* we have reached the topmost frame on the stack */
			rp = replace_find_replacement_point(code, frame);
		}

		assert(rp);

		/* map this frame */

		frame->tocode = code;
		frame->torp = rp;
	}
}


/* replace_build_execution_state ***********************************************

   Build an execution state for the given (mapped) source state.

   !!! CAUTION: This function rewrites the machine stack !!!

   THIS FUNCTION MUST BE CALLED USING A SAFE STACK AREA!

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
	rplpoint      *parent;

	parent = NULL;
	prevframe = NULL;
	rp = NULL;

	while (ss->frames) {
		if (REPLACE_IS_NATIVE_FRAME(ss->frames)) {
			prevframe = ss->frames;
			replace_push_native_frame(es, ss);
			parent = NULL;
			rp = NULL;
		} else {
			if (parent == NULL) {
				/* create a machine-level stack frame */

				replace_push_activation_record(es, rp, prevframe, ss->frames);
			}
	
			rp = ss->frames->torp;
			assert(rp);

			es->code = ss->frames->tocode;
			prevframe = ss->frames;

			replace_write_executionstate(rp, es, ss);
	
			if (REPLACE_IS_CALL_SITE(rp)) {
				parent = NULL;
			} else {
				/* inlining */
				parent = rp;
			}
		}
	}
}


/* replace_on_stack ************************************************************

   Performs on-stack replacement.

   THIS FUNCTION MUST BE CALLED USING A SAFE STACK AREA!
 
   IN:
       code.............the code of the method to be replaced.
       rp...............replacement point that has been reached
       es...............current execution state

   OUT:
       es...............the execution state after replacement finished.

*******************************************************************************/

static void replace_optimize(codeinfo *code, rplpoint *rp, executionstate_t *es)
{
	LOG(BoldCyan << "perform replacement" << reset_color << " at " << rp << nl);

	DumpMemoryArea dma;
	es->code = code;
	sourcestate_t *ss = (sourcestate_t*) DumpMemory::allocate(sizeof(sourcestate_t));
	ss->frames = NULL;

	/* recover the source frame of the first non-inlined method */

	rplpoint *rpcall = replace_recover_source_frame(rp, es, ss);

	/* recover the frame of the calling method (if it's not native) so that the
	   call can be patched if necessary */

	if (rpcall != NULL) {
		replace_recover_source_frame(rpcall, es, ss);
	}

	sourceframe_t *topframe;
	sourceframe_t *callerframe;

	if (ss->frames->down) {
		callerframe = ss->frames;
		topframe = ss->frames->down;
	} else {
		callerframe = NULL;
		topframe = ss->frames;
	}

	/* map the topmost frame to a replacement point in the optimized code */
	jit_request_optimization(topframe->method);
	topframe->tocode = jit_get_current_code(topframe->method);
	topframe->torp = replace_find_replacement_point(topframe->tocode, topframe);

	/* identity map the calling frame */

	if (callerframe) {
		callerframe->tocode = callerframe->fromcode;
		callerframe->torp = callerframe->fromrp;
	}

	/* rebuild execution state */

	replace_build_execution_state(ss, es);

	LOG(BoldGreen << "finished replacement: " << reset_color << "jump into optimized code" << nl);
}


/* replace_handle_countdown_trap ***********************************************

   This function is called by the signal handler. It recompiles the method that
   triggered the countdown trap and initiates on-stack replacement.

   THIS FUNCTION MUST BE CALLED USING A SAFE STACK AREA!

   IN:
       pc...............the program counter that triggered the countdown trap.
       es...............the execution state (machine state) at the countdown
                        trap.

   OUT:
       es...............the execution state after replacement finished.

*******************************************************************************/

void replace_handle_countdown_trap(u1 *pc, executionstate_t *es)
{
	LOG("handle countdown trap" << nl);
	
	/* search the codeinfo for the given PC */

	codeinfo *code = code_find_codeinfo_for_pc(pc);
	assert(code);

	/* search for a replacement point at the given PC */

	methodinfo *method = code->m;
	rplpoint *rp = replace_find_replacement_point_at_or_before_pc(code, pc, rplpoint::FLAG_COUNTDOWN);

	assert(rp);
	assert(rp->flags & rplpoint::FLAG_COUNTDOWN);
	assert(method->hitcountdown < 0);

	/* perform on-stack replacement */

	replace_optimize(code, rp, es);
}


/* replace_handle_replacement_trap *********************************************

   This function is called by the signal handler. Initiates on-stack replacement
   if there is an active replacement trap at the given PC.

   THIS FUNCTION MUST BE CALLED USING A SAFE STACK AREA!

   IN:
       pc...............the program counter that triggered the replacement trap
       es...............the execution state (machine state) at the
                        replacement trap.

   OUT:
       es...............the execution state after replacement finished.

   RETURNS:
       true.............if replacement was performed at the given pc.
       false............otherwise.

*******************************************************************************/

bool replace_handle_replacement_trap(u1 *pc, executionstate_t *es)
{
	/* search the codeinfo for the given PC */

	codeinfo *code = code_find_codeinfo_for_pc(pc);
	assert(code);

	/* search for a replacement point at the given PC */

	rplpoint *rp = replace_find_replacement_point_for_pc(code, pc);

	if (rp != NULL && (rp->flags & rplpoint::FLAG_ACTIVE)) {

		LOG("handle replacement trap" << nl);
	
		/* perform on-stack replacement */

		replace_optimize(code, rp, es);

		return true;
	}

	return false;
}


/* replace_handle_deoptimization_trap ******************************************

   This function is called by the signal handler. It deoptimizes the method that
   triggered the deopimization trap and initiates on-stack replacement.

   THIS FUNCTION MUST BE CALLED USING A SAFE STACK AREA!

   IN:
       pc...............the program counter that triggered the countdown trap.
       es...............the execution state (machine state) at the countdown
                        trap.

   OUT:
       es...............the execution state after replacement finished.

*******************************************************************************/

void replace_handle_deoptimization_trap(u1 *pc, executionstate_t *es) {
	LOG("handle deopimization trap" << nl);

	DumpMemoryArea dma;

	/* search the codeinfo for the given PC */

	codeinfo *code = code_find_codeinfo_for_pc(pc);
	es->code = code;
	assert(code);

	/* search for a replacement point at the given PC */

	methodinfo *method = code->m;
	rplpoint *rp = replace_find_replacement_point_at_or_before_pc(code, pc, rplpoint::FLAG_DEOPTIMIZE);
	assert(rp);
	assert(rp->flags & rplpoint::FLAG_DEOPTIMIZE);

	/* replace by deoptimized code */

	LOG("perform deoptimization at " << rp << nl);
	jit_invalidate_code(method);
	sourcestate_t *ss = replace_recover_inlined_source_state(rp, es);
	replace_map_source_state_to_deoptimized_code(ss);
	replace_build_execution_state(ss, es);

	LOG(BoldGreen << "finished deoptimization: " << reset_color << "jump into code" << nl);
}


/******************************************************************************/
/* NOTE: No important code below.                                             */
/******************************************************************************/

#define TYPECHAR(t)  (((t) >= 0 && (t) <= TYPE_RET) ? show_jit_type_letters[t] : '?')

namespace cacao {

OStream& operator<<(OStream &OS, const rplpoint *rp) {
	if (!rp) {
		return OS << "(rplpoint *) NULL";
	}
	return OS << *rp;
}

OStream& operator<<(OStream &OS, const rplpoint &rp) {
	OS << "[";

	OS << "id=" << rp.id << ", pc=" << rp.pc;

	OS << " flags=[";
	if (rp.flags & rplpoint::FLAG_ACTIVE) {
		OS << " ACTIVE ";
	}
	if (rp.flags & rplpoint::FLAG_TRAPPABLE) {
		OS << " TRAPPABLE ";
	}
	if (rp.flags & rplpoint::FLAG_COUNTDOWN) {
		OS << " COUNTDOWN ";
	}
	if (rp.flags & rplpoint::FLAG_DEOPTIMIZE) {
		OS << " DEOPTIMIZE ";
	}
	OS << "]";

	OS << ", parent=" << rp.parent << ", ";
	OS << "allocations=[";

	for (int j=0; j < rp.regalloccount; ++j) {
		if (j > 0) {
			OS << ", ";
		}
		OS << rp.regalloc[j];
	}

	OS << "], ";
	OS << "method=[" << *rp.method << "], ";
	OS << "callsize=" << rp.callsize;

	OS << "]";

	return OS;
}

OStream& operator<<(OStream &OS, const rplalloc *ra) {
	if (!ra) {
		return OS << "(rplalloc *) NULL";
	}
	return OS << *ra;
}

OStream& operator<<(OStream &OS, const rplalloc &ra) {
	OS << "[";

	switch (ra.index) {
		case RPLALLOC_STACK: OS << "S"; break;
		case RPLALLOC_PARAM: OS << "P"; break;
		case RPLALLOC_SYNC : OS << "Y"; break;
		default: OS << ra.index;
	}

	OS << ":" << TYPECHAR(ra.type) << ", ";

	if (ra.type == TYPE_RET) {
		OS << "ret(L" << ra.regoff << ")";
	} else if (ra.inmemory) {
		OS << "M" << ra.regoff;
	} else if (IS_FLT_DBL_TYPE(ra.type)) {
		OS << "F" << ra.regoff;
	} else {
		/* integer register */

#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
		if (IS_2_WORD_TYPE(ra.type)) {
# if defined(ENABLE_JIT)
			OS << abi_registers_integer_name[GET_LOW_REG(ra.regoff)];
			OS << "/";
			OS << abi_registers_integer_name[GET_HIGH_REG(ra.regoff)];
# else  /* defined(ENABLE_JIT) */
			OS << GET_LOW_REG(ra.regoff) << "/" << GET_HIGH_REG(ra.regoff);
# endif /* defined(ENABLE_JIT) */
		} else {
#endif /* defined(SUPPORT_COMBINE_INTEGER_REGISTERS) */

			OS << abi_registers_integer_name[ra.regoff];

#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
		}
#endif /* defined(SUPPORT_COMBINE_INTEGER_REGISTERS) */
	}

	OS << "]";

	return OS;
}

OStream& operator<<(OStream &OS, const sourceframe_t *frame) {
	if (!frame) {
		return OS << "(sourceframe_t *) NULL";
	}
	return OS << *frame;
}

static OStream& replace_val_print(OStream &OS, s4 type, replace_val_t value) {
	java_object_t *obj;
	Utf8String     u;

	OS << "type=";
	if (type < 0 || type > TYPE_RET) {
		OS << "<INVALID TYPE:" << type << ">";
	} else {
		OS << show_jit_type_names[type];
	}
	
	OS << ", raw=" << value.l << ", value=";

	if (type == TYPE_ADR && value.a != NULL) {
		obj = value.a;
		OS << obj->vftbl->clazz->name;

#if 0
		if (obj->vftbl->clazz == class_java_lang_String) {
			printf(" \"");
			u = JavaString(obj).to_utf8();
			utf_display_printable_ascii(u);
			printf("\"");
		}
#endif
	} else if (type == TYPE_INT) {
		OS << value.i;
	} else if (type == TYPE_LNG) {
		OS << value.l;
	} else if (type == TYPE_FLT) {
		OS << value.f;
	} else if (type == TYPE_DBL) {
		OS << value.d;
	}

	return OS;
}

static OStream& print_replace_vals(OStream &OS, replace_val_t *values, u1 *types, s4 count) {
	for (s4 i = 0; i < count; ++i) {
		s4 type = types[i];
		OS << "[";
		if (type == TYPE_VOID) {
			OS << "type=void";
		} else {
			replace_val_print(OS, type, values[i]);
		}
		OS << "]";

		if (i + 1 < count) {
			OS << ", ";
		}
	}
	return OS;
}

OStream& operator<<(OStream &OS, const sourceframe_t &frame) {
	OS << "[";

	if (REPLACE_IS_NATIVE_FRAME(&frame)) {
		OS << "NATIVE, ";
		OS << "nativepc=" << frame.nativepc << nl;
		OS << "framesize=" << frame.nativeframesize << nl;

		OS << "registers (integer)=[";
		for (s4 i = 0, j = 0; i < INT_REG_CNT; ++i) {
			if (nregdescint[i] == REG_SAV) {
				OS << abi_registers_integer_name[i] << ": " << (void*)frame.nativesavint[j++] << ", ";
			}
		}
		OS << "] ";

		OS << "registers (float)=[";
		for (s4 i = 0, j = 0; i < FLT_REG_CNT; ++i) {
			if (nregdescfloat[i] == REG_SAV) {
				OS << "F" << i << ": " << frame.nativesavflt[j++] << ", ";
			}
		}
		OS << "]";
	} else {
		OS << "method=" << frame.method->name << ", ";
		OS << "id=" << frame.id;

#if 0
		if (frame->instance.a) {
			printf("\tinstance: ");
			java_value_print(TYPE_ADR, frame->instance);
			printf("\n");
		}
#endif

		if (frame.javalocalcount) {
			OS << ", locals (" << frame.javalocalcount << ")=[";
			print_replace_vals(OS, frame.javalocals, frame.javalocaltype,
					frame.javalocalcount);
			OS << "]";
		}

		if (frame.javastackdepth) {
			OS << ", stack (depth=" << frame.javastackdepth << ")=[";
			print_replace_vals(OS, frame.javastack, frame.javastacktype,
					frame.javastackdepth);
			OS << "]";
		}

#if 0
		if (frame->syncslotcount) {
			printf("\tsynchronization slots (%d):\n",frame->syncslotcount);
			for (i=0; i<frame->syncslotcount; ++i) {
				printf("\tslot[%2d] = %016llx\n",i,(unsigned long long) frame->syncslots[i].p);
			}
			printf("\n");
		}
#endif

		if (frame.fromcode) {
			OS << ", from=" << (void*) frame.fromcode;
		}
		if (frame.tocode) {
			OS << ", to=" << (void*) frame.tocode;
		}

	}

	OS << "]";

	return OS;
}

OStream& operator<<(OStream &OS, const sourcestate_t *ss) {
	if (!ss) {
		return OS << "(sourcestate_t *) NULL" << nl;
	}
	return OS << *ss;
}

OStream& operator<<(OStream &OS, const sourcestate_t &ss) {
	OS << "sourcestate_t:" << nl;
	int i;
	sourceframe_t *frame;
	for (i = 0, frame = ss.frames; frame != NULL; frame = frame->down, ++i) {
		OS << "frame " << i << ": [" << frame << "]" << nl;
	}
	return OS;
}


} // end namespace cacao


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
