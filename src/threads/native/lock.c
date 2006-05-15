/* src/threads/native/lock.c - lock implementation

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
   			Edwin Steiner

   Changes: Christian Thalinger

   $Id: threads.c 4903 2006-05-11 12:48:43Z edwin $

*/


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <pthread.h>

#include "mm/memory.h"
#include "vm/types.h"
#include "vm/global.h"
#include "vm/exceptions.h"
#include "vm/stringlocal.h"

/* arch.h must be here because it defines USE_FAKE_ATOMIC_INSTRUCTIONS */

#include "arch.h"

/* includes for atomic instructions: */

#if defined(USE_FAKE_ATOMIC_INSTRUCTIONS)
#include "threads/native/generic-primitives.h"
#else
#include "machine-instr.h"
#endif


/* #define LOCK_VERBOSE */


/******************************************************************************/
/* MACROS                                                                     */
/******************************************************************************/

/* number of lock records in the first pool allocated for a thread */
#define INITIALLOCKRECORDS 8

#define COMPARE_AND_SWAP_OLD_VALUE(address, oldvalue, newvalue) \
	((ptrint) compare_and_swap((long *)(address), (long)(oldvalue), (long)(newvalue)))

/* CAUTION: oldvalue is evaluated twice! */
#define COMPARE_AND_SWAP_SUCCEEDS(address, oldvalue, newvalue) \
	(compare_and_swap((long *)(address), (long)(oldvalue), (long)(newvalue)) == (long)(oldvalue))


/******************************************************************************/
/* MACROS FOR THIN/FAT LOCKS                                                  */
/******************************************************************************/

/* We use a variant of the thin locks described in the paper
 * 
 *     Bacon, Konuru, Murthy, Serrano
 *     Thin Locks: Featherweight Synchronization for Java
 *	   Proceedings of the ACM Conference on Programming Language Design and 
 *	   Implementation (Montreal, Canada), SIGPLAN Notices volume 33, number 6,
 *	   June 1998
 *
 * In thin lock mode the lockword (monitorPtr) looks like this:
 *
 *     ,----------------------,-----------,---,
 *     |      thread ID       |   count   | 0 |
 *     `----------------------'-----------'---´
 *
 *     thread ID......the 'index' of the owning thread, or 0
 *     count..........number of times the lock has been entered	minus 1
 *     0..............the shape bit is 0 in thin lock mode
 *
 * In fat lock mode it is basically a lock_record_t *:
 *
 *     ,----------------------------------,---,
 *     |    lock_record_t * (without LSB) | 1 |
 *     `----------------------------------'---´
 *
 *     1..............the shape bit is 1 in fat lock mode
 */

#if SIZEOF_VOID_P == 8
#define THIN_LOCK_WORD_SIZE    64
#else
#define THIN_LOCK_WORD_SIZE    32
#endif

#define THIN_LOCK_SHAPE_BIT    0x01

#define THIN_LOCK_COUNT_SHIFT  1
#define THIN_LOCK_COUNT_SIZE   8
#define THIN_LOCK_COUNT_INCR   (1 << THIN_LOCK_COUNT_SHIFT)
#define THIN_LOCK_COUNT_MAX    ((1 << THIN_LOCK_COUNT_SIZE) - 1)
#define THIN_LOCK_COUNT_MASK   (THIN_LOCK_COUNT_MAX << THIN_LOCK_COUNT_SHIFT)

#define THIN_LOCK_TID_SHIFT    (THIN_LOCK_COUNT_SIZE + THIN_LOCK_COUNT_SHIFT)
#define THIN_LOCK_TID_SIZE     (THIN_LOCK_WORD_SIZE - THIN_LOCK_TID_SHIFT)

#define IS_THIN_LOCK(lockword)  (!((lockword) & THIN_LOCK_SHAPE_BIT))
#define IS_FAT_LOCK(lockword)     ((lockword) & THIN_LOCK_SHAPE_BIT)

#define GET_FAT_LOCK(lockword)  ((lock_record_t *) ((lockword) & ~THIN_LOCK_SHAPE_BIT))
#define MAKE_FAT_LOCK(ptr)      ((ptrint)(ptr) | THIN_LOCK_SHAPE_BIT)

#define LOCK_WORD_WITHOUT_COUNT(lockword) ((lockword) & ~THIN_LOCK_COUNT_MASK)


