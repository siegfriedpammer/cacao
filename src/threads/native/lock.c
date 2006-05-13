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

   Changes: Christian Thalinger
   			Edwin Steiner

   $Id: threads.c 4903 2006-05-11 12:48:43Z edwin $

*/


#include "config.h"

/* XXX cleanup these includes */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include <pthread.h>
#include <semaphore.h>

#include "vm/types.h"

#include "arch.h"

#ifndef USE_MD_THREAD_STUFF
#include "machine-instr.h"
#else
#include "threads/native/generic-primitives.h"
#endif

#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/native.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Throwable.h"
#include "native/include/java_lang_Thread.h"
#include "native/include/java_lang_ThreadGroup.h"
#include "native/include/java_lang_VMThread.h"
#include "threads/native/threads.h"
#include "toolbox/avl.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"
#include "vm/jit/asmpart.h"

#if !defined(__DARWIN__)
#if defined(__LINUX__)
#define GC_LINUX_THREADS
#elif defined(__MIPS__)
#define GC_IRIX_THREADS
#endif
#include "boehm-gc/include/gc.h"
#endif

#ifdef USE_MD_THREAD_STUFF
pthread_mutex_t _atomic_add_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _cas_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _mb_lock = PTHREAD_MUTEX_INITIALIZER;
#endif


/******************************************************************************/
/* MACROS                                                                     */
/******************************************************************************/

#define INITIALLOCKRECORDS 8

#define GRAB_LR(lr,t) \
    if (lr->owner != t) { \
		lr = lr->incharge; \
	}

#define CHECK_MONITORSTATE(lr,t,mo,a) \
    if (lr->o != mo || lr->owner != t) { \
		*exceptionptr = new_illegalmonitorstateexception(); \
		a; \
	}


/******************************************************************************/
/* GLOBAL VARIABLES                                                           */
/******************************************************************************/

/* unlocked dummy record - avoids NULL checks */
static lock_record_t *dummyLR;

pthread_mutex_t lock_global_pool_lock;
lock_record_pool_t *lock_global_pool;



/*============================================================================*/
/* INITIALIZATION OF DATA STRUCTURES                                          */
/*============================================================================*/


/* lock_init *******************************************************************

   Initialize global data for locking.

*******************************************************************************/

void lock_init(void)
{
	pthread_mutex_init(&lock_global_pool_lock, NULL);

	/* Every newly created object's monitorPtr points here so we save
	   a check against NULL */

	dummyLR = NEW(lock_record_t);
	dummyLR->o = NULL;
	dummyLR->owner = NULL;
	dummyLR->waiting = NULL;
	dummyLR->incharge = dummyLR;
}


/* lock_record_init ************************************************************

   Initialize a lock record.

   IN:
      r............the lock record to initialize
	  t............will become the owner

*******************************************************************************/

static void lock_record_init(lock_record_t *r, threadobject *t)
{
	r->lockCount = 1;
	r->owner = t;
	r->queuers = 0;
	r->o = NULL;
	r->waiter = NULL;
	r->incharge = (lock_record_t *) &dummyLR;
	r->waiting = NULL;
	threads_sem_init(&r->queueSem, 0, 0);
	pthread_mutex_init(&r->resolveLock, NULL);
	pthread_cond_init(&r->resolveWait, NULL);
}


/* lock_init_execution_env *****************************************************

   Initialize the execution environment for a thread.

   IN:
      thread.......the thread

*******************************************************************************/

void lock_init_execution_env(threadobject *thread)
{
	thread->ee.firstLR = NULL;
	thread->ee.lrpool = NULL;
	thread->ee.numlr = 0;
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
	lock_record_pool_t *p = mem_alloc(sizeof(lock_record_pool_header_t)
										+ sizeof(lock_record_t) * size);
	int i;

	p->header.size = size;
	for (i=0; i<size; i++) {
		lock_record_init(&p->lr[i], thread);
		p->lr[i].nextFree = &p->lr[i+1];
	}
	p->lr[i-1].nextFree = NULL;
	return p;
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
		lock_record_pool_t *pool = lock_global_pool;
		lock_global_pool = pool->header.next;

		pthread_mutex_unlock(&lock_global_pool_lock);

		for (i=0; i < pool->header.size; i++) {
			pool->lr[i].owner = t;
			pool->lr[i].nextFree = &pool->lr[i+1];
		}
		pool->lr[i-1].nextFree = NULL;

		return pool;
	}

	pthread_mutex_unlock(&lock_global_pool_lock);

	return lock_record_alloc_new_pool(t, size);
}


/* lock_record_free_pools ******************************************************

   Free the lock record pools in the given linked list.

   IN:
      pool.........list header

*******************************************************************************/

