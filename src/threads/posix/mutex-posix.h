/* src/threads/posix/mutex-posix.h - POSIX mutual exclusion functions

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


#ifndef _MUTEX_POSIX_H
#define _MUTEX_POSIX_H

#include "config.h"

#include <pthread.h>

#include "vm/vm.h"


/* POSIX mutex object *********************************************************/

typedef pthread_mutex_t mutex_t;


/* static mutex initializer ***************************************************/

#define MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER


/* inline functions ***********************************************************/

/* mutex_init ******************************************************************

   Initializes the given mutex object and checks for errors.

   ARGUMENTS:
       mutex ... POSIX mutex object

*******************************************************************************/

inline static void mutex_init(mutex_t *mutex)
{
	int result;

	result = pthread_mutex_init(mutex, NULL);

	if (result != 0)
		vm_abort_errnum(result, "mutex_init: pthread_mutex_init failed");
}


/* mutex_lock ******************************************************************

   Locks the given mutex object and checks for errors. If the mutex is
   already locked by another thread, the calling thread is suspended until
   the mutex is unlocked.

   If the mutex is already locked by the calling thread, the same applies,
   thus effectively causing the calling thread to deadlock. (This is because
   we use "fast" pthread mutexes without error checking.)

   ARGUMENTS:
       mutex ... POSIX mutex object

*******************************************************************************/

inline static void mutex_lock(mutex_t *mutex)
{
	int result;

	result = pthread_mutex_lock(mutex);

	if (result != 0)
		vm_abort_errnum(result, "mutex_lock: pthread_mutex_lock failed");
}


/* mutex_unlock ****************************************************************

   Unlocks the given mutex object and checks for errors. The mutex is
   assumed to be locked and owned by the calling thread.

   ARGUMENTS:
       mutex ... POSIX mutex object

*******************************************************************************/

inline static void mutex_unlock(mutex_t *mutex)
{
	int result;

	result = pthread_mutex_unlock(mutex);

	if (result != 0)
		vm_abort_errnum(result, "mutex_unlock: pthread_mutex_unlock failed");
}


/* mutex_destroy ***************************************************************

   Destroys the given mutex object and checks for errors.

   ARGUMENTS:
       mutex ... POSIX mutex object

*******************************************************************************/

inline static void mutex_destroy(mutex_t *mutex)
{
	int result;

	result = pthread_mutex_destroy(mutex);

	if (result != 0)
		vm_abort_errnum(result, "mutex_destroy: pthread_mutex_destroy failed");
}


#endif /* _MUTEX_POSIX_H */


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
