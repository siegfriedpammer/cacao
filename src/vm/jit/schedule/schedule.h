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

   $Id: schedule.h 1950 2005-02-17 11:40:01Z twisti $

*/


#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include "arch.h"
#include "types.h"


/* minstruction ****************************************************************

   This structure contains all information for one machine instruction
   required to schedule it.

*******************************************************************************/

typedef struct minstruction minstruction;

struct minstruction {
	u4            instr;                /* machine instruction word           */
	u1            latency;              /* instruction latency                */
	s4            priority;
	minstruction *opdep[3];             /* operand dependencies               */
	minstruction *next;                 /* link to next machine instruction   */
};


/* function prototypes ********************************************************/

void schedule_do_schedule(minstruction *mi);
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
