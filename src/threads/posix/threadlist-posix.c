/* src/threads/posix/threadlist-posix.c - POSIX threadlist functions

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

#include <pthread.h>

#include "vm/vm.h"

#include "vmcore/options.h"


/* global variables ***********************************************************/

/* global mutex for the thread list */
static pthread_mutex_t threadlist_mutex;


/* threadlist_impl_init ********************************************************

   Do some early initialization of stuff required.

*******************************************************************************/

void threadlist_impl_init(void)
{
	int result;

	TRACESUBSYSTEMINITIALIZATION("threadlist_impl_init");

	/* Initialize the thread list mutex. */

	result = pthread_mutex_init(&threadlist_mutex, NULL);

	if (result != 0)
		vm_abort_errnum(result, "threadlist_impl_init: pthread_mutex_init failed");
}


/* threadlist_lock *************************************************************

   Enter the thread list mutex.

   NOTE: We need this function as we can't use an internal lock for
         the threads lists because the thread's lock is initialized in
         threads_table_add (when we have the thread index), but we
         already need the lock at the entry of the function.

*******************************************************************************/

void threadlist_lock(void)
{
	int result;

	result = pthread_mutex_lock(&threadlist_mutex);

	if (result != 0)
		vm_abort_errnum(result, "threads_list_lock: pthread_mutex_lock failed");
}


/* threadlist_unlock *********************************************************

   Leave the thread list mutex.

*******************************************************************************/

void threadlist_unlock(void)
{
	int result;

	result = pthread_mutex_unlock(&threadlist_mutex);

	if (result != 0)
		vm_abort_errnum(result, "threadlist_unlock: pthread_mutex_unlock failed");
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