/******************************************************************************/
/* GLOBAL VARIABLES                                                           */
/******************************************************************************/

/* global lock record pool list header */
lock_record_pool_t *lock_global_pool;

/* mutex for synchronizing access to the global pool */
pthread_mutex_t lock_global_pool_lock;



/*============================================================================*/
/* INITIALIZATION OF DATA STRUCTURES                                          */
/*============================================================================*/


/* lock_init *******************************************************************

   Initialize global data for locking.

*******************************************************************************/

void lock_init(void)
{
	pthread_mutex_init(&lock_global_pool_lock, NULL);
}


/* lock_record_init ************************************************************

   Initialize a lock record.

   IN:
      r............the lock record to initialize
	  t............will become the owner

*******************************************************************************/

static void lock_record_init(lock_record_t *r, threadobject *t)
{
	r->owner = t;
	r->count = 0;
	r->waiters = NULL;

#if !defined(NDEBUG)
	r->nextfree = NULL;
#endif

	pthread_mutex_init(&(r->mutex), NULL);
}


/* lock_init_execution_env *****************************************************

   Initialize the execution environment for a thread.

   IN:
      thread.......the thread

*******************************************************************************/

void lock_init_execution_env(threadobject *thread)
{
	thread->ee.firstfree = NULL;
	thread->ee.lockrecordpools = NULL;
	thread->ee.lockrecordcount = 0;
}



/* lock_pre_compute_thinlock ***************************************************

   Pre-compute the thin lock value for a thread index.

   IN:
      index........the thead index (>= 1)

   RETURN VALUE:
      the thin lock value for this thread index

*******************************************************************************/

ptrint lock_pre_compute_thinlock(s4 index)
{
	return index << THIN_LOCK_TID_SHIFT;
}



/*============================================================================*/
/* LOCK RECORD MANAGEMENT                                                     */
/*============================================================================*/


/* lock_record_alloc_new_pool **************************************************

   Get a new lock record pool from the memory allocator.

   IN:
      thread.......the thread that will own the lock records
	  size.........number of lock records in the pool to allocate

   RETURN VALUE:
      the new lock record pool, with initialized lock records

*******************************************************************************/

static lock_record_pool_t *lock_record_alloc_new_pool(threadobject *thread, int size)
{
	int i;
	lock_record_pool_t *pool;

	/* get the pool from the memory allocator */

	pool = mem_alloc(sizeof(lock_record_pool_header_t)
				   + sizeof(lock_record_t) * size);

	/* initialize the pool header */

	pool->header.size = size;

	/* initialize the individual lock records */

	for (i=0; i<size; i++) {
		lock_record_init(&pool->lr[i], thread);

		pool->lr[i].nextfree = &pool->lr[i+1];
	}

	/* terminate free list */

	pool->lr[i-1].nextfree = NULL;

	return pool;
}


/* lock_record_alloc_pool ******************************************************

   Allocate a lock record pool. The pool is either taken from the global free
   list or requested from the memory allocator.

   IN:
      thread.......the thread that will own the lock records
	  size.........number of lock records in the pool to allocate

   RETURN VALUE:
      the new lock record pool, with initialized lock records

*******************************************************************************/

static lock_record_pool_t *lock_record_alloc_pool(threadobject *t, int size)
{
	pthread_mutex_lock(&lock_global_pool_lock);

	if (lock_global_pool) {
		int i;
		lock_record_pool_t *pool;

		/* pop a pool from the global freelist */

		pool = lock_global_pool;
		lock_global_pool = pool->header.next;

		pthread_mutex_unlock(&lock_global_pool_lock);

		/* re-initialize owner and freelist chaining */

		for (i=0; i < pool->header.size; i++) {
			pool->lr[i].owner = t;
			pool->lr[i].nextfree = &pool->lr[i+1];
		}
		pool->lr[i-1].nextfree = NULL;

		return pool;
	}

	pthread_mutex_unlock(&lock_global_pool_lock);

	/* we have to get a new pool from the allocator */

	return lock_record_alloc_new_pool(t, size);
}


/* lock_record_free_pools ******************************************************

   Free the lock record pools in the given linked list. The pools are inserted
   into the global freelist.

   IN:
      pool.........list header

*******************************************************************************/

