/* src/vm/jit/schedule/schedule.c - architecture independent instruction
                                    scheduler

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

   $Id: schedule.c 1985 2005-03-04 17:09:13Z twisti $

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

	sd->intregs_define_dep = DMNEW(s4, rd->intregsnum);
	sd->fltregs_define_dep = DMNEW(s4, rd->fltregsnum);

	sd->intregs_use_dep = DMNEW(nodelink*, rd->intregsnum);
	sd->fltregs_use_dep = DMNEW(nodelink*, rd->fltregsnum);

  	sd->memory_define_dep = NULL;
  	sd->memory_use_dep = NULL;

	return sd;
}


void schedule_setup(scheduledata *sd, registerdata *rd)
{
	sd->micount = 0;
	sd->leaders = NULL;

	/* set define array to -1, because 0 is a valid node number */

	MSET(sd->intregs_define_dep, -1, s4, rd->intregsnum);
	MSET(sd->fltregs_define_dep, -1, s4, rd->fltregsnum);

	/* clear all use pointers */

	MSET(sd->intregs_use_dep, 0, nodelink*, rd->intregsnum);
	MSET(sd->fltregs_use_dep, 0, nodelink*, rd->fltregsnum);

  	sd->memory_define_dep = NULL;
  	sd->memory_use_dep = NULL;
}


void schedule_add_define_dep(scheduledata *sd, s4 *define_dep, nodelink **use_dep)
{
	minstruction *mi;
	minstruction *defmi;
	minstruction *usemi;
	nodelink     *usenl;
	nodelink     *tmpnl;
	nodelink     *nl;
	s4            defnode;
	s4            minode;

	/* get current machine instruction */

	minode = sd->micount - 1;
	mi = &sd->mi[minode];

	/* get current use dependency nodes, if non-null use them... */

	if ((usenl = *use_dep)) {
		/* add a dependency link node to all use dependency nodes */

		while (usenl) {
			/* Save next pointer so we can link the current node to the       */
			/* machine instructions' dependency list.                         */

			tmpnl = usenl->next;

			/* don't add the current machine instruction */

			if (usenl->minode != minode) {
				/* some edges to current machine instruction -> no leader */

				mi->flags &= ~SCHEDULE_LEADER;

				/* get use machine instruction */

				usemi = &sd->mi[usenl->minode];

				/* link current use node into dependency list */

				usenl->minode = minode;
				usenl->next = usemi->deps;
				usemi->deps = usenl;
			}

			usenl = tmpnl;
		}

	} else {
		/* ...otherwise use last define dependency, if non-null */

		if ((defnode = *define_dep) >= 0) {
			/* some edges to current machine instruction -> no leader */

			mi->flags &= ~SCHEDULE_LEADER;

			/* get last define machine instruction */

			defmi = &sd->mi[defnode];

			nl = DNEW(nodelink);
			nl->minode = minode;
			nl->next = defmi->deps;
			defmi->deps = nl;
		}
	}

	/* Set current instruction as new define dependency and clear use         */
	/* depedencies.                                                           */

	*define_dep = minode;
	*use_dep = NULL;
}


void schedule_add_use_dep(scheduledata *sd, s4 *define_dep, nodelink **use_dep)
{
	minstruction *mi;
	minstruction *defmi;
	nodelink     *nl;
	s4            minode;
	s4            defnode;

	/* get current machine instruction */

	minode = sd->micount - 1;
	mi = &sd->mi[minode];

	/* get current define dependency instruction */

	if ((defnode = *define_dep) >= 0) {
		/* some edges to current machine instruction -> no leader */

		mi->flags &= ~SCHEDULE_LEADER;

		/* add node to dependency list of current define node */

		defmi = &sd->mi[defnode];

		nl = DNEW(nodelink);
		nl->minode = minode;
		nl->next = defmi->deps;
		defmi->deps = nl;
	}

	/* add node to list of current use nodes */

	nl = DNEW(nodelink);
	nl->minode = minode;
	nl->next = *use_dep;
	*use_dep = nl;
}


void schedule_add_memory_define_dep(scheduledata *sd)
{
}


void schedule_add_memory_use_dep(scheduledata *sd)
{
}


/* schedule_calc_priorities ****************************************************

   Calculates the current node's priority by getting highest priority
   of the dependency nodes, adding this nodes latency plus 1 (for the
   instruction itself).

*******************************************************************************/

void schedule_calc_priorities(scheduledata *sd)
{
	minstruction *mi;
	nodelink     *nl;
	s4            i;
	u1            startcycle;
	u1            endcycle;
	s1            priority;
	s4            criticalpath;
	s4            currentpath;

	/* walk through all machine instructions backwards */

	for (i = sd->micount - 1, mi = &sd->mi[sd->micount - 1]; i >= 0; i--, mi--) {
		nl = mi->deps;

		/* do we have some dependencies */

		if (nl) {
			endcycle = mi->endcycle;
			criticalpath = 0;

			/* walk through depedencies and calculate highest latency */

			while (nl) {
				startcycle = sd->mi[nl->minode].startcycle;
				priority = sd->mi[nl->minode].priority;

				currentpath = (endcycle - startcycle) + priority;

				if (currentpath > criticalpath)
					criticalpath = currentpath;

				nl = nl->next;
			}

			mi->priority = criticalpath;

		} else {
			/* no dependencies, use virtual sink node */

			mi->priority = mi->endcycle - mi->startcycle;
		}

		/* add leader to leader list */

		if (mi->flags & SCHEDULE_LEADER) {
			nl = DNEW(nodelink);
			nl->minode = i;
			nl->next = sd->leaders;
			sd->leaders = nl;
		}
	}
}


static void schedule_create_graph(scheduledata *sd)
{
	minstruction *mi;
	nodelink     *nl;
	s4            i;

	FILE *file;

	file = fopen("cacao.dot", "w+");

	fprintf(file, "digraph G {\n");

	for (i = 0, mi = sd->mi; i < sd->micount; i++, mi++) {
		nl = mi->deps;

		if (nl) {
			while (nl) {
				s4 priority = mi->priority - sd->mi[nl->minode].priority;

				fprintf(file, "%d -> %d [ label = \"%d\" ]\n", i, nl->minode, priority);
				nl = nl->next;
			}

		} else {
			fprintf(file, "%d\n", i);
		}
	}

	fprintf(file, "}\n");
	fclose(file);
}


/* schedule_do_schedule ********************************************************

   XXX

*******************************************************************************/

void schedule_do_schedule(scheduledata *sd)
{
	minstruction *mi;
	nodelink     *nl;
	s4            i;
	s4            criticalpath;

	/* calculate priorities and critical path */

	schedule_calc_priorities(sd);

	printf("bb start ---\n");

	printf("nodes: %d\n", sd->micount);

	printf("leaders: ");
	criticalpath = 0;
	nl = sd->leaders;
	while (nl) {
		printf("%d ", nl->minode);
		if (sd->mi[nl->minode].priority > criticalpath)
			criticalpath = sd->mi[nl->minode].priority;
		nl = nl->next;
	}
	printf("\n");

	printf("critical path: %d\n", criticalpath);

	for (i = 0, mi = sd->mi; i < sd->micount; i++, mi++) {
		disassinstr(&mi->instr);

		printf("\t--> #%d, start=%d, end=%d, prio=%d", i, mi->startcycle, mi->endcycle, mi->priority);

		printf(", deps= ");
		nl = mi->deps;
		while (nl) {
			printf("%d ", nl->minode);
			nl = nl->next;
		}

		printf("\n");
	}
	printf("bb end ---\n\n");

/*  	schedule_create_graph(sd); */
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
