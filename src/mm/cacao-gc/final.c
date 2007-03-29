/* mm/cacao-gc/final.c - GC module for finalization and weak references

   Copyright (C) 2006 R. Grafl, A. Krall, C. Kruegel,
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

   $Id$

*/


#include "config.h"
#include "vm/types.h"

#include "gc.h"
#include "final.h"
#include "heap.h"
#include "mm/memory.h"
#include "vm/finalizer.h"


/* Global Variables ***********************************************************/

list *final_list;



void final_init()
{
	final_list = list_create(OFFSET(final_entry, linkage));
}

void final_register(java_objectheader *o, methodinfo *finalizer)
{
	final_entry *fe;

	fe = NEW(final_entry);
	fe->type      = FINAL_REACHABLE;
	fe->o         = o;
	fe->finalizer = finalizer;

	list_add_first(final_list, fe);

	GC_LOG( printf("Finalizer registered for: %p\n", (void *) o); );
}

void final_invoke()
{
	final_entry *fe;
	final_entry *fe_next;

	fe = list_first(final_list);
	fe_next = NULL;
	while (fe) {
		fe_next = list_next(final_list, fe);

		if (fe->type == FINAL_RECLAIMABLE) {

			GC_LOG( printf("Finalizer starting for: ");
					heap_print_object(fe->o); printf("\n"); );

			GC_ASSERT(fe->finalizer == fe->o->vftbl->class->finalizer);

			fe->type = FINAL_FINALIZING;

			finalizer_run(fe->o, NULL);

			fe->type = FINAL_FINALIZED;

			list_remove(final_list, fe);
			FREE(fe, final_entry);
		}

		fe = fe_next;
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
