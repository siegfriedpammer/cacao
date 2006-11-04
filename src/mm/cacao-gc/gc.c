/* src/mm/cacao-gc/gc.c - main garbage collector methods

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

   Authors: Michael Starzinger

   Changes:

   $Id$

*/


#include "config.h"
#include <signal.h>
#include <stdlib.h>
#include "vm/types.h"

#include "heap.h"
#include "toolbox/logging.h"
#include "vm/options.h"


/* gc_init *********************************************************************

   Initializes the garbage collector.

*******************************************************************************/

void gc_init(u4 heapmaxsize, u4 heapstartsize)
{
	if (opt_verbosegc)
		dolog("GC: Initialising with heap-size %d (max. %d)",
			heapstartsize, heapmaxsize);

	heap_base = malloc(heapstartsize);

	if (heap_base == NULL)
		exceptions_throw_outofmemory_exit();

	heap_current_size = heapstartsize;
	heap_maximal_size = heapmaxsize;

	dolog("GC: Got base pointer %p", heap_base);
}


void gc_call(void)
{
	if (opt_verbosegc)
		dolog("GC: Forced Collection");
}


int GC_signum1()
{
	return SIGUSR1;
}


int GC_signum2()
{
	return SIGUSR2;
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