void lock_record_free_pools(lock_record_pool_t *pool)
{
	lock_record_pool_header_t *last;

	assert(false); /* XXX this function does not match the new locking */
	               /*     algorithm. We must find another way to free  */
	               /*     unused lock records.                         */

	if (!pool)
		return;

	pthread_mutex_lock(&lock_global_pool_lock);

	/* find the last pool in the list */

	last = &pool->header;
	while (last->next)
		last = &last->next->header;

	/* chain it to the lock_global_pool freelist */

	last->next = lock_global_pool;

	/* insert the freed pools into the freelist */

	lock_global_pool = pool;

	pthread_mutex_unlock(&lock_global_pool_lock);
}


/* lock_record_alloc ***********************************************************

   Allocate a lock record which is owned by the current thread.

   IN:
      t............the current thread 

   POST-CONDITION:
      The current thread holds the mutex of the returned lock record
	  and is recored as owner of the record.

*******************************************************************************/

static lock_record_t *lock_record_alloc(threadobject *t)
{
	lock_record_t *r;

	assert(t);
	r = t->ee.firstfree;

	if (!r) {
		int poolsize;
		lock_record_pool_t *pool;

		/* get a new pool */

		poolsize = t->ee.lockrecordcount ? t->ee.lockrecordcount * 2 : INITIALLOCKRECORDS;
		pool = lock_record_alloc_pool(t, poolsize);

		/* add it to our per-thread pool list */

		pool->header.next = t->ee.lockrecordpools;
		t->ee.lockrecordpools = pool;
		t->ee.lockrecordcount += pool->header.size;

		/* take the first record from the pool */
		r = &pool->lr[0];
	}

	/* pop the record from the freelist */

	t->ee.firstfree = r->nextfree;
#ifndef NDEBUG
	r->nextfree = NULL; /* in order to find invalid uses of nextfree */
#endif

	/* pre-acquire the mutex of the new lock record */

	pthread_mutex_lock(&(r->mutex));

	return r;
}


/* lock_record_recycle *********************************************************

   Recycle the given lock record. It will be inserted in the appropriate
   free list.

   IN:
      t............the owner
	  r............lock record to recycle

*******************************************************************************/

static inline void lock_record_recycle(threadobject *t, lock_record_t *r)
{
	assert(t);
	assert(r);
	assert(r->owner == t);
	assert(r->nextfree == NULL);

	r->nextfree = t->ee.firstfree;
	t->ee.firstfree = r;
}



/*============================================================================*/
/* OBJECT LOCK INITIALIZATION                                                 */
/*============================================================================*/


/* lock_init_object_lock *******************************************************

   Initialize the monitor pointer of the given object. The monitor gets
   initialized to an unlocked state.

*******************************************************************************/

void lock_init_object_lock(java_objectheader *o)
{
	assert(o);

	o->monitorPtr = NULL;
}


/* lock_get_initial_lock_word **************************************************

   Returns the initial (unlocked) lock word. The pointer is
   required in the code generator to set up a virtual
   java_objectheader for code patch locking.

*******************************************************************************/

lock_record_t *lock_get_initial_lock_word(void)
{
	return NULL;
}



/*============================================================================*/
/* LOCKING ALGORITHM                                                          */
/*============================================================================*/


/* lock_inflate ****************************************************************

   Inflate the lock of the given object. This may only be called by the
   owner of the monitor.

   IN:
      t............the current thread
	  o............the object of which to inflate the lock

   RETURN VALUE:
      the new lock record of the object

   PRE-CONDITION:
      The current thread must be the owner of this object's monitor!

*******************************************************************************/

static lock_record_t *lock_inflate(threadobject *t, java_objectheader *o)
{
	lock_record_t *lr;
	ptrint lockword;
	ptrint count;

	/* get the current lock count */

	lockword = (ptrint) o->monitorPtr;

	assert( LOCK_WORD_WITHOUT_COUNT(lockword) == t->thinlock );

	count = (lockword & THIN_LOCK_COUNT_MASK) >> THIN_LOCK_COUNT_SHIFT;

	/* allocate a fat lock */

	lr = lock_record_alloc(t);
	lr->count = count;

#if defined(LOCK_VERBOSE)
	printf("thread %3d: inflating lock of object %p current lockword %lx, count %d\n",
			t->index, (void*) o, (long)o->monitorPtr, (int)count);
#endif

	/* install it */

	o->monitorPtr = (lock_record_t *) MAKE_FAT_LOCK(lr);

	return lr;
}


