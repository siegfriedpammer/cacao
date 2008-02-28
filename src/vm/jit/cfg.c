/* src/vm/jit/cfg.c - build a control-flow graph

   Copyright (C) 2006, 2007 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
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

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "mm/memory.h"

#include "vm/global.h"

#include "vm/jit/jit.h"
#include "vm/jit/stack.h"


/* cfg_allocate_predecessors ***************************************************

   Allocates the predecessor array, if there is none, and resets the
   predecessor count.

*******************************************************************************/

static void cfg_allocate_predecessors(basicblock *bptr)
{
	if (bptr->predecessors == NULL) {
		bptr->predecessors = DMNEW(basicblock*, bptr->predecessorcount);

		bptr->predecessorcount = 0;
	}
}


/* cfg_allocate_successors *****************************************************

   Allocates the succecessor array, if there is none, and resets the
   predecessor count.

*******************************************************************************/

static void cfg_allocate_successors(basicblock *bptr)
{
	if (bptr->successors == NULL) {
		bptr->successors = DMNEW(basicblock*, bptr->successorcount);

		bptr->successorcount = 0;
	}
}


/* cfg_insert_predecessor ******************************************************

   Inserts a predecessor into the array, but checks for duplicate
   entries.  This is used for TABLESWITCH and LOOKUPSWITCH.

*******************************************************************************/

static void cfg_insert_predecessors(basicblock *bptr, basicblock *pbptr)
{
	basicblock **tbptr;
	int          i;

	tbptr = bptr->predecessors;

	/* check if the predecessors is already stored in the array */

	for (i = 0; i < bptr->predecessorcount; i++, tbptr++)
		if (*tbptr == pbptr)
			return;

	/* not found, insert it */

	bptr->predecessors[bptr->predecessorcount] = pbptr;
	bptr->predecessorcount++;
}

static void cfg_insert_successors(basicblock *bptr, basicblock *pbptr)
{
	basicblock **tbptr;
	int          i;

	tbptr = bptr->successors;

	/* check if the successor is already stored in the array */

	for (i = 0; i < bptr->successorcount; i++, tbptr++)
		if (*tbptr == pbptr)
			return;

	/* not found, insert it */

	bptr->successors[bptr->successorcount] = pbptr;
	bptr->successorcount++;
}


/* cfg_build *******************************************************************

   Build a control-flow graph in finding all predecessors and
   successors for the basic blocks.

*******************************************************************************/