void lock_record_free_pools(lock_record_pool_t *pool)
{
	lock_record_pool_header_t *last;

	pthread_mutex_lock(&lock_global_pool_lock);

	last = &pool->header;
	while (last->next)
		last = &last->next->header;
	last->next = lock_global_pool;
	lock_global_pool = pool;

	pthread_mutex_unlock(&lock_global_pool_lock);
}


/* lock_record_alloc ***********************************************************

   Allocate a lock record which is owned by the given thread.

   IN:
      t............the thread 

*******************************************************************************/

static lock_record_t *lock_record_alloc(threadobject *t)
{
	lock_record_t *r;

	assert(t);
	r = t->ee.firstLR;

	if (!r) {
		int poolsize = t->ee.numlr ? t->ee.numlr * 2 : INITIALLOCKRECORDS;
		lock_record_pool_t *pool = lock_record_alloc_pool(t, poolsize);
		pool->header.next = t->ee.lrpool;
		t->ee.lrpool = pool;
		r = &pool->lr[0];
		t->ee.numlr += pool->header.size;
	}

	t->ee.firstLR = r->nextFree;
#ifndef NDEBUG
	r->nextFree = NULL; /* in order to find invalid uses of nextFree */
#endif
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
	assert(r->nextFree == NULL);

	r->nextFree = t->ee.firstLR;
	t->ee.firstLR = r;
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

	o->monitorPtr = dummyLR;
}


/* lock_get_initial_lock_word **************************************************

   Returns the global dummy monitor lock record. The pointer is
   required in the code generator to set up a virtual
   java_objectheader for code patch locking.

*******************************************************************************/

lock_record_t *lock_get_initial_lock_word(void)
{
	return dummyLR;
}



/*============================================================================*/
/* LOCKING ALGORITHM                                                          */
/*============================================================================*/


/* lock_queue_on_lock_record ***************************************************

   Suspend the current thread and queue it on the given lock record.

*******************************************************************************/

static void lock_queue_on_lock_record(lock_record_t *lr, java_objectheader *o)
{
	atomic_add(&lr->queuers, 1);
	MEMORY_BARRIER_AFTER_ATOMIC();

	if (lr->o == o)
		threads_sem_wait(&lr->queueSem);

	atomic_add(&lr->queuers, -1);
}


/* lock_record_release *********************************************************

   Release the lock held by the given lock record. Threads queueing on the
   semaphore of the record will be woken up.

*******************************************************************************/

static void lock_record_release(lock_record_t *lr)
{
	int q;
	lr->o = NULL;
	MEMORY_BARRIER();
	q = lr->queuers;
	while (q--)
		threads_sem_post(&lr->queueSem);
}


static inline void lock_handle_waiter(lock_record_t *newlr,
									  lock_record_t *curlr,
									  java_objectheader *o)
{
	/* if the current lock record is used for waiting on the object */
	/* `o`, then record it as a waiter in the new lock record       */

	if (curlr->waiting == o)
		newlr->waiter = curlr;
}


/* lock_monitor_enter ****************************************************************

   Acquire the monitor of the given object. If the current thread already
   owns the monitor, the lock counter is simply increased.

   This function blocks until it can acquire the monitor.

   IN:
      t............the current thread
	  o............the object of which to enter the monitor

   RETURN VALUE:
      the new lock record of the object when it has been entered

*******************************************************************************/

lock_record_t *lock_monitor_enter(threadobject *t, java_objectheader *o)
{
	for (;;) {
		lock_record_t *lr = o->monitorPtr;
		if (lr->o != o) {
			/* the lock record does not lock this object */
			lock_record_t *nlr;
			lock_record_t *mlr;

			/* allocate a new lock record for this object */
			mlr	= lock_record_alloc(t);
			mlr->o = o;

			/* check if it is the same record the object refered to earlier */
			if (mlr == lr) {
				MEMORY_BARRIER();
				nlr = o->monitorPtr;
				if (nlr == lr) {
					/* the object still refers to the same lock record */
					/* got it! */
					lock_handle_waiter(mlr, lr, o);
					return mlr;
				}
			}
			else {
				/* no, it's another lock record */
				/* if we don't own the old record, set incharge XXX */
				if (lr->owner != t)
					mlr->incharge = lr;

				/* if the object still refers to lr, replace it by the new mlr */
				MEMORY_BARRIER_BEFORE_ATOMIC();
				nlr = (lock_record_t *) compare_and_swap((long*) &o->monitorPtr, (long) lr, (long) mlr);
			}

			if (nlr == lr) {
				/* we swapped the new record in successfully */
				if (mlr == lr || lr->o != o) {
					/* the old lock record is the same as the new one, or */
					/* it locks another object.                           */
					/* got it! */
					lock_handle_waiter(mlr, lr, o);
					return mlr;
				}
				/* lr locks the object, we have to wait */
				while (lr->o == o)
					lock_queue_on_lock_record(lr, o);

				/* got it! */
				lock_handle_waiter(mlr, lr, o);
				return mlr;
			}

			/* forget this mlr lock record, wait on nlr and try again */
			lock_record_release(mlr);
			lock_record_recycle(t, mlr);
			lock_queue_on_lock_record(nlr, o);
		}
		else {
			/* the lock record is for the object we want */

			if (lr->owner == t) {
				/* we own it already, just recurse */
				lr->lockCount++;
				return lr;
			}

			/* it's locked. we wait and then try again */
			lock_queue_on_lock_record(lr, o);
		}
	}
}