/* lock_monitor_enter **********************************************************

   Acquire the monitor of the given object. If the current thread already
   owns the monitor, the lock counter is simply increased.

   This function blocks until it can acquire the monitor.

   IN:
      t............the current thread
	  o............the object of which to enter the monitor

   RETURN VALUE:
      the new lock record of the object when it has been entered, or
	  NULL if a thin lock has been entered

*******************************************************************************/

lock_record_t *lock_monitor_enter(threadobject *t, java_objectheader *o)
{
	/* CAUTION: This code assumes that ptrint is unsigned! */
	ptrint lockword;
	ptrint thinlock;

	thinlock = t->thinlock;

	/* most common case: try to thin-lock an unlocked object */

	if ((lockword = COMPARE_AND_SWAP_OLD_VALUE(&(o->monitorPtr), 0, thinlock)) == 0) {
		/* success. we locked it */
		/* The Java Memory Model requires a memory barrier here: */
		MEMORY_BARRIER();
		return NULL;
	}

	/* next common case: recursive lock with small recursion count */
	/* We don't have to worry about stale values here, as any stale value  */
	/* will indicate another thread holding the lock (or an inflated lock) */

	if (LOCK_WORD_WITHOUT_COUNT(lockword) == thinlock) {
		/* we own this monitor               */
		/* check the current recursion count */

		if ((lockword ^ thinlock) < (THIN_LOCK_COUNT_MAX << THIN_LOCK_COUNT_SHIFT))
		{
			/* the recursion count is low enough */

			o->monitorPtr = (lock_record_t *) (lockword + THIN_LOCK_COUNT_INCR);

			/* success. we locked it */
			return NULL;
		}
		else {
			lock_record_t *lr;

			/* recursion count overflow */

			lr = lock_inflate(t, o);
			lr->count++;

			return lr;
		}
	}

	/* the lock is either contented or fat */

	{
		lock_record_t *lr;
		ptrint fatlock;

		if (IS_FAT_LOCK(lockword)) {

			lr = GET_FAT_LOCK(lockword);

			/* check for recursive entering */
			if (lr->owner == t) {
				lr->count++;
				return lr;
			}
		}
		else {
			/* alloc a lock record owned by us */
			lr = lock_record_alloc(t);
			fatlock = MAKE_FAT_LOCK(lr);

#if defined(LOCK_VERBOSE)
			printf("thread %3d: SPINNING for inflating lock of %p, current lockword = %lx\n",
					t->index, (void*)o, (long)lockword);
#endif

			/* SPIN LOOP */
			while (true) {
				lockword = COMPARE_AND_SWAP_OLD_VALUE(&(o->monitorPtr), 0, fatlock);
				if (lockword == 0) {
#if defined(LOCK_VERBOSE)
					printf("thread %3d: successfully inflated lock of %p\n",
							t->index, (void*)o);
#endif
					/* we managed to install our lock record */
					/* The Java Memory Model requires a memory barrier here: */
					MEMORY_BARRIER();
					return lr;
				}

				if (IS_FAT_LOCK(lockword)) {
#if defined(LOCK_VERBOSE)
					printf("thread %3d: lock of %p was inflated by other thread, lockword = %lx\n",
							t->index, (void*)o, (long)lockword);
#endif
					/* another thread inflated the lock */
					pthread_mutex_unlock(&(lr->mutex));
					lock_record_recycle(t, lr);

					lr = GET_FAT_LOCK(lockword);
					break;
				}
			}
		}

		/* acquire the mutex of the lock record */
		pthread_mutex_lock(&(lr->mutex));

		/* enter us as the owner */
		lr->owner = t;

		assert(lr->count == 0);

		return lr;
	}
}


/* lock_monitor_exit *****************************************************************

   Decrement the counter of a (currently owned) monitor. If the counter
   reaches zero, release the monitor.

   If the current thread is not the owner of the monitor, an 
   IllegalMonitorState exception is thrown.

   IN:
      t............the current thread
	  o............the object of which to exit the monitor

   RETURN VALUE:
      true.........everything ok,
	  false........an exception has been thrown

*******************************************************************************/

