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

   $Id: schedule.c 1973 2005-03-02 16:27:05Z twisti $

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

	sd->micount = 0;

	sd->intregs_define_dep = DMNEW(s4, rd->intregsnum);
	sd->fltregs_define_dep = DMNEW(s4, rd->fltregsnum);

	sd->intregs_use_dep = DMNEW(nodelink*, rd->intregsnum);
	sd->fltregs_use_dep = DMNEW(nodelink*, rd->fltregsnum);

	/* clear all pointers */

	MSET(sd->intregs_define_dep, 0, s4, rd->intregsnum);
	MSET(sd->fltregs_define_dep, 0, s4, rd->fltregsnum);

	MSET(sd->intregs_use_dep, 0, nodelink*, rd->intregsnum);
	MSET(sd->fltregs_use_dep, 0, nodelink*, rd->fltregsnum);

  	sd->memory_define_dep = NULL;
  	sd->memory_use_dep = NULL;

	return sd;
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
/*  		if (mi->opdep[i]) { */
/*  			if (mi->opdep[i]->priority > pathpriority) */
/*  				pathpriority = mi->opdep[i]->priority; */
/*  		} */
	}

	mi->priority = pathpriority + mi->latency + 1;
}


void schedule_add_int_define_dep(scheduledata *sd, s4 reg)
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

	minode = sd->micount;
	mi = &sd->mi[minode - 1];

	/* get current use dependency nodes, if non-null use them... */

	if ((usenl = sd->intregs_use_dep[reg])) {
		/* add a dependency link node to all use dependency nodes */

		while (usenl) {
			tmpnl = usenl->next;

			/* don't add to the current machine instruction */

			if (usenl->minode != minode) {
				/* some edges to current machine instruction -> no leader */

				mi->leader = false;

				/* get use machine instruction */

				usemi = &sd->mi[usenl->minode - 1];

/*  				nl = DNEW(nodelink); */
/*  				nl->minode = sd->micount; */
/*  				nl->next = usemi->deps; */
/*  				usemi->deps = nl; */

				usenl->minode = minode;
				usenl->next = usemi->deps;
				usemi->deps = usenl;
			}

			usenl = tmpnl;
		}

	} else {
		/* ...otherwise use last define dependency, if non-null */

		if ((defnode = sd->intregs_define_dep[reg]) > 0) {
			/* some edges to current machine instruction -> no leader */

			mi->leader = false;

			/* get last define machine instruction */

			defmi = &sd->mi[defnode - 1];

			nl = DNEW(nodelink);
			nl->minode = sd->micount;
			nl->next = defmi->deps;
			defmi->deps = nl;
		}
	}

	/* Set current instruction as new define dependency and clear use         */
	/* depedencies.                                                           */

	sd->intregs_define_dep[reg] = sd->micount;
	sd->intregs_use_dep[reg] = NULL;
}


void schedule_add_int_use_dep(scheduledata *sd, s4 reg)
{
	minstruction *mi;
	minstruction *defmi;
	nodelink     *nl;
	s4            defnode;

	/* get current machine instruction */

	mi = &sd->mi[sd->micount - 1];

	/* get current define dependency instruction */

	if ((defnode = sd->intregs_define_dep[reg]) > 0) {
		/* some edges to current machine instruction -> no leader */

		mi->leader = false;

		/* add node to dependency list of current define node */

		defmi = &sd->mi[defnode - 1];

		nl = DNEW(nodelink);
		nl->minode = sd->micount;
		nl->next = defmi->deps;
		defmi->deps = nl;
	}

	/* add node to list of current use nodes */

	nl = DNEW(nodelink);
	nl->minode = sd->micount;
	nl->next = sd->intregs_use_dep[reg];
	sd->intregs_use_dep[reg] = nl;
}


void schedule_add_flt_define_dep(scheduledata *sd, s4 reg)
{
	minstruction *mi;
	minstruction *defmi;
	minstruction *usemi;
	nodelink     *usenl;
	nodelink     *nl;
	s4            defnode;

	/* get current machine instruction */

	mi = &sd->mi[sd->micount - 1];

	/* get current use dependency nodes, if non-null use them... */

	if ((usenl = sd->fltregs_use_dep[reg])) {
		/* add a dependency link node to all use dependency nodes */

		while (usenl) {
			/* don't add to the current machine instruction */

			if (usenl->minode != sd->micount) {
				/* some edges to current machine instruction -> no leader */

				mi->leader = false;

				/* get use machine instruction */

				usemi = &sd->mi[usenl->minode - 1];

				nl = DNEW(nodelink);
				nl->minode = sd->micount;
				nl->next = usemi->deps;
				usemi->deps = nl;
			}

			usenl = usenl->next;
		}

	} else {
		/* ...otherwise use last define dependency, if non-null */

		if ((defnode = sd->fltregs_define_dep[reg]) > 0) {
			/* some edges to current machine instruction -> no leader */

			mi->leader = false;

			/* get last define machine instruction */

			defmi = &sd->mi[defnode - 1];

			nl = DNEW(nodelink);
			nl->minode = sd->micount;
			nl->next = defmi->deps;
			defmi->deps = nl;
		}
	}

	/* Set current instruction as new define dependency and clear use         */
	/* depedencies.                                                           */

	sd->fltregs_define_dep[reg] = sd->micount;
	sd->fltregs_use_dep[reg] = NULL;
}


void schedule_add_flt_use_dep(scheduledata *sd, s4 reg)
{
	minstruction *mi;
	minstruction *defmi;
	nodelink     *nl;
	s4            defnode;

	/* get current machine instruction */

	mi = &sd->mi[sd->micount - 1];

	/* get current define dependency instruction */

	if ((defnode = sd->fltregs_define_dep[reg]) > 0) {
		/* some edges to current machine instruction -> no leader */

		mi->leader = false;

		/* add node to dependency list of current define node */

		defmi = &sd->mi[defnode - 1];

		nl = DNEW(nodelink);
		nl->minode = sd->micount;
		nl->next = defmi->deps;
		defmi->deps = nl;
	}

	/* add node to list of current use nodes */

	nl = DNEW(nodelink);
	nl->minode = sd->micount;
	nl->next = sd->fltregs_use_dep[reg];
	sd->fltregs_use_dep[reg] = nl;
}


void schedule_add_memory_define_dep(scheduledata *sd)
{
}


void schedule_add_memory_use_dep(scheduledata *sd)
{
}


/* schedule_do_schedule ********************************************************

   XXX

*******************************************************************************/

void schedule_do_schedule(scheduledata *sd)
{
	minstruction *mi;
	nodelink     *nl;
	s4            i;

	printf("bb start ---\n");

	for (i = 1, mi = sd->mi; i <= sd->micount; i++, mi++) {
		disassinstr(&mi->instr);

		printf("\t--> #%d, %d, %d, %d, ", i, mi->latency, mi->priority, mi->leader);

		printf("deps= ");
		nl = mi->deps;
		while (nl) {
			printf("%d ", nl->minode);
			nl = nl->next;
		}

		printf("\n");
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
