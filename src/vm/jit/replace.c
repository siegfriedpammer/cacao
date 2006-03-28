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

#include "mm/memory.h"
#include "toolbox/logging.h"
#include "vm/options.h"
#include "vm/jit/jit.h"
#include "vm/jit/replace.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/disass.h"
#include "arch.h"

/*** constants used internally ************************************************/

#define TOP_IS_NORMAL    0
#define TOP_IS_ON_STACK  1
#define TOP_IS_IN_ITMP1  2

/* replace_create_replacement_points *******************************************
 
   Create the replacement points for the given code.
  
   IN:
       code.............codeinfo where replacement points should be stored
	                    code->rplpoints must be NULL.
						code->rplpointcount must be 0.
	   rd...............registerdata containing allocation info.
  
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
	codeinfo     *code;
	registerdata *rd;
	basicblock *bptr;
	int count;
	methodinfo *m;
	rplpoint *rplpoints;
	rplpoint *rp;
	int alloccount;
	int globalcount;
	rplalloc *regalloc;
	rplalloc *ra;
	int i;
	int t;
	stackptr sp;
	bool indexused;

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
	for (bptr = m->basicblocks; bptr; bptr = bptr->next) {
		if (!(bptr->bitflags & BBFLAG_REPLACEMENT))
			continue;

		/* there will be a replacement point at the start of this block */
		
		count++;
		alloccount += bptr->indepth;
	}

	/* if no points were found, there's nothing to do */
	
	if (!count)
		return true;

	/* count global register allocations */

	globalcount = 0;

	for (i=0; i<m->maxlocals; ++i) {
		indexused = false;
		for (t=0; t<5; ++t) {
#if defined(ENABLE_INTRP)
			if (!opt_intrp) {
#endif
				if (rd->locals[i][t].type == t) {
					globalcount++;
					indexused = true;
				}
#if defined(ENABLE_INTRP)
			}
#endif
		}
		if (!indexused)
			globalcount++; /* dummy rplalloc */
	}

	alloccount += globalcount;

	/* allocate replacement point array and allocation array */
	
	rplpoints = MNEW(rplpoint,count);
	regalloc = MNEW(rplalloc,alloccount);
	ra = regalloc;

	/* store global register allocations */

	for (i=0; i<m->maxlocals; ++i) {
		indexused = false;
		for (t=0; t<5; ++t) {
#if defined(ENABLE_INTRP)
			if (!opt_intrp) {
#endif
				if (rd->locals[i][t].type == t) {
					ra->flags = rd->locals[i][t].flags & (INMEMORY);
					ra->index = rd->locals[i][t].regoff;
					ra->type  = t;
					ra->next = (indexused) ? 0 : 1;
					ra++;
					indexused = true;
				}
#if defined(ENABLE_INTRP)
			}
#endif
		}
		if (!indexused) {
			/* dummy rplalloc */
			ra->type = -1;
			ra->flags = 0;
			ra->index = 0;
			ra->next = 1;
			ra++;
		}
	}

	/* initialize replacement point structs */

	rp = rplpoints;
	for (bptr = m->basicblocks; bptr; bptr = bptr->next) {
		if (!(bptr->bitflags & BBFLAG_REPLACEMENT))
			continue;

		/* there will be a replacement point at the start of this block */

		rp->pc = NULL;        /* set by codegen */
		rp->outcode = NULL;   /* set by codegen */
		rp->code = code;
		rp->target = NULL;
		rp->regalloc = ra;
		rp->flags = 0;
		rp->type = bptr->type;

		/* store local allocation info */

		for (sp = bptr->instack; sp; sp = sp->prev) {
			ra->flags = sp->flags & (INMEMORY);
			ra->index = sp->regoff;
			ra->type  = sp->type;
			ra->next  = 1;
			ra++;
		}

		rp->regalloccount = ra - rp->regalloc;
		
		rp++;
	}

	/* store the data in the codeinfo */

	code->rplpoints = rplpoints;
	code->rplpointcount = count;
	code->regalloc = regalloc;
	code->regalloccount = alloccount;
	code->globalcount = globalcount;
	code->savedintcount = INT_SAV_CNT - rd->savintreguse;
	code->savedfltcount = FLT_SAV_CNT - rd->savfltreguse;
	code->memuse = rd->memuse;
	code->isleafmethod = m->isleafmethod; /* XXX will be moved to codeinfo */

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
	