bool lock_monitor_exit(threadobject *t, java_objectheader *o)
{
	ptrint lockword;
	ptrint thinlock;

	/* We don't have to worry about stale values here, as any stale value */
	/* will indicate that we don't own the lock.                          */

	lockword = (ptrint) o->monitorPtr;
	thinlock = t->thinlock;

	/* most common case: we release a thin lock that we hold once */

	if (lockword == thinlock) {
		/* memory barrier for Java Memory Model */
		MEMORY_BARRIER();
		o->monitorPtr = 0;
		/* memory barrier for thin locking */
		MEMORY_BARRIER();
		return true;
	}

	/* next common case: we release a recursive lock, count > 0 */

	if (LOCK_WORD_WITHOUT_COUNT(lockword) == thinlock) {
		o->monitorPtr = (lock_record_t *) (lockword - THIN_LOCK_COUNT_INCR);
		return true;
	}

	/* either the lock is fat, or we don't hold it at all */

	if (IS_FAT_LOCK(lockword)) {

		lock_record_t *lr;

		lr = GET_FAT_LOCK(lockword);

		/* check if we own this monitor */
		/* We don't have to worry about stale values here, as any stale value */
		/* will be != t and thus fail this check.                             */

		if (lr->owner != t) {
			*exceptionptr = new_illegalmonitorstateexception();
			return false;
		}

		/* { the current thread `t` owns the lock record `lr` on object `o` } */

		if (lr->count != 0) {
			/* we had locked this one recursively. just decrement, it will */
			/* still be locked. */
			lr->count--;
			return true;
		}

		/* unlock this lock record */

		lr->owner = NULL;
		pthread_mutex_unlock(&(lr->mutex));

		return true;
	}

	/* legal thin lock cases have been handled above, so this is an error */

	*exceptionptr = new_illegalmonitorstateexception();
	return false;
}


/* lock_record_remove_waiter *********************************************************

   Remove a thread from the list of waiting threads of a lock record.

   IN:
      lr...........the lock record
      t............the current thread

   PRE-CONDITION:
      The current thread must be the owner of the lock record.
   
*******************************************************************************/

void lock_record_remove_waiter(lock_record_t *lr, threadobject *t)
{
	lock_waiter_t **link;
	lock_waiter_t *w;

	link = &(lr->waiters);
	while ((w = *link)) {
		if (w->waiter == t) {
			*link = w->next;
			return;
		}

		link = &(w->next);
	}

	/* this should never happen */
	fprintf(stderr,"error: waiting thread not found in list of waiters\n");
	fflush(stderr);
	abort();
}


/* lock_monitor_wait *****************************************************************

   Wait on an object for a given (maximum) amount of time.

   IN:
      t............the current thread
	  o............the object
	  millis.......milliseconds of timeout
	  nanos........nanoseconds of timeout

   PRE-CONDITION:
      The current thread must be the owner of the object's monitor.
   
*******************************************************************************/

void lock_monitor_wait(threadobject *t, java_objectheader *o, s8 millis, s4 nanos)
{
	ptrint         lockword;
	lock_record_t *lr;
	lock_waiter_t *waiter;
	s4             lockcount;
	bool           wasinterrupted;

	lockword = (ptrint) o->monitorPtr;

	/* check if we own this monitor */
	/* We don't have to worry about stale values here, as any stale value */
	/* will fail this check.                                              */

	if (IS_FAT_LOCK(lockword)) {

		lr = GET_FAT_LOCK(lockword);

		if (lr->owner != t) {
			*exceptionptr = new_illegalmonitorstateexception();
			return;
		}
	}
	else {
		/* it's a thin lock */

		if (LOCK_WORD_WITHOUT_COUNT(lockword) != t->thinlock) {
			*exceptionptr = new_illegalmonitorstateexception();
			return;
		}

		/* inflate this lock */
		lr = lock_inflate(t, o);
	}

	/* { the thread t owns the fat lock record lr on the object o } */

	/* register us as waiter for this object */

	waiter = NEW(lock_waiter_t);
	waiter->waiter = t;
	waiter->next = lr->waiters;
	lr->waiters = waiter;

	/* remember the old lock count */

	lockcount = lr->count;

	/* unlock this record */

	lr->count = 0;
	lr->owner = NULL;
	pthread_mutex_unlock(&(lr->mutex));

	/* wait until notified/interrupted/timed out */

	wasinterrupted = threads_wait_with_timeout_relative(t, millis, nanos);

	/* re-enter the monitor */

	lr = lock_monitor_enter(t, o);

	/* remove us from the list of waiting threads */

	lock_record_remove_waiter(lr, t);

	/* restore the old lock count */

	lr->count = lockcount;

	/* if we have been interrupted, throw the appropriate exception */

	if (wasinterrupted)
		*exceptionptr = new_exception(string_java_lang_InterruptedException);
}


