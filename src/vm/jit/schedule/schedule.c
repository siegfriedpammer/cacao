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

   $Id: schedule.c 2025 2005-03-10 12:20:49Z twisti $

*/


#include <stdio.h>

#include "config.h"
#include "disass.h"

#include "mm/memory.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/jit/schedule/schedule.h"


#define DEBUGPRINT 1

scheduledata *schedule_init(registerdata *rd)
{
	scheduledata *sd;

	sd = DNEW(scheduledata);

	/* XXX quick hack: just use a fixed number of instructions */
	sd->mi = DMNEW(minstruction, 20000);

	sd->intregs_define_dep = DMNEW(nodelink*, rd->intregsnum);
	sd->fltregs_define_dep = DMNEW(nodelink*, rd->fltregsnum);

	sd->intregs_use_dep = DMNEW(nodelink*, rd->intregsnum);
	sd->fltregs_use_dep = DMNEW(nodelink*, rd->fltregsnum);

  	sd->memory_define_dep = NULL;
  	sd->memory_use_dep = NULL;


#if DEBUGPRINT
	/* open graphviz file */

	{
		FILE *file;

		file = fopen("cacao.dot", "w+");
		fprintf(file, "digraph G {\n");

		sd->file = file;
	}
#endif

	return sd;
}


void schedule_reset(scheduledata *sd, registerdata *rd)
{
	sd->micount = 0;
	sd->leaders = NULL;

	/* set define array to -1, because 0 is a valid node number */

	MSET(sd->intregs_define_dep, 0, nodelink*, rd->intregsnum);
	MSET(sd->fltregs_define_dep, 0, nodelink*, rd->fltregsnum);

	/* clear all use pointers */

	MSET(sd->intregs_use_dep, 0, nodelink*, rd->intregsnum);
	MSET(sd->fltregs_use_dep, 0, nodelink*, rd->fltregsnum);

  	sd->memory_define_dep = NULL;
  	sd->memory_use_dep = NULL;
}


void schedule_close(scheduledata *sd)
{
#if DEBUGPRINT
	FILE *file;

	file = sd->file;

	fprintf(file, "}\n");
	fclose(file);
#endif
}



/* schedule_add_define_dep *****************************************************

   XXX

*******************************************************************************/

void schedule_add_define_dep(scheduledata *sd, s1 opnum, nodelink **define_dep, nodelink **use_dep)
{
	minstruction *mi;
	minstruction *defmi;
	minstruction *usemi;
	nodelink     *usenl;
	nodelink     *defnl;
	nodelink     *tmpnl;
	nodelink     *nl;
	s4            defnode;
	s4            minode;

	/* get current machine instruction */

	minode = sd->micount - 1;
	mi = &sd->mi[minode];

	/* get current use dependency nodes, if non-null use them... */

	if ((usenl = *use_dep)) {
		while (usenl) {
			/* Save next pointer so we can link the current node to the       */
			/* machine instructions' dependency list.                         */

			tmpnl = usenl->next;

			/* don't add the current machine instruction (e.g. add A0,A0,A0) */

			if (usenl->minode != minode) {
				/* some edges to current machine instruction -> no leader */

				mi->flags &= ~SCHEDULE_LEADER;

				/* get use machine instruction */

				usemi = &sd->mi[usenl->minode];

				/* link current use node into dependency list */

				usenl->minode = minode;
				usenl->opnum2 = opnum;

				/* calculate latency */
				usenl->latency = (usemi->op[usenl->opnum].lastcycle - mi->op[opnum].firstcycle) + 1;

				usenl->next = usemi->deps;
				usemi->deps = usenl;
			}

			usenl = tmpnl;
		}

	} else {
		/* ...otherwise use last define dependency, if non-null */

		if ((defnl = *define_dep)) {
			/* some edges to current machine instruction -> no leader */

			mi->flags &= ~SCHEDULE_LEADER;

			/* get last define machine instruction */

			defmi = &sd->mi[defnl->minode];

			/* link current define node into dependency list */

			defnl->minode = minode;
			defnl->opnum2 = opnum;

			/* calculate latency, for def-def add 1 cycle */
			defnl->latency = (defmi->op[defnl->opnum].lastcycle - mi->op[opnum].firstcycle) + 1;

			defnl->next = defmi->deps;
			defmi->deps = defnl;
		}
	}

	/* Set current instruction as new define dependency and clear use         */
	/* dependencies.                                                          */

	nl = DNEW(nodelink);
	nl->minode = minode;
	nl->opnum = opnum;
	
	*define_dep = nl;
	*use_dep = NULL;
}


