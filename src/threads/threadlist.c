/* src/threads/threadlist.c - different thread-lists

   Copyright (C) 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

*/


#include "config.h"

#include <stdint.h>

#include "mm/memory.h"

#include "threads/threadlist.h"
#include "threads/threads-common.h"

#include "toolbox/list.h"

#include "vmcore/options.h"


/* global variables ***********************************************************/

static list_t *list_thread;                            /* global threads list */
static list_t *list_thread_free;                  /* global free threads list */
static list_t *list_thread_index_free;


typedef struct thread_index_t {
	int32_t    index;
	listnode_t linkage;
} thread_index_t;


/* threadlist_init *************************************************************

   Initialize thread-lists.

*******************************************************************************/

void threadlist_init(void)
{
	TRACESUBSYSTEMINITIALIZATION("threadlist_init");

	/* Initialize the thread lists. */

	list_thread            = list_create(OFFSET(threadobject, linkage));
	list_thread_free       = list_create(OFFSET(threadobject, linkage_free));
	list_thread_index_free = list_create(OFFSET(thread_index_t, linkage));
}


/* threadlist_add **************************************************************

   Add the given threadobject as last entry to the thread list.

   IN:
       t ... threadobject to be added

*******************************************************************************/

void threadlist_add(threadobject *t)
{
	list_add_last_unsynced(list_thread, t);
}


/* threadlist_remove ***********************************************************

   Remove the given threadobject from the thread list.

   IN:
       t ... threadobject to be removed

*******************************************************************************/

void threadlist_remove(threadobject *t)
{
	list_remove_unsynced(list_thread, t);
}


/* threadlist_first ************************************************************

   Return the first entry in the thread list.

   RETURN:
       threadobject of the first entry

*******************************************************************************/

threadobject *threadlist_first(void)
{
	threadobject *t;

	t = list_first_unsynced(list_thread);

	return t;
}


/* threads_list_next ***********************************************************

   Return the next entry in the thread list.

   IN:
       t ... threadobject to get next thread of

   RETURN:
       threadobject of the next entry

*******************************************************************************/

threadobject *threadlist_next(threadobject *t)
{
	threadobject *next;

	next = list_next_unsynced(list_thread, t);

	return next;
}


/* threadlist_free_add *********************************************************

   Add the given threadobject as last entry to the free thread list.

   IN:
       t ... threadobject to be added

*******************************************************************************/

void threadlist_free_add(threadobject *t)
{
	list_add_last_unsynced(list_thread_free, t);
}


/* threadlist_free_remove ******************************************************

   Remove the given entry from the free thread list.

   IN:
       t ... threadobject to be removed

*******************************************************************************/

void threadlist_free_remove(threadobject *t)
{
	list_remove_unsynced(list_thread_free, t);
}


/* threadlist_free_first *******************************************************

   Return the first entry in the free thread list.

   RETURN:
       threadobject of the first free entry

*******************************************************************************/

threadobject *threadlist_free_first(void)
{
	threadobject *t;

	t = list_first_unsynced(list_thread_free);

	return t;
}


/* threadlist_get_non_daemons **************************************************

   Return the number of non-daemon threads.

   NOTE: This function does a linear-search over the threads list,
         because it's only used for joining the threads.

*******************************************************************************/

int threadlist_get_non_daemons(void)
{
	threadobject *t;
	int           nondaemons;

	/* Lock the threads lists. */

	threads_list_lock();

	nondaemons = 0;

	for (t = threadlist_first(); t != NULL; t = threadlist_next(t)) {
		if (!(t->flags & THREAD_FLAG_DAEMON))
			nondaemons++;
	}

	/* Unlock the threads lists. */

	threads_list_unlock();

	return nondaemons;
}


/* threadlist_index_first ******************************************************

   Return the first entry in the thread-index list.

   RETURN VALUE:
       thread-index structure

*******************************************************************************/

static inline thread_index_t *threadlist_index_first(void)
{
	thread_index_t *ti;

	ti = list_first_unsynced(list_thread_index_free);

	return ti;
}


/* threadlist_index_add ********************************************************

   Add the given thread-index to the thread-index free list.

   IN:
       i ... thread index

*******************************************************************************/

void threadlist_index_add(int index)
{
	thread_index_t *ti;

	ti = NEW(thread_index_t);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_thread_index_t += sizeof(thread_index_t);
#endif

	/* Set the index in the structure. */

	ti->index = index;

	list_add_last_unsynced(list_thread_index_free, ti);
}


/* threadlist_index_remove *****************************************************

   Remove the given thread-index from the thread-index list and free
   the thread-index structure.

   IN:
       ti ... thread-index structure

*******************************************************************************/

static inline void threadlist_index_remove(thread_index_t *ti)
{
	list_remove_unsynced(list_thread_index_free, ti);

	FREE(ti, thread_index_t);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_thread_index_t -= sizeof(thread_index_t);
#endif
}


/* threadlist_get_free_index ***************************************************

   Return a free thread index.

   RETURN VALUE:
       free thread index

*******************************************************************************/

int threadlist_get_free_index(void)
{
	thread_index_t *ti;
	int             index;

	/* Try to get a thread index from the free-list. */

	ti = threadlist_index_first();

	/* Is a free thread index available? */

	if (ti != NULL) {
		/* Yes, get the index and remove it from the free list. */

		index = ti->index;

		threadlist_index_remove(ti);
	}
	else {
		/* Get a new the thread index. */

		index = list_thread->size + 1;
	}

	return index;
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
