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

   $Id: schedule.c 1950 2005-02-17 11:40:01Z twisti $

*/


#include <stdio.h>

#include "mm/memory.h"
#include "vm/jit/schedule/schedule.h"


void schedule_do_schedule(minstruction *mi)
{
	minstruction *tmpmi;

	tmpmi = mi;

	printf("bb start ---\n");
	while (tmpmi) {
		printf("%p: %05x\n", tmpmi, tmpmi->instr);

		tmpmi = tmpmi->next;
	}
	printf("bb end ---\n\n");
}


minstruction *schedule_prepend_minstruction(minstruction *mi)
{
	minstruction *tmpmi;

	/* add new instruction in front of the list */

	tmpmi = DNEW(minstruction);
	tmpmi->opdep[0] = NULL;
	tmpmi->opdep[1] = NULL;
	tmpmi->opdep[2] = NULL;
	tmpmi->next = mi;

	return tmpmi;
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