/* schedule_add_use_dep ********************************************************

   XXX

*******************************************************************************/

void schedule_add_use_dep(scheduledata *sd, s1 opnum, nodelink **define_dep, nodelink **use_dep)
{
	minstruction *mi;
	minstruction *defmi;
	nodelink     *nl;
	s4            minode;
	s4            defnode;
	nodelink     *defnl;

	/* get current machine instruction */

	minode = sd->micount - 1;
	mi = &sd->mi[minode];

	/* get current define dependency instruction */

	if ((defnl = *define_dep)) {
		/* some edges to current machine instruction -> no leader */

		mi->flags &= ~SCHEDULE_LEADER;

		/* add node to dependency list of current define node */

		defmi = &sd->mi[defnl->minode];

		nl = DNEW(nodelink);
		nl->minode = minode;
		nl->opnum = defnl->opnum;
		nl->opnum2 = opnum;

		/* calculate latency */
		nl->latency = (defmi->op[defnl->opnum].lastcycle - mi->op[opnum].firstcycle);

		nl->next = defmi->deps;
		defmi->deps = nl;
	}

	/* add node to list of current use nodes */

	nl = DNEW(nodelink);
	nl->minode = minode;
	nl->opnum = opnum;
	nl->next = *use_dep;
	*use_dep = nl;
}


/* schedule_calc_priorities ****************************************************

   Calculates the current node's priority by getting highest priority
   of the dependency nodes, adding this nodes latency plus 1 (for the
   instruction itself).

*******************************************************************************/

void schedule_calc_priorities(scheduledata *sd)
{
	minstruction *mi;
	minstruction *lastmi;
	nodelink     *nl;
	nodelink     *tmpnl;
	s4            lastnode;
	s4            i;
	s4            j;
	s4            criticalpath;
	s4            currentpath;
	s1            firstcycle;
	s1            lastcycle;


	/* last node MUST always be the last instruction scheduled */

	lastnode = sd->micount - 1;

	/* if last instruction is the first instruction, skip everything */

	if (lastnode > 0) {
		lastmi = &sd->mi[lastnode];

		/* last instruction is no leader */

		lastmi->flags &= ~SCHEDULE_LEADER;

		/* last instruction has no dependencies, use virtual sink node */

		firstcycle = 127;
		lastcycle = 0;

		for (j = 0; j < 4; j++) {
			if (lastmi->op[j].firstcycle < firstcycle)
				firstcycle = lastmi->op[j].firstcycle;

			if (lastmi->op[j].lastcycle > lastcycle)
				lastcycle = lastmi->op[j].lastcycle;
		}

		lastmi->priority = (lastcycle - firstcycle) + 1;


		/* walk through all remaining machine instructions backwards */

		for (i = lastnode - 1, mi = &sd->mi[lastnode - 1]; i >= 0; i--, mi--) {
			nl = mi->deps;

#if 0
			/* no dependencies (is a sink node), dependency to last node */

			if (!nl) {
				nl = DNEW(nodelink);
				nl->minode = lastnode;
				nl->next = mi->deps;
				mi->deps = nl;
			}

			/* each node MUST have at least one dependency */
#endif

			if (nl) {
				s4 priority;

				criticalpath = 0;

				/* walk through depedencies and calculate highest latency */

				while (nl) {
					priority = sd->mi[nl->minode].priority;

					currentpath = nl->latency + priority;

					if (currentpath > criticalpath)
						criticalpath = currentpath;

					nl = nl->next;
				}

				/* set priority to critical path */

				mi->priority = criticalpath;

			} else {
				/* no dependencies, use virtual sink node */

				firstcycle = 127;
				lastcycle = 0;

				for (j = 0; j < 4; j++) {
					if (mi->op[j].firstcycle < firstcycle)
						firstcycle = mi->op[j].firstcycle;

					if (mi->op[j].lastcycle > lastcycle)
						lastcycle = mi->op[j].lastcycle;
				}

				mi->priority = (lastcycle - firstcycle);
			}

			/* add current machine instruction to leader list, if one */

			if (mi->flags & SCHEDULE_LEADER) {
				nl = DNEW(nodelink);
				nl->minode = i;
				nl->next = sd->leaders;
				sd->leaders = nl;
			}
		}

	} else {
		/* last node is first instruction, add to leader list */

		nl = DNEW(nodelink);
		nl->minode = lastnode;
		nl->next = sd->leaders;
		sd->leaders = nl;
	}
}


