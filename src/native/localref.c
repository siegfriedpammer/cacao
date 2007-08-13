/* src/native/localref.c - Management of local reference tables

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

#include <assert.h>
#include <stdint.h>

#include "mm/memory.h"

#include "native/localref.h"

#include "threads/threads-common.h"

#include "toolbox/logging.h"


/* debug **********************************************************************/

#if !defined(NDEBUG) && 0
# define TRACELOCALREF(message) log_println("%s", message)
#else
# define TRACELOCALREF(message)
#endif


/* global variables ***********************************************************/

#if !defined(ENABLE_THREADS)
localref_table *_no_threads_localref_table;
#endif


/* localref_table_init *********************************************************

   Initializes the local references table of the current thread.

*******************************************************************************/

bool localref_table_init(void)
{
	localref_table *lrt;

	TRACELOCALREF("table init");

	assert(LOCALREFTABLE == NULL);

	lrt = GCNEW(localref_table);

	if (lrt == NULL)
		return false;

	localref_table_add(lrt);

	return true;
}

/* localref_table_add **********************************************************

   Add a new local references table to the current thread.

*******************************************************************************/

void localref_table_add(localref_table *lrt)
{
	/* initialize the local reference table */

	lrt->capacity    = LOCALREFTABLE_CAPACITY;
	lrt->used        = 0;
	lrt->localframes = 1;
	lrt->prev        = LOCALREFTABLE;

	/* clear the references array (memset is faster the a for-loop) */

	MSET(lrt->refs, 0, void*, LOCALREFTABLE_CAPACITY);

	/* add given local references table to this thread */

	LOCALREFTABLE = lrt;
}


/* localref_table_remove *******************************************************

   Remoces the local references table from the current thread.

*******************************************************************************/

void localref_table_remove()
{
	localref_table *lrt;

	/* get current local reference table from thread */

	lrt = LOCALREFTABLE;

	assert(lrt != NULL);
	assert(lrt->localframes == 1);

	lrt = lrt->prev;

	LOCALREFTABLE = lrt;
}


/* localref_frame_push *********************************************************

   Creates a new local reference frame, in which at least a given
   number of local references can be created.

*******************************************************************************/

bool localref_frame_push(int32_t capacity)
{
	localref_table *lrt;
	localref_table *nlrt;
	int32_t         additionalrefs;

	TRACELOCALREF("frame push");

	/* get current local reference table from thread */

	lrt = LOCALREFTABLE;

	assert(lrt != NULL);
	assert(capacity > 0);

	/* Allocate new local reference table on Java heap.  Calculate the
	   additional memory we have to allocate. */

	if (capacity > LOCALREFTABLE_CAPACITY)
		additionalrefs = capacity - LOCALREFTABLE_CAPACITY;
	else
		additionalrefs = 0;

	nlrt = GCMNEW(u1, sizeof(localref_table) + additionalrefs * SIZEOF_VOID_P);

	if (nlrt == NULL)
		return false;

	/* Set up the new local reference table and add it to the local
	   frames chain. */

	nlrt->capacity    = capacity;
	nlrt->used        = 0;
	nlrt->localframes = lrt->localframes + 1;
	nlrt->prev        = lrt;

	/* store new local reference table in thread */

	LOCALREFTABLE = nlrt;

	return true;
}


/* localref_frame_pop_all ******************************************************

   Pops off all the local reference frames of the current table.

*******************************************************************************/

void localref_frame_pop_all(void)
{
	localref_table *lrt;
	localref_table *plrt;
	int32_t         localframes;

	TRACELOCALREF("frame pop all");

	/* get current local reference table from thread */

	lrt = LOCALREFTABLE;

	assert(lrt != NULL);

	localframes = lrt->localframes;

	/* Don't delete the top local frame, as this one is allocated in
	   the native stub on the stack and is freed automagically on
	   return. */

	if (localframes == 1)
		return;

	/* release all current local frames */

	for (; localframes > 1; localframes--) {
		/* get previous frame */

		plrt = lrt->prev;

		/* clear all reference entries */

		MSET(lrt->refs, 0, void*, lrt->capacity);

		lrt->prev = NULL;

		/* set new local references table */

		lrt = plrt;
	}

	/* store new local reference table in thread */

	LOCALREFTABLE = lrt;
}


/* localref_dump ***************************************************************

   Dumps all local reference tables, including all frames.

*******************************************************************************/

#if !defined(NDEBUG)
void localref_dump()
{
	localref_table *lrt;
	int i;

	/* get current local reference table from thread */

	lrt = LOCALREFTABLE;

	log_println("--------- Local Reference Tables Dump ---------");

	while (lrt != NULL) {
		log_println("Frame #%d, Used=%d, Capacity=%d, Addr=%p:", lrt->localframes, lrt->used, lrt->capacity, (void *) lrt);

		for (i = 0; i < lrt->used; i++) {
			printf("\t0x%08lx ", (intptr_t) lrt->refs[i]);
		}

		if (lrt->used != 0)
			printf("\n");

		lrt = lrt->prev;
	}
}
#endif


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
