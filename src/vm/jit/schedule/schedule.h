/* vm/jit/schedule/schedule.h - architecture independent instruction scheduler

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

   $Id: schedule.h 1970 2005-03-01 17:17:05Z twisti $

*/


#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include "arch.h"
#include "types.h"
#include "vm/jit/reg.h"


typedef struct scheduledata scheduledata;
typedef struct minstruction minstruction;
typedef struct nodelink nodelink;


/* scheduledata ****************************************************************

   XXX

*******************************************************************************/

struct scheduledata {
	minstruction *mi;                   /* machine instruction array          */
	s4            micount;              /* number of machine instructions     */
	nodelink     *leaders;              /* list containing sink nodes         */
	nodelink     *intregs_read_dep;
	nodelink     *intregs_write_dep;
	nodelink     *fltregs_read_dep;
	nodelink     *fltregs_write_dep;
	nodelink     *memory_read_dep;
    nodelink     *memory_write_dep;
};


/* minstruction ****************************************************************

   This structure contains all information for one machine instruction
   required to schedule it.

*******************************************************************************/

struct minstruction {
	u4            instr;                /* machine instruction word           */
	u1            latency;              /* instruction latency                */
	s4            priority;             /* priority of this instruction node  */
	nodelink     *opdep[3];             /* operand dependencies               */
	minstruction *next;                 /* link to next machine instruction   */
};


/* nodelink ********************************************************************

   XXX

*******************************************************************************/

struct nodelink {
	s4        mnode;                    /* postition in minstruction array    */
	nodelink *next;                     /* link to next node                  */
};


/* function prototypes ********************************************************/

scheduledata *schedule_setup(registerdata *rd);
void schedule_calc_priority(minstruction *mi);
void schedule_add_dep(nodelink **reg, s4 mnode);
void schedule_do_schedule(scheduledata *sd);
minstruction *schedule_prepend_minstruction(minstruction *mi);

#endif /* _SCHEDULE_H */


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
