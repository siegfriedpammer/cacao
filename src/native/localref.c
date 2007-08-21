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

   $Id: localref.c 8391 2007-08-21 20:34:27Z michi $

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

#if defined(ENABLE_GC_CACAO)
	/* this is freed by localref_table_destroy */
	lrt = NEW(localref_table);
#else
	/* this does not need to be freed again */
	lrt = GCNEW(localref_table);
#endif

	if (lrt == NULL)
		return false;

	localref_table_add(lrt);

	return true;
}


/* localref_table_destroy ******************************************************

   Destroys the complete local references table of the current thread.

*******************************************************************************/

bool localref_table_destroy(void)
{
	localref_table *lrt;

	TRACELOCALREF("table destroy");

	lrt = LOCALREFTABLE;

	assert(lrt);
	assert(lrt->prev == NULL);

#if defined(ENABLE_GC_CACAO)
	FREE(lrt, localref_table);
#endif

	LOCALREFTABLE = NULL;

	return true;
}


/* localref_table_add **********************************************************

   Adds a new local references table to the current thread.

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

   Removes the topmost local references table from the current thread.

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

#if defined(ENABLE_GC_CACAO)
	nlrt = MNEW(u1, sizeof(localref_table) + additionalrefs * SIZEOF_VOID_P);
#else
	nlrt = GCMNEW(u1, sizeof(localref_table) + additionalrefs * SIZEOF_VOID_P);
#endif

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
#if defined(ENABLE_GC_CACAO)
	int32_t         additionalrefs;
#endif

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

#if defined(ENABLE_GC_CACAO)
		/* for the exact GC local reference tables are not on the heap,
		   so we need to free them explicitly here. */

		if (lrt->capacity > LOCALREFTABLE_CAPACITY)
			additionalrefs = lrt->capacity - LOCALREFTABLE_CAPACITY;
		else
			additionalrefs = 0;

		MFREE(lrt, u1, sizeof(localref_table) + additionalrefs * SIZEOF_VOID_P);
#endif

		/* set new local references table */

		lrt = plrt;
	}

	/* store new local reference table in thread */

	LOCALREFTABLE = lrt;
}


/* localref_add ****************************************************************

   Adds a new entry into the local reference table and returns the
   new local reference.

*******************************************************************************/

java_handle_t *localref_add(java_object_t *o)
{
	localref_table *lrt;
	java_handle_t  *h;
	int32_t         i;

#if !defined(NDEBUG)
	if (o == NULL) {
		log_println("localref_add: WARNING: trying to add localref for (NIL)!");
		return NULL;
	}
#endif

	/* XXX: assert that we are in a GC critical section! */

	/* XXX: this is only an ugly hack */
#if defined(ENABLE_HANDLES)
	if (LOCALREFTABLE == NULL) {
		h = NEW(java_handle_t);
		h->heap_object = o;
		log_println("localref_add: WARNING: added preliminary localref %p for %p", h, o);
		return h;
	}
#endif

	/* get current local reference table from thread */

	lrt = LOCALREFTABLE;

	assert(lrt != NULL);

	/* Check if we have space for the requested reference?  No,
	   allocate a new frame.  This is actually not what the spec says,
	   but for compatibility reasons... */

    if (lrt->used == lrt->capacity) {
		if (!localref_frame_push(16))
			assert(0);

		/* get the new local reference table */ 

		lrt = LOCALREFTABLE;
	}

	/* insert the reference into the local reference table */

	for (i = 0; i < lrt->capacity; i++) {
		if (lrt->refs[i] == NULL) {
			lrt->refs[i] = o;
			lrt->used++;

#if defined(ENABLE_HANDLES)
			h = (java_handle_t *) &(lrt->refs[i]);
#else
			h = (java_handle_t *) o;
#endif

#if 0
			{
				int count = 0;
				for (lrt = LOCALREFTABLE; lrt != NULL; lrt = lrt->prev)
					count += lrt->used;
				log_println("added localref %p for %p (total count %d)", h, o, count);
				/*localref_dump();*/
			}
#endif

			return h;
		}
	}

	/* this should not happen */

	log_println("localref_add: WARNING: unable to add localref for %p", o);

	return NULL;
}


/* localref_del ****************************************************************

   Deletes an entry from the local reference table.

*******************************************************************************/

void localref_del(java_handle_t *localref)
{
	localref_table *lrt;
	java_handle_t  *h;
	int32_t         i;

	/* get local reference table from thread */

	lrt = LOCALREFTABLE;

	assert(lrt != NULL);

	/* go through all local frames */

	/* XXX: this is definitely not what the spec wants! */
	/*for (; lrt != NULL; lrt = lrt->prev) {*/

		/* and try to remove the reference */
    
		for (i = 0; i < lrt->capacity; i++) {
#if defined(ENABLE_HANDLES)
			h = (java_handle_t *) &(lrt->refs[i]);
#else
			h = (java_handle_t *) lrt->refs[i];
#endif

			if (h == localref) {
				lrt->refs[i] = NULL;
				lrt->used--;

				return;
			}
		}
	/*}*/

	/* this should not happen */

	log_println("localref_del: WARNING: unable to find localref %p", localref);
}


/* localref_dump ***************************************************************

   Dumps all local reference tables, including all frames.

*******************************************************************************/

#if !defined(NDEBUG)
# define LOCALREF_DUMP_REFS_PER_LINE 4
void localref_dump()
{
	localref_table *lrt;
	int i, j;

	/* get current local reference table from thread */

	lrt = LOCALREFTABLE;

	log_println("--------- Local Reference Tables Dump ---------");

	while (lrt != NULL) {
		log_println("Frame #%d, Used=%d, Capacity=%d, Addr=%p:", lrt->localframes, lrt->used, lrt->capacity, (void *) lrt);

			if (lrt->used != 0) {

				log_start();

				j = 0;
				for (i = 0; i < lrt->capacity; i++) {
					if (lrt->refs[i] != NULL) {
						if (j != 0 && j % LOCALREF_DUMP_REFS_PER_LINE == 0) {
							log_finish();
							log_start();
						}
						j++;
						log_print("\t0x%016lx ", (intptr_t) lrt->refs[i]);
					}
				}

				log_finish();
			}

		lrt = lrt->prev;
	}
}
#endif /* !defined(NDEBUG) */


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
