/* vm/jit/schedule/schedule.c - architecture independent instruction scheduler

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Christian Thalinger

   Changes:

   $Id: schedule.c 1966 2005-02-24 23:39:12Z twisti $

*/


#include <stdio.h>

#include "disass.h"

#include "mm/memory.h"
#include "vm/jit/schedule/schedule.h"


scheduledata *schedule_init(registerdata *rd)
{
	scheduledata *sd;

	sd = DNEW(scheduledata);

	/* XXX quick hack: just use a fix number of instructions */
	sd->mi = DMNEW(minstruction, 100);

	sd->intregs_read_dep = DMNEW(minstruction*, rd->intregsnum);
	sd->intregs_write_dep = DMNEW(minstruction*, rd->intregsnum);

	sd->fltregs_read_dep = DMNEW(minstruction*, rd->fltregsnum);
	sd->fltregs_write_dep = DMNEW(minstruction*, rd->fltregsnum);

	/* XXX: memory is currently only one cell */
/*  	sd->memory_write_dep; */

	/* clear all pointers */

	MSET(sd->intregs_read_dep, 0, minstruction*, rd->intregsnum);
	MSET(sd->intregs_write_dep, 0, minstruction*, rd->intregsnum);

	MSET(sd->fltregs_read_dep, 0, minstruction*, rd->fltregsnum);
	MSET(sd->fltregs_write_dep, 0, minstruction*, rd->fltregsnum);

  	sd->memory_write_dep = NULL;

	return sd;
}


minstruction *schedule_prepend_minstruction(minstruction *mi)
{
	minstruction *tmpmi;

	/* add new instruction in front of the list */

	tmpmi = DNEW(minstruction);

	tmpmi->latency = 0;
	tmpmi->priority = 0;
	tmpmi->opdep[0] = NULL;
	tmpmi->opdep[1] = NULL;
	tmpmi->opdep[2] = NULL;

	tmpmi->next = mi;                   /* link to next instruction           */

	return tmpmi;
}


/* schedule_calc_priority ******************************************************

   Calculates the current node's priority by getting highest priority
   of the dependency nodes, adding this nodes latency plus 1 (for the
   instruction itself).

*******************************************************************************/

void schedule_calc_priority(minstruction *mi)
{
	s4 i;
	s4 pathpriority;

	/* check all dependencies for their priority */

	pathpriority = 0;

	for (i = 0; i < 3; i++) {
		if (mi->opdep[i]) {
			if (mi->opdep[i]->priority > pathpriority)
				pathpriority = mi->opdep[i]->priority;
		}
	}

	mi->priority = pathpriority + mi->latency + 1;
}


/* schedule_do_schedule ********************************************************

   XXX

*******************************************************************************/

void schedule_do_schedule(minstruction *mi)
{
	minstruction *rootmi;

	/* initialize variables */

	rootmi = mi;

	printf("bb start ---\n");

	mi = rootmi;

	while (mi) {
		printf("%p: ", mi);

/*  		disassinstr(&tmpmi->instr); */
		printf("%05x", mi->instr);

		printf("   --> %d, %d:   op1=%p, op2=%p, op3=%p\n", mi->latency, mi->priority, mi->opdep[0], mi->opdep[1], mi->opdep[2]);

		mi = mi->next;
	}
	printf("bb end ---\n\n");
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
 */