bool cfg_build(jitdata *jd)
{
	basicblock      *bptr;
	basicblock      *tbptr;
	basicblock      *ntbptr;
	instruction     *iptr;
	branch_target_t *table;
	lookup_target_t *lookup;
	int              i;

	/* process all basic blocks to find the predecessor/successor counts */

	bptr = jd->basicblocks;

	for (bptr = jd->basicblocks; bptr != NULL; bptr = bptr->next) {
		if ((bptr->icount == 0) || (bptr->flags == BBUNDEF))
			continue;

		iptr = bptr->iinstr + bptr->icount - 1;

		/* skip NOPs at the end of the block */

		while (iptr->opc == ICMD_NOP) {
			if (iptr == bptr->iinstr)
				break;
			iptr--;
		}

		switch (icmd_table[iptr->opc].controlflow) {

		case CF_END:
			break;

		case CF_IF:

			bptr->successorcount += 2;

			tbptr  = iptr->dst.block;
			ntbptr = bptr->next;

			tbptr->predecessorcount++;
			ntbptr->predecessorcount++;
			break;

		case CF_JSR:
			bptr->successorcount++;

			tbptr = iptr->sx.s23.s3.jsrtarget.block;
			tbptr->predecessorcount++;
			break;

		case CF_GOTO:
		case CF_RET:
			bptr->successorcount++;

			tbptr = iptr->dst.block;
			tbptr->predecessorcount++;
			break;

		case CF_TABLE:
			table = iptr->dst.table;

			bptr->successorcount++;

			tbptr = table->block;
			tbptr->predecessorcount++;
			table++;

			i = iptr->sx.s23.s3.tablehigh - iptr->sx.s23.s2.tablelow + 1;

			while (--i >= 0) {
				bptr->successorcount++;

				tbptr = table->block;
				tbptr->predecessorcount++;
				table++;
			}
			break;
					
		case CF_LOOKUP:
			lookup = iptr->dst.lookup;

			bptr->successorcount++;

			tbptr = iptr->sx.s23.s3.lookupdefault.block;
			tbptr->predecessorcount++;

			i = iptr->sx.s23.s2.lookupcount;

			while (--i >= 0) {
				bptr->successorcount++;

				tbptr = lookup->target.block;
				tbptr->predecessorcount++;
				lookup++;
			}
			break;

		default:
			bptr->successorcount++;

			tbptr = bptr->next;

			/* An exception handler has no predecessors. */

			if (tbptr->type != BBTYPE_EXH)
				tbptr->predecessorcount++;
			break;
		}
	}

	/* Second iteration to allocate the arrays and insert the basic
	   block pointers. */

	bptr = jd->basicblocks;

	for (bptr = jd->basicblocks; bptr != NULL; bptr = bptr->next) {
		if ((bptr->icount == 0) || (bptr->flags == BBUNDEF))
			continue;

		iptr = bptr->iinstr + bptr->icount - 1;

		/* skip NOPs at the end of the block */

		while (iptr->opc == ICMD_NOP) {
			if (iptr == bptr->iinstr)
				break;
			iptr--;
		}

		switch (icmd_table[iptr->opc].controlflow) {

		case CF_END:
			break;

		case CF_IF:

			tbptr  = iptr->dst.block;
			ntbptr = bptr->next;

			cfg_allocate_successors(bptr);

			cfg_insert_successors(bptr, tbptr);
			cfg_insert_successors(bptr, ntbptr);

			cfg_allocate_predecessors(tbptr);
			cfg_allocate_predecessors(ntbptr);

			cfg_insert_predecessors(tbptr, bptr);
			cfg_insert_predecessors(ntbptr, bptr);
			break;

		case CF_JSR:
			tbptr = iptr->sx.s23.s3.jsrtarget.block;
			goto goto_tail;

		case CF_GOTO:
		case CF_RET:

			tbptr = iptr->dst.block;
goto_tail:
			cfg_allocate_successors(bptr);

			cfg_insert_successors(bptr, tbptr);

			cfg_allocate_predecessors(tbptr);

			cfg_insert_predecessors(tbptr, bptr);
			break;

		case CF_TABLE:
			table = iptr->dst.table;

			tbptr = table->block;
			table++;

			cfg_allocate_successors(bptr);

			cfg_insert_successors(bptr, tbptr);

			cfg_allocate_predecessors(tbptr);

			cfg_insert_predecessors(tbptr, bptr);

			i = iptr->sx.s23.s3.tablehigh - iptr->sx.s23.s2.tablelow + 1;

			while (--i >= 0) {
				tbptr = table->block;
				table++;

				cfg_insert_successors(bptr, tbptr);

				cfg_allocate_predecessors(tbptr);
				cfg_insert_predecessors(tbptr, bptr);
			}
			break;
					
		case CF_LOOKUP:
			lookup = iptr->dst.lookup;

			tbptr = iptr->sx.s23.s3.lookupdefault.block;

			cfg_allocate_successors(bptr);

			cfg_insert_successors(bptr, tbptr);

			cfg_allocate_predecessors(tbptr);

			cfg_insert_predecessors(tbptr, bptr);

			i = iptr->sx.s23.s2.lookupcount;

			while (--i >= 0) {
				tbptr = lookup->target.block;
				lookup++;

				cfg_insert_successors(bptr, tbptr);

				cfg_allocate_predecessors(tbptr);
				cfg_insert_predecessors(tbptr, bptr);
			}
			break;

		default:
			tbptr = bptr->next;

			cfg_allocate_successors(bptr);

			bptr->successors[0] = tbptr;
			bptr->successorcount++;

			/* An exception handler has no predecessors. */

			if (tbptr->type != BBTYPE_EXH) {
				cfg_allocate_predecessors(tbptr);

				tbptr->predecessors[tbptr->predecessorcount] = bptr;
				tbptr->predecessorcount++;
			}
			break;
		}
	}

	/* everything's ok */

	return true;
}