#ifndef NDEBUG
	printf("activate replacement point: ");
	replace_replacement_point_println(rp);
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
	
#ifndef NDEBUG
	printf("deactivate replacement point: ");
	replace_replacement_point_println(rp);
	fflush(stdout);
#endif
	
	rp->target = NULL;
	
#if (defined(__I386__) || defined(__X86_64__) || defined(__ALPHA__) || defined(__POWERPC__) || defined(__MIPS__)) && defined(ENABLE_JIT)
	md_patch_replacement_point(rp);
#endif
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

inline static void replace_read_value(executionstate *es,
#ifdef HAS_4BYTE_STACKSLOT
									  u4 *sp,
#else
									  u8 *sp,
#endif
									  rplalloc *ra,
									  u8 *javaval)
{
	if (ra->flags & INMEMORY) {
		/* XXX HAS_4BYTE_STACKSLOT may not be the right discriminant here */
#ifdef HAS_4BYTE_STACKSLOT
		if (IS_2_WORD_TYPE(ra->type)) {
			*javaval = *(u8*)(sp + ra->index);
		}
		else {
#endif
			*javaval = sp[ra->index];
#ifdef HAS_4BYTE_STACKSLOT
		}
#endif
	}
	else {
		/* allocated register */
		if (IS_FLT_DBL_TYPE(ra->type)) {
			*javaval = es->fltregs[ra->index];
		}
		else {
			*javaval = es->intregs[ra->index];
		}
	}
}

inline static void replace_write_value(executionstate *es,
#ifdef HAS_4BYTE_STACKSLOT
									  u4 *sp,
#else
									  u8 *sp,
#endif
									  rplalloc *ra,
									  u8 *javaval)
{
	if (ra->flags & INMEMORY) {
		/* XXX HAS_4BYTE_STACKSLOT may not be the right discriminant here */
#ifdef HAS_4BYTE_STACKSLOT
		if (IS_2_WORD_TYPE(ra->type)) {
			*(u8*)(sp + ra->index) = *javaval;
		}
		else {
#endif
			sp[ra->index] = *javaval;
#ifdef HAS_4BYTE_STACKSLOT
		}
#endif
	}
	else {
		/* allocated register */
		if (IS_FLT_DBL_TYPE(ra->type)) {
			es->fltregs[ra->index] = *javaval;
		}
		else {
			es->intregs[ra->index] = *javaval;
		}
	}
}

