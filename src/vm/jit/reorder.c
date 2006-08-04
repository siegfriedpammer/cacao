/* src/vm/reorder.c - basic block reordering

   Copyright (C) 2006 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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

   Contact: cacao@cacaojvm.org

   Authors: Christian Thalinger

   Changes:

   $Id: cacao.c 4357 2006-01-22 23:33:38Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "vm/jit/jit.h"


/* reorder *********************************************************************

   Reorders basic blocks in respect to their execution frequency.

*******************************************************************************/

bool reorder(jitdata *jd)
{
	methodinfo  *m;
	codeinfo    *pcode;
	basicblock  *bptr;
	basicblock  *tbptr;                 /* taken basic block pointer          */
	basicblock  *ntbptr;                /* not-taken basic block pointer      */
	u4           freq;
	u4           tfreq;
	u4           ntfreq;
	instruction *iptr;
	u1          *blocks;                /* flag array                         */
	s4           placed;                /* number of placed basic blocks      */

	/* get required compiler data */

	m = jd->m;
	method_println(m);

	/* get the codeinfo of the previous method */

	pcode = m->code;

	/* allocate flag array for blocks which are placed */

	blocks = DMNEW(u1, m->basicblockcount);

	MZERO(blocks, u1, m->basicblockcount);

	/* Get the entry block and iterate over all basic blocks until we
	   have placed all blocks. */

	bptr   = m->basicblocks;
	placed = 0;

	while (placed < m->basicblockcount) {
		/* get last instruction of basic block */

		iptr = bptr->iinstr + bptr->icount - 1;

		printf("L%03d, ", bptr->debug_nr);

		switch (bptr->type) {
		case BBTYPE_STD:
			printf("STD");
			break;
		case BBTYPE_EXH:
			printf("EXH");
			break;
		case BBTYPE_SBR:
			printf("SBR");
			break;
		}

		printf(", predecessors: %d, frequency: %d -> ", bptr->predecessorcount,
			   pcode->bbfrequency[bptr->debug_nr]);

		switch (iptr->opc) {
		case ICMD_RETURN:
		case ICMD_IRETURN:
		case ICMD_LRETURN:
		case ICMD_FRETURN:
		case ICMD_DRETURN:
		case ICMD_ARETURN:
		case ICMD_ATHROW:
			puts("return");
			break;

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

		case ICMD_IFNULL:
		case ICMD_IFNONNULL:

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
			tbptr  = (basicblock *) iptr->target;
			ntbptr = bptr->next;

			printf("cond. L%03d\n", tbptr->debug_nr);

			if ((tbptr != NULL) && (ntbptr != NULL)) {
				tfreq  = pcode->bbfrequency[tbptr->debug_nr];
				ntfreq = pcode->bbfrequency[ntbptr->debug_nr];

				/* check which branch is more frequently */

				if (tfreq > ntfreq) {
					/* place taken block */

					blocks[tbptr->debug_nr] = true;

					/* set last placed block */

					bptr = tbptr;
				}
				else {
					/* place not-taken block */

					blocks[ntbptr->debug_nr] = true;

					/* set last placed block */

					bptr = ntbptr;
				}
			}

			placed++;
			break;

		case ICMD_GOTO:
			tbptr  = (basicblock *) iptr->target;
			ntbptr = NULL;

			printf("L%03d\n", tbptr->debug_nr);

			/* Check if the target block has other predecessors.  And
			   if the other predecessors have a higher frequency,
			   don't place it. */

			if (tbptr->predecessorcount > 1) {
				freq  = pcode->bbfrequency[bptr->debug_nr];
				tfreq = pcode->bbfrequency[tbptr->debug_nr];

				/* XXX check */

				/* place block */

				blocks[tbptr->debug_nr] = true;
				placed++;
			}

			/* set last placed block */

			bptr = tbptr;
			break;

		default:
			/* No control flow instruction at the end of the basic
			   block (fall-through).  Just place the block. */

			tbptr = bptr->next;

			puts("next");

			/* place block */

			blocks[tbptr->debug_nr] = true;
			placed++;

			/* set last placed block */

			bptr = tbptr;
			break;
		}
	}

	puts("");

	/* everything's ok */

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