/* cfg_add_root ****************************************************************

   Adds an empty root basicblock.
   The numbers of all other basicblocks are set off by one.
   Needed for some analyses that require the root basicblock to have no 
   predecessors and to perform special initializations.

*******************************************************************************/

void cfg_add_root(jitdata *jd) {
	basicblock *root, *zero, *it;

	zero = jd->basicblocks;

	root = DNEW(basicblock);
	MZERO(root, basicblock, 1);

	root->successorcount = 1;
	root->successors = DMNEW(basicblock *, 1);
	root->successors[0] = zero;
	root->next = zero;
	root->nr = 0;
	root->type = BBTYPE_STD;

	if (zero->predecessorcount == 0) {
		zero->predecessors = DNEW(basicblock *);
	} else {
		zero->predecessors = DMREALLOC(zero->predecessors, basicblock *, zero->predecessorcount, zero->predecessorcount + 1);
	}
	zero->predecessors[zero->predecessorcount] = root;
	zero->predecessorcount += 1;

	jd->basicblocks = root;
	jd->basicblockcount += 1;

	for (it = zero; it; it = it->next) {
		it->nr += 1;
	}
}

/* cfg_add_exceptional_edges ***************************************************
 
   Edges from basicblocks to their exception handlers and from exception 
   handlers to the blocks they handle exceptions for are added. Further
   the number of potentially throwing instructions in the basicblocks are 
   counted.

   We don't consider nor do we determine the types of exceptions thrown. Edges
   are added from every block to every potential handler.

*******************************************************************************/

void cfg_add_exceptional_edges(jitdata *jd) {
	basicblock *bptr;
	instruction *iptr;
	exception_entry *ee;

	/* Count the number of exceptional exits for every block.
	 * Every PEI is an exceptional out.
	 */

	FOR_EACH_BASICBLOCK(jd, bptr) {

		if (bptr->flags == BBUNDEF) {
			continue;
		}

		FOR_EACH_INSTRUCTION(bptr, iptr) {
			if (icmd_table[iptr->opc].flags & ICMDTABLE_PEI) {	
				bptr->exouts += 1;
			}
		}
	}

	/* Count the number of exception handlers for every block. */

	for (ee = jd->exceptiontable; ee; ee = ee->down) {
		for (bptr = ee->start; bptr != ee->end; bptr = bptr->next) {
			/* Linking a block with a handler, even if there are no exceptional exits
			   breaks stuff in other passes. */
			if (bptr->exouts > 0) {
				bptr->exhandlercount += 1;
				ee->handler->expredecessorcount += 1;
			}
		}
	}

	/* Allocate and fill exception handler arrays. */

	for (ee = jd->exceptiontable; ee; ee = ee->down) {
		for (bptr = ee->start; bptr != ee->end; bptr = bptr->next) {
			if (bptr->exouts > 0) {

				if (bptr->exhandlers == NULL) {
					bptr->exhandlers = DMNEW(basicblock *, bptr->exhandlercount);
					/* Move pointer past the end of the array, 
					 * It will be filled in the reverse order.
					 */
					bptr->exhandlers += bptr->exhandlercount;
				}

				bptr->exhandlers -= 1;
				*(bptr->exhandlers) = ee->handler;

				if (ee->handler->expredecessors == NULL) {
					ee->handler->expredecessors = DMNEW(basicblock *, ee->handler->expredecessorcount);
					ee->handler->expredecessors += ee->handler->expredecessorcount;
				}

				ee->handler->expredecessors -= 1;
				*(ee->handler->expredecessors) = bptr;
			}
		}
	}
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