static void replace_read_executionstate(rplpoint *rp,executionstate *es,
									 sourcestate *ss)
{
	methodinfo *m;
	codeinfo *code;
	int count;
	int i;
	int t;
	int allocs;
	rplalloc *ra;
	methoddesc *md;
	int topslot;
#ifdef HAS_4BYTE_STACKSLOT
	u4 *sp;
	u4 *basesp;
#else
	u8 *sp;
	u8 *basesp;
#endif

	code = rp->code;
	m = code->m;
	md = m->parseddesc;
	topslot = TOP_IS_NORMAL;

	/* stack pointers */

#ifdef HAS_4BYTE_STACKSLOT
	sp = (u4*) es->sp;
#else
	sp = (u8*) es->sp;
#endif

	/* on some architectures the returnAddress is passed on the stack by JSR */

#if defined(__I386__) || defined(__X86_64__)
	if (rp->type == BBTYPE_SBR) {
		sp++;
		topslot = TOP_IS_ON_STACK;
	}
#endif

	/* in some cases the top stack slot is passed in REG_ITMP1 */

	if (  (rp->type == BBTYPE_EXH)
#if defined(__ALPHA__) || defined(__POWERPC__) || defined(__MIPS__)
	   || (rp->type == BBTYPE_SBR)
#endif
	   )
	{
		topslot = TOP_IS_IN_ITMP1;
	}

	/* calculate base stack pointer */
	
	basesp = sp + code_get_stack_frame_size(code);

	ss->stackbase = (u1*) basesp;

	/* read local variables */

	count = m->maxlocals;
	ss->javalocalcount = count;
	ss->javalocals = DMNEW(u8,count * 5);

#ifndef NDEBUG
	/* mark values as undefined */
	for (i=0; i<count*5; ++i)
		ss->javalocals[i] = (u8) 0x00dead0000dead00ULL;

	/* some entries in the intregs array are not meaningful */
	/*es->intregs[REG_ITMP3] = (u8) 0x11dead1111dead11ULL;*/
	es->intregs[REG_SP   ] = (u8) 0x11dead1111dead11ULL;
#ifdef REG_PV
	es->intregs[REG_PV   ] = (u8) 0x11dead1111dead11ULL;
#endif
#endif /* NDEBUG */
	
	ra = code->regalloc;

	i = -1;
	for (allocs = code->globalcount; allocs--; ra++) {
		if (ra->next)
			i++;
		t = ra->type;
		if (t == -1)
			continue; /* dummy rplalloc */

		replace_read_value(es,sp,ra,ss->javalocals + (5*i+t));
	}

	/* read stack slots */

	count = rp->regalloccount;
	ss->javastackdepth = count;
	ss->javastack = DMNEW(u8,count);

#ifndef NDEBUG
	/* mark values as undefined */
	for (i=0; i<count; ++i)
		ss->javastack[i] = (u8) 0x00dead0000dead00ULL;
#endif
	
	i = 0;
	ra = rp->regalloc;

	/* the first stack slot is special in SBR and EXH blocks */

	if (topslot == TOP_IS_ON_STACK) {
		assert(count);
		
		ss->javastack[i] = sp[-1];
		count--;
		i++;
		ra++;
	}
	else if (topslot == TOP_IS_IN_ITMP1) {
		assert(count);

		ss->javastack[i] = es->intregs[REG_ITMP1];
		count--;
		i++;
		ra++;
	}
	
	/* read remaining stack slots */
	
	for (; count--; ra++, i++) {
		assert(ra->next);

		replace_read_value(es,sp,ra,ss->javastack + i);
	}

	/* read unused callee saved int regs */
	
	count = INT_SAV_CNT;
	for (i=0; count > code->savedintcount; ++i) {
		assert(i < INT_REG_CNT);
		if (nregdescint[i] == REG_SAV)
			ss->savedintregs[--count] = es->intregs[i];
	}

	/* read saved int regs */

	for (i=0; i<code->savedintcount; ++i) {
		ss->savedintregs[i] = *--basesp;
	}

	/* read unused callee saved flt regs */
	
	count = FLT_SAV_CNT;
	for (i=0; count > code->savedfltcount; ++i) {
		assert(i < FLT_REG_CNT);
		if (nregdescfloat[i] == REG_SAV)
			ss->savedfltregs[--count] = es->fltregs[i];
	}

	/* read saved flt regs */

	for (i=0; i<code->savedfltcount; ++i) {
#ifdef HAS_4BYTE_STACKSLOT
		basesp -= 2;
#else
		basesp--;
#endif
		ss->savedfltregs[i] = *(u8*)basesp;
	}

	/* read slots used for synchronization */

	count = code_get_sync_slot_count(code);
	ss->syncslotcount = count;
	ss->syncslots = DMNEW(u8,count);
	for (i=0; i<count; ++i) {
		ss->syncslots[i] = sp[code->memuse + i];
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

static void replace_write_executionstate(rplpoint *rp,executionstate *es,
										 sourcestate *ss)
{
	methodinfo *m;
	codeinfo *code;
	int count;
	int i;
	int t;
	int allocs;
	rplalloc *ra;
	methoddesc *md;
	int topslot;
#ifdef HAS_4BYTE_STACKSLOT
	u4 *sp;
	u4 *basesp;
#else
	u8 *sp;
	u8 *basesp;
#endif

	code = rp->code;
	m = code->m;
	md = m->parseddesc;
	topslot = TOP_IS_NORMAL;
	
	/* calculate stack pointer */
	
#ifdef HAS_4BYTE_STACKSLOT
	basesp = (u4*) ss->stackbase;
#else
	basesp = (u8*) ss->stackbase;
#endif
	
	sp = basesp - code_get_stack_frame_size(code);

	/* on some architectures the returnAddress is passed on the stack by JSR */

#if defined(__I386__) || defined(__X86_64__)
	if (rp->type == BBTYPE_SBR) {
		topslot = TOP_IS_ON_STACK;
	}
#endif
	
	/* in some cases the top stack slot is passed in REG_ITMP1 */

	if (  (rp->type == BBTYPE_EXH)
#if defined(__ALPHA__) || defined(__POWERPC__) || defined(__MIPS__)
	   || (rp->type == BBTYPE_SBR) 
#endif
	   )
	{
		topslot = TOP_IS_IN_ITMP1;
	}

	/* in debug mode, invalidate stack frame first */

#ifndef NDEBUG
	for (i=0; i<(basesp - sp); ++i) {
		sp[i] = 0xdeaddeadU;
	}
#endif

	/* write local variables */

	count = m->maxlocals;

	ra = code->regalloc;

	i = -1;
	for (allocs = code->globalcount; allocs--; ra++) {
		if (ra->next)
			i++;

		assert(i >= 0 && i < m->maxlocals);
		
		t = ra->type;
		if (t == -1)
			continue; /* dummy rplalloc */

		replace_write_value(es,sp,ra,ss->javalocals + (5*i+t));
	}

	/* write stack slots */

	count = rp->regalloccount;

	i = 0;
	ra = rp->regalloc;

	/* the first stack slot is special in SBR and EXH blocks */

	if (topslot == TOP_IS_ON_STACK) {
		assert(count);
		
		sp[-1] = ss->javastack[i];
		count--;
		i++;
		ra++;
	}
	else if (topslot == TOP_IS_IN_ITMP1) {
		assert(count);

		es->intregs[REG_ITMP1] = ss->javastack[i];
		count--;
		i++;
		ra++;
	}

	/* write remaining stack slots */
	
	for (; count--; ra++, i++) {
		assert(ra->next);

		replace_write_value(es,sp,ra,ss->javastack + i);
	}
	
	/* write unused callee saved int regs */
	
	count = INT_SAV_CNT;
	for (i=0; count > code->savedintcount; ++i) {
		assert(i < INT_REG_CNT);
		if (nregdescint[i] == REG_SAV)
			es->intregs[i] = ss->savedintregs[--count];
	}

	/* write saved int regs */

	for (i=0; i<code->savedintcount; ++i) {
		*--basesp = ss->savedintregs[i];
	}

	/* write unused callee saved flt regs */
	
	count = FLT_SAV_CNT;
	for (i=0; count > code->savedfltcount; ++i) {
		assert(i < FLT_REG_CNT);
		if (nregdescfloat[i] == REG_SAV)
			es->fltregs[i] = ss->savedfltregs[--count];
	}

	/* write saved flt regs */

	for (i=0; i<code->savedfltcount; ++i) {
#ifdef HAS_4BYTE_STACKSLOT
		basesp -= 2;
#else
		basesp--;
#endif
		*(u8*)basesp = ss->savedfltregs[i];
	}

	/* write slots used for synchronization */

	count = code_get_sync_slot_count(code);
	assert(count == ss->syncslotcount);
	for (i=0; i<count; ++i) {
		sp[code->memuse + i] = ss->syncslots[i];
	}

	/* set new pc */

	es->pc = rp->pc;
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

void replace_me(rplpoint *rp,executionstate *es)
{
	rplpoint *target;
	sourcestate ss;
	s4 dumpsize;
	
	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* fetch the target of the replacement */

	target = rp->target;

	/* XXX DEBUG turn of self-replacement */
	if (target == rp)
		replace_deactivate_replacement_point(rp);
	
#ifndef NDEBUG
	printf("replace_me(%p,%p)\n",(void*)rp,(void*)es);
	fflush(stdout);
	replace_replacement_point_println(rp);
	replace_executionstate_println(es,rp->code);
#endif

	/* read execution state of old code */

	replace_read_executionstate(rp,es,&ss);

#ifndef NDEBUG
	replace_sourcestate_println(&ss);
#endif

	/* write execution state of new code */

	replace_write_executionstate(target,es,&ss);

#ifndef NDEBUG
	replace_executionstate_println(es,target->code);
#endif
	
	/* release dump area */

	dump_release(dumpsize);

	/* enter new code */

#if (defined(__I386__) || defined(__X86_64__) || defined(__ALPHA__) || defined(__POWERPC__) || defined(__MIPS__)) && defined(ENABLE_JIT)
	asm_replacement_in(es);
#endif
	abort();
}

/* replace_replacement_point_println *******************************************
 
   Print replacement point info.
  
   IN:
       rp...............the replacement point to print
  
*******************************************************************************/

#ifndef NDEBUG
static const char *type_char = "IJFDA";

#define TYPECHAR(t)  (((t) >= 0 && (t) <= 4) ? type_char[t] : '?')

void replace_replacement_point_println(rplpoint *rp)
{
	int j;

 	if (!rp) {
		printf("(rplpoint *)NULL\n");
		return;
	}

	printf("rplpoint %p pc:%p out:%p target:%p mcode:%016llx type:%01d flags:%01x ra:%d = [",
			(void*)rp,rp->pc,rp->outcode,(void*)rp->target,
			(unsigned long long)rp->mcode,rp->type,rp->flags,rp->regalloccount);

	for (j=0; j<rp->regalloccount; ++j)
		printf("%c%1c%01x:%02d",
				(rp->regalloc[j].next) ? '^' : ' ',
				TYPECHAR(rp->regalloc[j].type),
				rp->regalloc[j].flags,
				rp->regalloc[j].index);

	printf("]\n          method: ");
	method_print(rp->code->m);

	printf("\n");
}
#endif

/* replace_show_replacement_points *********************************************
 
   Print replacement point info.
  
   IN:
       code.............codeinfo whose replacement points should be printed.
  
*******************************************************************************/

#ifndef NDEBUG
void replace_show_replacement_points(codeinfo *code)
{
	int i;
	rplpoint *rp;
	
	if (!code) {
		printf("(codeinfo *)NULL\n");
		return;
	}

	printf("\treplacement points: %d\n",code->rplpointcount);
	printf("\tglobal allocations: %d = [",code->globalcount);

	for (i=0; i<code->globalcount; ++i)
		printf("%c%1c%01x:%02d",
				(code->regalloc[i].next) ? '^' : ' ',
				TYPECHAR(code->regalloc[i].type),
				code->regalloc[i].flags,code->regalloc[i].index);

	printf("]\n");

	printf("\ttotal allocations : %d\n",code->regalloccount);
	printf("\tsaved int regs    : %d\n",code->savedintcount);
	printf("\tsaved flt regs    : %d\n",code->savedfltcount);
	printf("\tmemuse            : %d\n",code->memuse);

	printf("\n");

	for (i=0; i<code->rplpointcount; ++i) {
		rp = code->rplpoints + i;

		assert(rp->code == code);
		
		printf("\t");
		replace_replacement_point_println(rp);
	}
}
#endif

/* replace_executionstate_println **********************************************
 
   Print execution state
  
   IN:
       es...............the execution state to print
	   code.............the codeinfo for which this execution state is meant
	                    (may be NULL)
  
*******************************************************************************/

#ifndef NDEBUG
void replace_executionstate_println(executionstate *es,codeinfo *code)
{
	int i;
	int slots;
#ifdef HAS_4BYTE_STACKSLOT
	u4 *sp;
#else
	u8 *sp;
#endif

 	if (!es) {
		printf("(executionstate *)NULL\n");
		return;
	}

	printf("executionstate %p:\n",(void*)es);
	printf("\tpc = %p\n",(void*)es->pc);
	printf("\tsp = %p\n",(void*)es->sp);
	printf("\tpv = %p\n",(void*)es->pv);
	for (i=0; i<INT_REG_CNT; ++i) {
		printf("\t%-3s = %016llx\n",regs[i],(unsigned long long)es->intregs[i]);
	}
	for (i=0; i<FLT_REG_CNT; ++i) {
		printf("\tfltregs[%2d] = %016llx\n",i,(unsigned long long)es->fltregs[i]);
	}

#ifdef HAS_4BYTE_STACKSLOT
	sp = (u4*) es->sp;
#else
	sp = (u8*) es->sp;
#endif

	if (code)
		slots = code_get_stack_frame_size(code);
	else
		slots = 0;

	printf("\tstack slots at sp:\n");
	for (i=0; i<slots; ++i) {
#ifdef HAS_4BYTE_STACKSLOT
		printf("\t\t%08lx\n",(unsigned long)*sp++);
#else
		printf("\t\t%016llx\n",(unsigned long long)*sp++);
#endif
	}

	printf("\n");
}
#endif

/* replace_sourcestate_println *************************************************
 
   Print source state
  
   IN:
       ss...............the source state to print
  
*******************************************************************************/

#ifndef NDEBUG
void replace_sourcestate_println(sourcestate *ss)
{
	int i;
	int t;
	int reg;

 	if (!ss) {
		printf("(sourcestate *)NULL\n");
		return;
	}

	printf("sourcestate %p: stackbase=%p\n",(void*)ss,(void*)ss->stackbase);

	printf("\tlocals (%d):\n",ss->javalocalcount);
	for (i=0; i<ss->javalocalcount; ++i) {
		for (t=0; t<5; ++t) {
			if (ss->javalocals[i*5+t] != 0x00dead0000dead00ULL) {
				printf("\tlocal[%c%2d] = ",TYPECHAR(t),i);
				printf("%016llx\n",(unsigned long long) ss->javalocals[i*5+t]);
			}
		}
	}

	printf("\n");

	printf("\tstack (depth %d):\n",ss->javastackdepth);
	for (i=0; i<ss->javastackdepth; ++i) {
		printf("\tstack[%2d] = ",i);
		printf("%016llx\n",(unsigned long long) ss->javastack[i]);
	}

	printf("\n");

	printf("\tsaved int registers (%d):\n",INT_SAV_CNT);
	reg = INT_REG_CNT;
	for (i=0; i<INT_SAV_CNT; ++i) {
		while (nregdescint[--reg] != REG_SAV)
			;
		if (ss->savedintregs[i] != 0x00dead0000dead00ULL) {
			printf("\t%-3s = ",regs[reg]);
			printf("%016llx\n",(unsigned long long) ss->savedintregs[i]);
		}
	}

	printf("\n");

	printf("\tsaved float registers (%d):\n",FLT_SAV_CNT);
	for (i=0; i<FLT_SAV_CNT; ++i) {
		if (ss->savedfltregs[i] != 0x00dead0000dead00ULL) {
			printf("\tsavedfltreg[%2d] = ",i);
			printf("%016llx\n",(unsigned long long) ss->savedfltregs[i]);
		}
	}
	
	printf("\n");

	printf("\tsynchronization slots (%d):\n",ss->syncslotcount);
	for (i=0; i<ss->syncslotcount; ++i) {
		printf("\tslot[%2d] = ",i);
#ifdef HAS_4BYTE_STACKSLOT
		printf("%08lx\n",(unsigned long) ss->syncslots[i]);
#else
		printf("%016llx\n",(unsigned long long) ss->syncslots[i]);
#endif
	}
	
	printf("\n");
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
