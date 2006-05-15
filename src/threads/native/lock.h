/* src/threads/native/lock.h - lock implementation

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

   Authors: Stefan Ring

   Changes: Christian Thalinger
   			Edwin Steiner

   $Id: threads.h 4866 2006-05-01 21:40:38Z edwin $

*/


#ifndef _LOCK_H
#define _LOCK_H

#include "config.h"
#include "vm/types.h"

/* XXX Darwin */
#include <semaphore.h>

#include "vm/global.h"

/* typedefs *******************************************************************/

typedef struct lock_execution_env_t      lock_execution_env_t;
typedef struct lock_record_t             lock_record_t;
typedef struct lock_record_pool_header_t lock_record_pool_header_t;
typedef struct lock_record_pool_t        lock_record_pool_t;
typedef struct lock_waiter_t             lock_waiter_t;


/* lock_execution_env_t ********************************************************

   Execution environment. Contains the lock record freelist and pools.

*******************************************************************************/

struct lock_execution_env_t {
	lock_record_t         *firstfree;        /* lock record freelist          */
	lock_record_pool_t    *lockrecordpools;  /* list of per-thread pools      */
	int                    lockrecordcount;  /* # of records for this thread  */
};


/* lock_waiter_t ***************************************************************

   List node for storing a waiting thread.

*******************************************************************************/

struct lock_waiter_t {
	struct threadobject *waiter;         /* the waiting thread                */
	lock_waiter_t       *next;           /* next in list                      */
};


/* lock_record_t ***************************************************************

   Lock record struct representing an inflated ("fat") lock.

*******************************************************************************/

struct lock_record_t {
	struct threadobject *owner;              /* current owner of this monitor */
	s4                   count;              /* recursive lock count          */
	pthread_mutex_t      mutex;              /* mutex for synchronizing       */
	lock_waiter_t       *waiters;            /* list of threads waiting       */
	lock_record_t       *nextfree;           /* next in free list             */
};


/* XXXXXXXXXXXXXXXXX ***********************************************************

*******************************************************************************/

struct lock_record_pool_header_t {
	lock_record_pool_t *next;                /* next pool                     */
	int                 size;                /* records in this pool          */
}; 


/* XXXXXXXXXXXXXXXXX ***********************************************************

*******************************************************************************/

struct lock_record_pool_t {
	lock_record_pool_header_t header;        /* pool header (see above)       */
	lock_record_t             lr[1];         /* variable array of records     */
};


/* functions ******************************************************************/

void lock_init(void);

void lock_init_execution_env(struct threadobject *thread);
void lock_record_free_pools(lock_record_pool_t *pool);

void lock_init_object_lock(java_objectheader *);
lock_record_t *lock_get_initial_lock_word(void);

ptrint lock_pre_compute_thinlock(s4 index);

void lock_monitor_enter(struct threadobject *, java_objectheader *);
bool lock_monitor_exit(struct threadobject *, java_objectheader *);

bool lock_does_thread_hold_lock(struct threadobject *t, java_objectheader *o);

void lock_wait_for_object(java_objectheader *o, s8 millis, s4 nanos);
void lock_notify_object(java_objectheader *o);
void lock_notify_all_object(java_objectheader *o);

#endif /* _LOCK_H */


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
