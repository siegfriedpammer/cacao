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
		*ra++ = rd->locals[i][0].regoff; /* XXX */
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

/* replace_show_replacement_points *********************************************
 
   Print replacement point info.
  
   IN:
       code.............codeinfo whose replacement points should be freed.
  
*******************************************************************************/

#ifndef NDEBUG
void replace_show_replacement_points(codeinfo *code)
{
	int i;
	int j;
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
		
		printf("\trplpoint %p pc:%p out:%p target:%p allocs:%d =",
				(void*)rp,rp->pc,rp->outcode,(void*)rp->target,
				rp->regalloccount);

		for (j=0; j<rp->regalloccount; ++j)
			printf(" %02d",rp->regalloc[j]);

		printf("\n");
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