/* lock_monitor_notify **************************************************************

   Notify one thread or all threads waiting on the given object.

   IN:
      t............the current thread
	  o............the object
	  one..........if true, only notify one thread

   PRE-CONDITION:
      The current thread must be the owner of the object's monitor.
   
*******************************************************************************/

static void lock_monitor_notify(threadobject *t, java_objectheader *o, bool one)
{
	ptrint lockword;
	lock_record_t *lr;
	lock_waiter_t *waiter;
	threadobject *waitingthread;

	lockword = (ptrint) o->monitorPtr;

	/* check if we own this monitor */
	/* We don't have to worry about stale values here, as any stale value */
	/* will fail this check.                                              */

	if (IS_FAT_LOCK(lockword)) {

		lr = GET_FAT_LOCK(lockword);

		if (lr->owner != t) {
			*exceptionptr = new_illegalmonitorstateexception();
			return;
		}
	}
	else {
		/* it's a thin lock */

		if (LOCK_WORD_WITHOUT_COUNT(lockword) != t->thinlock) {
			*exceptionptr = new_illegalmonitorstateexception();
			return;
		}

		/* inflate this lock */
		lr = lock_inflate(t, o);
	}

	/* { the thread t owns the fat lock record lr on the object o } */

	/* for each waiter: */

	for (waiter = lr->waiters; waiter; waiter = waiter->next) {

		/* signal the waiting thread */

		waitingthread = waiter->waiter;

		pthread_mutex_lock(&waitingthread->waitmutex);
		if (waitingthread->sleeping)
			pthread_cond_signal(&waitingthread->waitcond);
		waitingthread->signaled = true;
		pthread_mutex_unlock(&waitingthread->waitmutex);

		/* if we should only wake one, we are done */

		if (one)
			break;
	}
}



/*============================================================================*/
/* INQUIRY FUNCIONS                                                           */
/*============================================================================*/


/* lock_does_thread_hold_lock **************************************************

   Return true if the given thread owns the monitor of the given object.

   IN:
      t............the thread
	  o............the object
   
   RETURN VALUE:
      true, if the thread is locking the object

*******************************************************************************/

bool lock_does_thread_hold_lock(threadobject *t, java_objectheader *o)
{
	ptrint lockword;

	/* check if we own this monitor */
	/* We don't have to worry about stale values here, as any stale value */
	/* will fail this check.                                              */

	lockword = (ptrint) o->monitorPtr;

	if (IS_FAT_LOCK(lockword)) {
		lock_record_t *lr;

		/* it's a fat lock */
		lr = GET_FAT_LOCK(lockword);

		return (lr->owner == t);
	}
	else {
		/* it's a thin lock */

		return (LOCK_WORD_WITHOUT_COUNT(lockword) == t->thinlock);
	}
}



/*============================================================================*/
/* WRAPPERS FOR OPERATIONS ON THE CURRENT THREAD                              */
/*============================================================================*/


/* lock_wait_for_object ********************************************************

   Wait for the given object.

   IN:
	  o............the object
	  millis.......milliseconds to wait
	  nanos........nanoseconds to wait
   
*******************************************************************************/

void lock_wait_for_object(java_objectheader *o, s8 millis, s4 nanos)
{
	threadobject *t = (threadobject*) THREADOBJECT;
	lock_monitor_wait(t, o, millis, nanos);
}


/* lock_notify_object **********************************************************

   Notify one thread waiting on the given object.

   IN:
	  o............the object
   
*******************************************************************************/

void lock_notify_object(java_objectheader *o)
{
	threadobject *t = (threadobject*) THREADOBJECT;
	lock_monitor_notify(t, o, true);
}


/* lock_notify_all_object ******************************************************

   Notify all threads waiting on the given object.

   IN:
	  o............the object
   
*******************************************************************************/

void lock_notify_all_object(java_objectheader *o)
{
	threadobject *t = (threadobject*) THREADOBJECT;
	lock_monitor_notify(t, o, false);
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
