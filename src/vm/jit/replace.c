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

#include "mm/memory.h"
#include "vm/options.h"
#include "vm/jit/replace.h"

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

bool replace_create_replacement_points(codeinfo *code,registerdata *rd)
{
	basicblock *bptr;
	int count;
	methodinfo *m;
	rplpoint *rplpoints;
	rplpoint *rp;
	int alloccount;
	int globalcount;
	s2 *regalloc;
	s2 *ra;
	int i;
	stackptr sp;

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

	globalcount = m->maxlocals;
	alloccount += globalcount;

	/* allocate replacement point array and allocation array */
	
	rplpoints = MNEW(rplpoint,count);
	regalloc = MNEW(s2,alloccount);
	ra = regalloc;

	/* store global register allocations */

	for (i=0; i<m->maxlocals; ++i) {
#if defined(ENABLE_INTRP)
		if (!opt_intrp) {
#endif
		*ra++ = rd->locals[i][0].regoff; /* XXX */
#if defined(ENABLE_INTRP)
		}
		else {
			*ra++ = 0;
		}
#endif
	}

	/* initialize replacement point structs */

	rp = rplpoints;
	for (bptr = m->basicblocks; bptr; bptr = bptr->next) {
		if (!(bptr->bitflags & BBFLAG_REPLACEMENT))
			continue;

		/* there will be a replacement point at the start of this block */

		rp->pc = NULL;        /* set by codegen */
		rp->outcode = NULL;   /* set by codegen */
		rp->hashlink = NULL;
		rp->code = code;
		rp->target = NULL;
		rp->regalloc = ra;

		/* store local allocation info */

		for (sp = bptr->instack; sp; sp = sp->prev) {
			*ra++ = sp->regoff; /* XXX */
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
		MFREE(code->regalloc,s2,code->regalloccount);

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
	
#if defined(__I386__) && defined(ENABLE_JIT)
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
	
#if defined(__I386__) && defined(ENABLE_JIT)
	md_patch_replacement_point(rp);
#endif
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
	
	target = rp->target;
	
#ifndef NDEBUG
	printf("replace_me(%p,%p)\n",(void*)rp,(void*)es);
	fflush(stdout);
	replace_replacement_point_println(rp);
	replace_executionstate_println(es);
#endif

	es->pc = rp->target->pc;
}

/* replace_replacement_point_println *******************************************
 
   Print replacement point info.
  
   IN:
       rp...............the replacement point to print
  
*******************************************************************************/

#ifndef NDEBUG
void replace_replacement_point_println(rplpoint *rp)
{
	int j;

 	if (!rp) {
		printf("(rplpoint *)NULL\n");
		return;
	}

	printf("rplpoint %p pc:%p out:%p target:%p mcode:%016llx allocs:%d = [",
			(void*)rp,rp->pc,rp->outcode,(void*)rp->target,
			rp->mcode,rp->regalloccount);

	for (j=0; j<rp->regalloccount; ++j)
		printf(" %02d",rp->regalloc[j]);

	printf("] method:");
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
	printf("\tglobal allocations: %d\n",code->globalcount);
	printf("\ttotal allocations : %d\n",code->regalloccount);
	printf("\n");

	for (i=0; i<code->rplpointcount; ++i) {
		rp = code->rplpoints + i;

		assert(rp->code == code);
		
		replace_replacement_point_println(rp);
	}
}
#endif

/* replace_executionstate_println **********************************************
 
   Print execution state
  
   IN:
       es...............the execution state to print
  
*******************************************************************************/

#ifndef NDEBUG
void replace_executionstate_println(executionstate *es)
{
	int i;

 	if (!es) {
		printf("(executionstate *)NULL\n");
		return;
	}

	printf("executionstate %p:\n",(void*)es);
	printf("\tpc = %p\n",(void*)es->pc);
	for (i=0; i<3; ++i) {
		printf("\tregs[%2d] = %016llx\n",i,(u8)es->regs[i]);
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