/* lock_wake_waiters ********************************************************

   For each lock record in the given waiter list, post the queueSem
   once for each queuer of the lock record.

   IN:
      lr...........the head of the waiter list

*******************************************************************************/

static void lock_wake_waiters(lock_record_t *lr)
{
	lock_record_t *tmplr;
	s4 q;

	/* move it to a local variable (Stefan commented this especially.
	 * Might be important somehow...) */

	tmplr = lr;

	do {
		q = tmplr->queuers;

		while (q--)
			threads_sem_post(&tmplr->queueSem);

		tmplr = tmplr->waiter;
	} while (tmplr != NULL && tmplr != lr); /* this breaks cycles to lr */
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
	lock_record_t *lr;

	lr = o->monitorPtr;
	GRAB_LR(lr, t);
	CHECK_MONITORSTATE(lr, t, o, return false);

	/* { the current thread `t` owns the lock record `lr` on object `o` } */

	if (lr->lockCount > 1) {
		/* we had locked this one recursively. just decrement, it will */
		/* still be locked. */
		lr->lockCount--;
		return true;
	}

	/* we are going to unlock and recycle this lock record */

	if (lr->waiter) {
		lock_record_t *wlr = lr->waiter;
		if (o->monitorPtr != lr ||
			(void*) compare_and_swap((long*) &o->monitorPtr, (long) lr, (long) wlr) != lr)
		{
			lock_record_t *nlr = o->monitorPtr;
			assert(nlr->waiter == NULL);
			nlr->waiter = wlr; /* XXX is it ok to overwrite the nlr->waiter field like that? */
			STORE_ORDER_BARRIER();
		}
		else {
			lock_wake_waiters(wlr);
		}
		lr->waiter = NULL;
	}

	/* unlock and throw away this lock record */
	lock_record_release(lr);
	lock_record_recycle(t, lr);
	return true;
}


/* lock_record_remove_waiter *******************************************************

   Remove a waiter lock record from the waiter list of the given lock record

   IN:
      lr...........the lock record holding the waiter list
	  toremove.....the record to remove from the list

*******************************************************************************/

static void lock_record_remove_waiter(lock_record_t *lr,
									  lock_record_t *toremove)
{
	do {
		if (lr->waiter == toremove) {
			lr->waiter = toremove->waiter;
			break;
		}
		lr = lr->waiter;
	} while (lr); /* XXX need to break cycle? */
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
	bool wasinterrupted;
	lock_record_t *newlr;
	lock_record_t *lr;

	lr = o->monitorPtr;
	GRAB_LR(lr, t);
	CHECK_MONITORSTATE(lr, t, o, return);

	/* { the thread t owns the lock record lr on the object o } */

	/* wake threads waiting on this record XXX why? */

	if (lr->waiter)
		lock_wake_waiters(lr->waiter);

	/* mark the lock record as "waiting on object o" */

	lr->waiting = o;
	STORE_ORDER_BARRIER();

	/* unlock this record */

	lock_record_release(lr);

	/* wait until notified/interrupted/timed out */

	wasinterrupted = threads_wait_with_timeout_relative(t, millis, nanos);

	/* re-enter the monitor */

	newlr = lock_monitor_enter(t, o);

	/* we are no longer waiting */

	lock_record_remove_waiter(newlr, lr);
	newlr->lockCount = lr->lockCount;

	/* recylce the old lock record */

	lr->lockCount = 1;
	lr->waiting = NULL;
	lr->waiter = NULL;
	lock_record_recycle(t, lr);

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
	lock_record_t *lr;
	lock_record_t *wlr;
	threadobject *wthread;

	lr = o->monitorPtr;
	GRAB_LR(lr, t);
	CHECK_MONITORSTATE(lr, t, o, return);

	/* { the thread t owns the lock record lr on the object o } */

	/* for each waiter: */

	for (wlr = lr->waiter; wlr; wlr = wlr->waiter) {

		/* signal the waiting thread */

		wthread = wlr->owner;
		pthread_mutex_lock(&wthread->waitLock);
		if (wthread->isSleeping)
			pthread_cond_signal(&wthread->waitCond);
		wthread->signaled = true;
		pthread_mutex_unlock(&wthread->waitLock);

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
	lock_record_t *lr;

	lr = o->monitorPtr;
	GRAB_LR(lr, t);

	return (lr->o == o) && (lr->owner == t);
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