static void schedule_create_graph(scheduledata *sd, s4 criticalpath)
{
	minstruction *mi;
	nodelink     *nl;
	s4            i;

	FILE *file;
	static int bb = 1;

	file = sd->file;

	fprintf(file, "subgraph cluster_%d {\n", bb);
	fprintf(file, "label = \"BB%d (nodes: %d, critical path: %d)\"\n", bb, sd->micount, criticalpath);

	for (i = 0, mi = sd->mi; i < sd->micount; i++, mi++) {

		nl = mi->deps;

		if (nl) {
			while (nl) {
				fprintf(file, "\"#%d.%d ", bb, i);
				disassinstr(file, &mi->instr);
				fprintf(file, "\"");

				fprintf(file, " -> ");

				fprintf(file, "\"#%d.%d ", bb, nl->minode);
				disassinstr(file, &sd->mi[nl->minode].instr);
				fprintf(file, "\"");

				fprintf(file, " [ label = \"%d\" ]\n", nl->latency);
				nl = nl->next;
			}

		} else {
			fprintf(file, "\"#%d.%d ", bb, i);
			disassinstr(file, &mi->instr);
			fprintf(file, "\"");

			fprintf(file, " -> ");
			
			fprintf(file, "\"end %d\"", bb);

			fprintf(file, " [ label = \"%d\" ]\n", mi->priority);
	
			fprintf(file, "\n");
		}
	}

	fprintf(file, "}\n");

	bb++;
}


/* schedule_do_schedule ********************************************************

   XXX

*******************************************************************************/

void schedule_do_schedule(scheduledata *sd)
{
	minstruction *mi;
	nodelink     *nl;
	s4            i;
	s4            j;
	s4            leaders;
	s4            criticalpath;

	/* It may be possible that no instructions are in the current basic block */
	/* because after an branch instruction the instructions are scheduled.    */

	if (sd->micount > 0) {
		/* calculate priorities and critical path */

		schedule_calc_priorities(sd);

#if DEBUGPRINT
		printf("bb start ---\n");
		printf("nodes: %d\n", sd->micount);
		printf("leaders: ");
#endif

		leaders = 0;
		criticalpath = 0;

		nl = sd->leaders;
		while (nl) {

#if DEBUGPRINT
			printf("%d ", nl->minode);
#endif

			leaders++;
			if (sd->mi[nl->minode].priority > criticalpath)
				criticalpath = sd->mi[nl->minode].priority;
			nl = nl->next;
		}

#if DEBUGPRINT
		printf("\n");
		printf("critical path: %d\n", criticalpath);

		for (i = 0, mi = sd->mi; i < sd->micount; i++, mi++) {
			disassinstr(stdout, &mi->instr);

			printf("\t--> #%d, prio=%d", i, mi->priority);

			printf(", mem=%d:%d", mi->op[0].firstcycle, mi->op[0].lastcycle);

			for (j = 1; j <= 3; j++) {
				printf(", op%d=%d:%d", j, mi->op[j].firstcycle, mi->op[j].lastcycle);
			}

			printf(", deps= ");
			nl = mi->deps;
			while (nl) {
				printf("#%d (op%d->op%d: %d) ", nl->minode, nl->opnum, nl->opnum2, nl->latency);
				nl = nl->next;
			}
			printf("\n");
		}
		printf("bb end ---\n\n");

		schedule_create_graph(sd, criticalpath);
#endif

#if defined(STATISTICS)
		if (opt_stat) {
			count_schedule_basic_blocks++;
			count_schedule_nodes += sd->micount;
			count_schedule_leaders += leaders;

			if (leaders > count_schedule_max_leaders)
				count_schedule_max_leaders = leaders;

			count_schedule_critical_path += criticalpath;
		}
#endif
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
 */
