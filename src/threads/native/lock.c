/* src/threads/native/lock.c - lock implementation

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

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "threads/native/lock.h"
#include "threads/native/threads.h"

#include "toolbox/list.h"

#include "vm/global.h"
#include "vm/exceptions.h"
#include "vm/finalizer.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vmcore/options.h"

#if defined(ENABLE_STATISTICS)
# include "vmcore/statistics.h"
#endif

#if defined(ENABLE_VMLOG)
#include <vmlog_cacao.h>
#endif

/* arch.h must be here because it defines USE_FAKE_ATOMIC_INSTRUCTIONS */

#include "arch.h"

/* includes for atomic instructions: */

#if defined(USE_FAKE_ATOMIC_INSTRUCTIONS)
#include "threads/native/generic-primitives.h"
#else
#include "machine-instr.h"
#endif

#if defined(ENABLE_JVMTI)
#include "native/jvmti/cacaodbg.h"
#endif

#if defined(ENABLE_GC_BOEHM)
# include "mm/boehm-gc/include/gc.h"
#endif


/* debug **********************************************************************/

#if !defined(NDEBUG)
# define DEBUGLOCKS(format) \
    do { \
        if (opt_DebugLocks) { \
            log_println format; \
        } \
    } while (0)
#else
# define DEBUGLOCKS(format)
#endif


/******************************************************************************/
/* MACROS                                                                     */
/******************************************************************************/

/* number of lock records in the first pool allocated for a thread */
#define LOCK_INITIAL_LOCK_RECORDS 8

#define LOCK_INITIAL_HASHTABLE_SIZE  1613  /* a prime in the middle between 1024 and 2048 */

#define COMPARE_AND_SWAP_OLD_VALUE(address, oldvalue, newvalue) \
	((ptrint) compare_and_swap((long *)(address), (long)(oldvalue), (long)(newvalue)))

/* CAUTION: oldvalue is evaluated twice! */
#define COMPARE_AND_SWAP_SUCCEEDS(address, oldvalue, newvalue) \
	(compare_and_swap((long *)(address), (long)(oldvalue), (long)(newvalue)) == (long)(oldvalue))


/******************************************************************************/
/* MACROS FOR THE FLAT LOCK CONTENTION BIT                                    */
/******************************************************************************/

#define LOCK_SET_FLC_BIT(obj)    ((obj)->hdrflags |= HDRFLAG_FLC)
#define LOCK_CLEAR_FLC_BIT(obj)  ((obj)->hdrflags &= ~ HDRFLAG_FLC)
#define LOCK_TEST_FLC_BIT(obj)   ((obj)->hdrflags & HDRFLAG_FLC)


/******************************************************************************/
/* MACROS FOR THIN/FAT LOCKS                                                  */
/******************************************************************************/

/* We use a variant of the tasuki locks described in the paper
 *     
 *     Tamiya Onodera, Kiyokuni Kawachiya
 *     A Study of Locking Objects with Bimodal Fields
 *     Proceedings of the ACM OOPSLA '99, pp. 223-237
 *     1999
 *
 * The underlying thin locks are a variant of the thin locks described in
 * 
 *     Bacon, Konuru, Murthy, Serrano
 *     Thin Locks: Featherweight Synchronization for Java
 *	   Proceedings of the ACM Conference on Programming Language Design and 
 *	   Implementation (Montreal, Canada), SIGPLAN Notices volume 33, number 6,
 *	   June 1998
 *
 * In thin lock mode the lockword looks like this:
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

#define THIN_UNLOCKED          0

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
#define MAKE_FAT_LOCK(ptr)      ((uintptr_t) (ptr) | THIN_LOCK_SHAPE_BIT)

#define LOCK_WORD_WITHOUT_COUNT(lockword) ((lockword) & ~THIN_LOCK_COUNT_MASK)


/* global variables ***********************************************************/

/* hashtable mapping objects to lock records */
static lock_hashtable_t lock_hashtable;


/******************************************************************************/
/* PROTOTYPES                                                                 */
/******************************************************************************/

static void lock_hashtable_init(void);

static void lock_record_enter(threadobject *t, lock_record_t *lr);
static void lock_record_exit(threadobject *t, lock_record_t *lr);
static bool lock_record_wait(threadobject *t, lock_record_t *lr, s8 millis, s4 nanos);
static void lock_record_notify(threadobject *t, lock_record_t *lr, bool one);


/*============================================================================*/
/* INITIALIZATION OF DATA STRUCTURES                                          */
/*============================================================================*/


/* lock_init *******************************************************************

   Initialize global data for locking.

*******************************************************************************/

void lock_init(void)
{
	/* initialize lock hashtable */

	lock_hashtable_init();

#if defined(ENABLE_VMLOG)
	vmlog_cacao_init_lock();
#endif
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
	return (index << THIN_LOCK_TID_SHIFT) | THIN_UNLOCKED;
}


/* lock_record_new *************************************************************

   Allocate a lock record.

*******************************************************************************/

static lock_record_t *lock_record_new(void)
{
	lock_record_t *lr;

	/* allocate the data structure on the C heap */

	lr = NEW(lock_record_t);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_lock_record += sizeof(lock_record_t);
#endif

	/* initialize the members */

	lr->object  = NULL;
	lr->owner   = NULL;
	lr->count   = 0;
	lr->waiters = list_create(OFFSET(lock_waiter_t, linkage));

	/* initialize the mutex */

	pthread_mutex_init(&(lr->mutex), NULL);

	DEBUGLOCKS(("[lock_record_new   : lr=%p]", (void *) lr));

	return lr;
}


/* lock_record_free ************************************************************

   Free a lock record.

   IN:
       lr....lock record to free

*******************************************************************************/

static void lock_record_free(lock_record_t *lr)
{
	DEBUGLOCKS(("[lock_record_free  : lr=%p]", (void *) lr));

	/* Destroy the mutex. */

	pthread_mutex_destroy(&(lr->mutex));

	/* Free the waiters list. */

	list_free(lr->waiters);

	/* Free the data structure. */

	FREE(lr, lock_record_t);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_lock_record -= sizeof(lock_record_t);
#endif
}


/*============================================================================*/
/* HASHTABLE MAPPING OBJECTS TO LOCK RECORDS                                  */
/*============================================================================*/

/* lock_hashtable_init *********************************************************

   Initialize the global hashtable mapping objects to lock records.

*******************************************************************************/

static void lock_hashtable_init(void)
{
	pthread_mutex_init(&(lock_hashtable.mutex), NULL);

	lock_hashtable.size    = LOCK_INITIAL_HASHTABLE_SIZE;
	lock_hashtable.entries = 0;
	lock_hashtable.ptr     = MNEW(lock_record_t *, lock_hashtable.size);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_lock_hashtable += sizeof(lock_record_t *) * lock_hashtable.size;
#endif

	MZERO(lock_hashtable.ptr, lock_record_t *, lock_hashtable.size);
}


/* lock_hashtable_grow *********************************************************

   Grow the lock record hashtable to about twice its current size and
   rehash the entries.

*******************************************************************************/

/* must be called with hashtable mutex locked */
static void lock_hashtable_grow(void)
{
	u4 oldsize;
	u4 newsize;
	lock_record_t **oldtable;
	lock_record_t **newtable;
	lock_record_t *lr;
	lock_record_t *next;
	u4 i;
	u4 h;
	u4 newslot;

	/* allocate a new table */

	oldsize = lock_hashtable.size;
	newsize = oldsize*2 + 1; /* XXX should use prime numbers */

	DEBUGLOCKS(("growing lock hashtable to size %d", newsize));

	oldtable = lock_hashtable.ptr;
	newtable = MNEW(lock_record_t *, newsize);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_lock_hashtable += sizeof(lock_record_t *) * newsize;
#endif

	MZERO(newtable, lock_record_t *, newsize);

	/* rehash the entries */

	for (i = 0; i < oldsize; i++) {
		lr = oldtable[i];
		while (lr) {
			next = lr->hashlink;

			h = heap_hashcode(lr->object);
			newslot = h % newsize;

			lr->hashlink = newtable[newslot];
			newtable[newslot] = lr;

			lr = next;
		}
	}

	/* replace the old table */

	lock_hashtable.ptr  = newtable;
	lock_hashtable.size = newsize;

	MFREE(oldtable, lock_record_t *, oldsize);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_lock_hashtable -= sizeof(lock_record_t *) * oldsize;
#endif
}


/* lock_hashtable_get **********************************************************

   Find the lock record for the given object.  If it does not exists,
   yet, create it and enter it in the hashtable.

   IN:
	  o....the object to look up

   RETURN VALUE:
      the lock record to use for this object

*******************************************************************************/

#if defined(ENABLE_GC_BOEHM)
static void lock_record_finalizer(void *object, void *p);
#endif

static lock_record_t *lock_hashtable_get(java_object_t *o)
{
	uintptr_t      lockword;
	u4             slot;
	lock_record_t *lr;

	lockword = o->lockword;

	if (IS_FAT_LOCK(lockword))
		return GET_FAT_LOCK(lockword);

	/* lock the hashtable */

	pthread_mutex_lock(&(lock_hashtable.mutex));

	/* lookup the lock record in the hashtable */

	slot = heap_hashcode(o) % lock_hashtable.size;
	lr   = lock_hashtable.ptr[slot];

	for (; lr != NULL; lr = lr->hashlink) {
		if (lr->object == o) {
			pthread_mutex_unlock(&(lock_hashtable.mutex));
			return lr;
		}
	}

	/* not found, we must create a new one */

	lr = lock_record_new();

	lr->object = o;

#if defined(ENABLE_GC_BOEHM)
	/* register new finalizer to clean up the lock record */

	GC_REGISTER_FINALIZER(o, lock_record_finalizer, 0, 0, 0);
#endif

	/* enter it in the hashtable */

	lr->hashlink             = lock_hashtable.ptr[slot];
	lock_hashtable.ptr[slot] = lr;
	lock_hashtable.entries++;

	/* check whether the hash should grow */

	if (lock_hashtable.entries * 3 > lock_hashtable.size * 4) {
		lock_hashtable_grow();
	}

	/* unlock the hashtable */

	pthread_mutex_unlock(&(lock_hashtable.mutex));

	/* return the new lock record */

	return lr;
}


/* lock_hashtable_remove *******************************************************

   Remove the lock record for the given object from the hashtable.

   IN:
       o....the object to look up

*******************************************************************************/

static void lock_hashtable_remove(java_object_t *o)
{
	uintptr_t      lockword;
	lock_record_t *lr;
	u4             slot;
	lock_record_t *tmplr;

	/* lock the hashtable */

	pthread_mutex_lock(&(lock_hashtable.mutex));

	/* get lock record */

	lockword = o->lockword;

	assert(IS_FAT_LOCK(lockword));

	lr = GET_FAT_LOCK(lockword);

	/* remove the lock-record from the hashtable */

	slot  = heap_hashcode(o) % lock_hashtable.size;
	tmplr = lock_hashtable.ptr[slot];

	if (tmplr == lr) {
		/* special handling if it's the first in the chain */

		lock_hashtable.ptr[slot] = lr->hashlink;
	}
	else {
		for (; tmplr != NULL; tmplr = tmplr->hashlink) {
			if (tmplr->hashlink == lr) {
				tmplr->hashlink = lr->hashlink;
				break;
			}
		}

		assert(tmplr != NULL);
	}

	/* decrease entry count */

	lock_hashtable.entries--;

	/* unlock the hashtable */

	pthread_mutex_unlock(&(lock_hashtable.mutex));
}


/* lock_record_finalizer *******************************************************

   XXX Remove me for exact GC.

*******************************************************************************/

static void lock_record_finalizer(void *object, void *p)
{
	java_object_t *o;
	uintptr_t      lockword;
	lock_record_t *lr;

	o = (java_object_t *) object;

	/* check for a finalizer function */

	if (o->vftbl->class->finalizer != NULL)
		finalizer_run(object, p);

	/* remove the lock-record entry from the hashtable */

	lock_hashtable_remove(o);

	/* get lock record */

	lockword = o->lockword;

	assert(IS_FAT_LOCK(lockword));

	lr = GET_FAT_LOCK(lockword);

	/* now release the lock record */

	lock_record_free(lr);
}


/*============================================================================*/
/* OBJECT LOCK INITIALIZATION                                                 */
/*============================================================================*/


/* lock_init_object_lock *******************************************************

   Initialize the monitor pointer of the given object. The monitor gets
   initialized to an unlocked state.

*******************************************************************************/

void lock_init_object_lock(java_object_t *o)
{
	assert(o);

	o->lockword = THIN_UNLOCKED;
	LOCK_CLEAR_FLC_BIT(o);
}


/* lock_get_initial_lock_word **************************************************

   Returns the initial (unlocked) lock word. The pointer is
   required in the code generator to set up a virtual
   java_objectheader for code patch locking.

*******************************************************************************/

lock_record_t *lock_get_initial_lock_word(void)
{
	return (lock_record_t *) THIN_UNLOCKED;
}



/*============================================================================*/
/* LOCKING ALGORITHM                                                          */
/*============================================================================*/


/* lock_record_enter ***********************************************************

   Enter the lock represented by the given lock record.

   IN:
      t.................the current thread
	  lr................the lock record

*******************************************************************************/

static inline void lock_record_enter(threadobject *t, lock_record_t *lr)
{
	pthread_mutex_lock(&(lr->mutex));

	lr->owner = t;
}


/* lock_record_exit ************************************************************

   Release the lock represented by the given lock record.

   IN:
      t.................the current thread
	  lr................the lock record

   PRE-CONDITION:
      The current thread must own the lock represented by this lock record.
	  This is NOT checked by this function!

*******************************************************************************/

static inline void lock_record_exit(threadobject *t, lock_record_t *lr)
{
	lr->owner = NULL;
	pthread_mutex_unlock(&(lr->mutex));
}


/* lock_inflate ****************************************************************

   Inflate the lock of the given object. This may only be called by the
   owner of the monitor of the object.

   IN:
      t............the current thread
	  o............the object of which to inflate the lock
	  lr...........the lock record to install. The current thread must
	               own the lock of this lock record!

   PRE-CONDITION:
      The current thread must be the owner of this object's monitor AND
	  of the lock record's lock!

*******************************************************************************/

static void lock_inflate(threadobject *t, java_object_t *o, lock_record_t *lr)
{
	uintptr_t lockword;

	/* get the current lock count */

	lockword = o->lockword;

	if (IS_FAT_LOCK(lockword)) {
		assert(GET_FAT_LOCK(lockword) == lr);
	}
	else {
		assert(LOCK_WORD_WITHOUT_COUNT(lockword) == t->thinlock);

		/* copy the count from the thin lock */

		lr->count = (lockword & THIN_LOCK_COUNT_MASK) >> THIN_LOCK_COUNT_SHIFT;
	}

	DEBUGLOCKS(("[lock_inflate      : lr=%p, t=%p, o=%p, o->lockword=%lx, count=%d]",
				lr, t, o, o->lockword, lr->count));

	/* clear flat-lock-contention bit */

	LOCK_CLEAR_FLC_BIT(o);

	/* notify waiting objects */

	lock_record_notify(t, lr, false);

	/* install it */

	o->lockword = MAKE_FAT_LOCK(lr);
}


/* lock_monitor_enter **********************************************************

   Acquire the monitor of the given object. If the current thread already
   owns the monitor, the lock counter is simply increased.

   This function blocks until it can acquire the monitor.

   IN:
      t............the current thread
	  o............the object of which to enter the monitor

   RETURN VALUE:
      true.........the lock has been successfully acquired
	  false........an exception has been thrown

*******************************************************************************/

bool lock_monitor_enter(java_object_t *o)
{
	threadobject  *t;
	/* CAUTION: This code assumes that ptrint is unsigned! */
	ptrint         lockword;
	ptrint         thinlock;
	lock_record_t *lr;

	if (o == NULL) {
		exceptions_throw_nullpointerexception();
		return false;
	}

	t = THREADOBJECT;

	thinlock = t->thinlock;

	/* most common case: try to thin-lock an unlocked object */

	if ((lockword = COMPARE_AND_SWAP_OLD_VALUE(&(o->lockword), THIN_UNLOCKED, thinlock)) == THIN_UNLOCKED) {
		/* success. we locked it */
		/* The Java Memory Model requires a memory barrier here: */
		MEMORY_BARRIER();
		return true;
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

			o->lockword = lockword + THIN_LOCK_COUNT_INCR;

			/* success. we locked it */
			return true;
		}
		else {
			/* recursion count overflow */

			lr = lock_hashtable_get(o);
			lock_record_enter(t, lr);
			lock_inflate(t, o, lr);
			lr->count++;

			return true;
		}
	}

	/* the lock is either contented or fat */

	if (IS_FAT_LOCK(lockword)) {

		lr = GET_FAT_LOCK(lockword);

		/* check for recursive entering */
		if (lr->owner == t) {
			lr->count++;
			return true;
		}

		/* acquire the mutex of the lock record */

		lock_record_enter(t, lr);

		assert(lr->count == 0);

		return true;
	}

	/****** inflation path ******/

	/* first obtain the lock record for this object */

	lr = lock_hashtable_get(o);

#if defined(ENABLE_JVMTI)
	/* Monitor Contended Enter */
	jvmti_MonitorContendedEntering(false, o);
#endif

	/* enter the monitor */

	lock_record_enter(t, lr);

#if defined(ENABLE_JVMTI)
	/* Monitor Contended Entered */
	jvmti_MonitorContendedEntering(true, o);
#endif

	/* inflation loop */

	while (IS_THIN_LOCK(lockword = o->lockword)) {
		/* Set the flat lock contention bit to let the owning thread
		   know that we want to be notified of unlocking. */

		LOCK_SET_FLC_BIT(o);

		DEBUGLOCKS(("thread %d set flc bit on %p lr %p",
					t->index, (void*) o, (void*) lr));

		/* try to lock the object */

		if (COMPARE_AND_SWAP_SUCCEEDS(&(o->lockword), THIN_UNLOCKED, thinlock)) {
			/* we can inflate the lock ourselves */

			DEBUGLOCKS(("thread %d inflating lock of %p to lr %p",
						t->index, (void*) o, (void*) lr));

			lock_inflate(t, o, lr);
		}
		else {
			/* Wait until another thread sees the flc bit and notifies
			   us of unlocking. */

			(void) lock_record_wait(t, lr, 0, 0);
		}
	}

	/* we own the inflated lock now */

	return true;
}


/* lock_monitor_exit ***********************************************************

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

bool lock_monitor_exit(java_object_t *o)
{
	threadobject *t;
	uintptr_t     lockword;
	ptrint        thinlock;

	if (o == NULL) {
		exceptions_throw_nullpointerexception();
		return false;
	}

	t = THREADOBJECT;

	/* We don't have to worry about stale values here, as any stale value */
	/* will indicate that we don't own the lock.                          */

	lockword = o->lockword;
	thinlock = t->thinlock;

	/* most common case: we release a thin lock that we hold once */

	if (lockword == thinlock) {
		/* memory barrier for Java Memory Model */
		MEMORY_BARRIER();
		o->lockword = THIN_UNLOCKED;
		/* memory barrier for thin locking */
		MEMORY_BARRIER();

		/* check if there has been a flat lock contention on this object */

		if (LOCK_TEST_FLC_BIT(o)) {
			lock_record_t *lr;

			DEBUGLOCKS(("thread %d saw flc bit on %p",
						t->index, (void*) o));

			/* there has been a contention on this thin lock */

			lr = lock_hashtable_get(o);

			DEBUGLOCKS(("thread %d for %p got lr %p",
						t->index, (void*) o, (void*) lr));

			lock_record_enter(t, lr);

			if (LOCK_TEST_FLC_BIT(o)) {
				/* notify a thread that it can try to inflate the lock now */

				lock_record_notify(t, lr, true);
			}

			lock_record_exit(t, lr);
		}

		return true;
	}

	/* next common case: we release a recursive lock, count > 0 */

	if (LOCK_WORD_WITHOUT_COUNT(lockword) == thinlock) {
		o->lockword = lockword - THIN_LOCK_COUNT_INCR;
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
			exceptions_throw_illegalmonitorstateexception();
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

	exceptions_throw_illegalmonitorstateexception();

	return false;
}


/* lock_record_add_waiter ******************************************************

   Add a thread to the list of waiting threads of a lock record.

   IN:
      lr...........the lock record
      thread.......the thread to add

*******************************************************************************/

static void lock_record_add_waiter(lock_record_t *lr, threadobject *thread)
{
	lock_waiter_t *w;

	/* Allocate a waiter data structure. */

	w = NEW(lock_waiter_t);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_lock_waiter += sizeof(lock_waiter_t);
#endif

	/* Store the thread in the waiter structure. */

	w->thread = thread;

	/* Add the waiter as last entry to waiters list. */

	list_add_last(lr->waiters, w);
}


/* lock_record_remove_waiter ***************************************************

   Remove a thread from the list of waiting threads of a lock record.

   IN:
      lr...........the lock record
      t............the current thread

   PRE-CONDITION:
      The current thread must be the owner of the lock record.
   
*******************************************************************************/

static void lock_record_remove_waiter(lock_record_t *lr, threadobject *thread)
{
	list_t        *l;
	lock_waiter_t *w;

	/* Get the waiters list. */

	l = lr->waiters;

	for (w = list_first_unsynced(l); w != NULL; w = list_next_unsynced(l, w)) {
		if (w->thread == thread) {
			/* Remove the waiter entry from the list. */

			list_remove_unsynced(l, w);

			/* Free the waiter data structure. */

			FREE(w, lock_waiter_t);

#if defined(ENABLE_STATISTICS)
			if (opt_stat)
				size_lock_waiter -= sizeof(lock_waiter_t);
#endif

			return;
		}
	}

	/* This should never happen. */

	vm_abort("lock_record_remove_waiter: thread not found in list of waiters\n");
}


/* lock_record_wait ************************************************************

   Wait on a lock record for a given (maximum) amount of time.

   IN:
      t............the current thread
	  lr...........the lock record
	  millis.......milliseconds of timeout
	  nanos........nanoseconds of timeout

   RETURN VALUE:
      true.........we have been interrupted,
      false........everything ok

   PRE-CONDITION:
      The current thread must be the owner of the lock record.
	  This is NOT checked by this function!
   
*******************************************************************************/

static bool lock_record_wait(threadobject *thread, lock_record_t *lr, s8 millis, s4 nanos)
{
	s4   lockcount;
	bool wasinterrupted;

	DEBUGLOCKS(("[lock_record_wait  : lr=%p, t=%p, millis=%lld, nanos=%d]",
				lr, thread, millis, nanos));

	/* { the thread t owns the fat lock record lr on the object o } */

	/* register us as waiter for this object */

	lock_record_add_waiter(lr, thread);

	/* remember the old lock count */

	lockcount = lr->count;

	/* unlock this record */

	lr->count = 0;
	lock_record_exit(thread, lr);

	/* wait until notified/interrupted/timed out */

	wasinterrupted = threads_wait_with_timeout_relative(thread, millis, nanos);

	/* re-enter the monitor */

	lock_record_enter(thread, lr);

	/* remove us from the list of waiting threads */

	lock_record_remove_waiter(lr, thread);

	/* restore the old lock count */

	lr->count = lockcount;

	/* return if we have been interrupted */

	return wasinterrupted;
}


/* lock_monitor_wait ***********************************************************

   Wait on an object for a given (maximum) amount of time.

   IN:
      t............the current thread
	  o............the object
	  millis.......milliseconds of timeout
	  nanos........nanoseconds of timeout

   PRE-CONDITION:
      The current thread must be the owner of the object's monitor.
   
*******************************************************************************/

static void lock_monitor_wait(threadobject *t, java_object_t *o, s8 millis, s4 nanos)
{
	uintptr_t      lockword;
	lock_record_t *lr;

	lockword = o->lockword;

	/* check if we own this monitor */
	/* We don't have to worry about stale values here, as any stale value */
	/* will fail this check.                                              */

	if (IS_FAT_LOCK(lockword)) {

		lr = GET_FAT_LOCK(lockword);

		if (lr->owner != t) {
			exceptions_throw_illegalmonitorstateexception();
			return;
		}
	}
	else {
		/* it's a thin lock */

		if (LOCK_WORD_WITHOUT_COUNT(lockword) != t->thinlock) {
			exceptions_throw_illegalmonitorstateexception();
			return;
		}

		/* inflate this lock */

		lr = lock_hashtable_get(o);

		lock_record_enter(t, lr);
		lock_inflate(t, o, lr);
	}

	/* { the thread t owns the fat lock record lr on the object o } */

	if (lock_record_wait(t, lr, millis, nanos))
		exceptions_throw_interruptedexception();
}


/* lock_record_notify **********************************************************

   Notify one thread or all threads waiting on the given lock record.

   IN:
      t............the current thread
	  lr...........the lock record
	  one..........if true, only notify one thread

   PRE-CONDITION:
      The current thread must be the owner of the lock record.
	  This is NOT checked by this function!
   
*******************************************************************************/

static void lock_record_notify(threadobject *t, lock_record_t *lr, bool one)
{
	list_t        *l;
	lock_waiter_t *w;
	threadobject  *waitingthread;

	/* { the thread t owns the fat lock record lr on the object o } */

	/* Get the waiters list. */

	l = lr->waiters;

	for (w = list_first_unsynced(l); w != NULL; w = list_next_unsynced(l, w)) {
		/* signal the waiting thread */

		waitingthread = w->thread;

		/* If the thread was already signaled but hasn't removed
		   itself from the list yet, just ignore it. */

		if (waitingthread->signaled == true)
			continue;

		/* Enter the wait-mutex. */

		pthread_mutex_lock(&(waitingthread->waitmutex));

		DEBUGLOCKS(("[lock_record_notify: lr=%p, t=%p, waitingthread=%p, sleeping=%d, one=%d]",
					lr, t, waitingthread, waitingthread->sleeping, one));

		/* Signal the thread if it's sleeping. */

		if (waitingthread->sleeping)
			pthread_cond_signal(&(waitingthread->waitcond));

		/* Mark the thread as signaled. */

		waitingthread->signaled = true;

		/* Leave the wait-mutex. */

		pthread_mutex_unlock(&(waitingthread->waitmutex));

		/* if we should only wake one, we are done */

		if (one)
			break;
	}
}


/* lock_monitor_notify *********************************************************

   Notify one thread or all threads waiting on the given object.

   IN:
      t............the current thread
	  o............the object
	  one..........if true, only notify one thread

   PRE-CONDITION:
      The current thread must be the owner of the object's monitor.
   
*******************************************************************************/

static void lock_monitor_notify(threadobject *t, java_object_t *o, bool one)
{
	uintptr_t      lockword;
	lock_record_t *lr;

	lockword = o->lockword;

	/* check if we own this monitor */
	/* We don't have to worry about stale values here, as any stale value */
	/* will fail this check.                                              */

	if (IS_FAT_LOCK(lockword)) {

		lr = GET_FAT_LOCK(lockword);

		if (lr->owner != t) {
			exceptions_throw_illegalmonitorstateexception();
			return;
		}
	}
	else {
		/* it's a thin lock */

		if (LOCK_WORD_WITHOUT_COUNT(lockword) != t->thinlock) {
			exceptions_throw_illegalmonitorstateexception();
			return;
		}

		/* inflate this lock */

		lr = lock_hashtable_get(o);

		lock_record_enter(t, lr);
		lock_inflate(t, o, lr);
	}

	/* { the thread t owns the fat lock record lr on the object o } */

	lock_record_notify(t, lr, one);
}



/*============================================================================*/
/* INQUIRY FUNCIONS                                                           */
/*============================================================================*/


/* lock_is_held_by_current_thread **********************************************

   Return true if the current thread owns the monitor of the given object.

   IN:
	  o............the object

   RETURN VALUE:
      true, if the current thread holds the lock of this object.
   
*******************************************************************************/

bool lock_is_held_by_current_thread(java_object_t *o)
{
	threadobject  *t;
	uintptr_t      lockword;
	lock_record_t *lr;

	t = THREADOBJECT;

	/* check if we own this monitor */
	/* We don't have to worry about stale values here, as any stale value */
	/* will fail this check.                                              */

	lockword = o->lockword;

	if (IS_FAT_LOCK(lockword)) {
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

void lock_wait_for_object(java_object_t *o, s8 millis, s4 nanos)
{
	threadobject *thread;

	thread = THREADOBJECT;

	lock_monitor_wait(thread, o, millis, nanos);
}


/* lock_notify_object **********************************************************

   Notify one thread waiting on the given object.

   IN:
	  o............the object
   
*******************************************************************************/

void lock_notify_object(java_object_t *o)
{
	threadobject *thread;

	thread = THREADOBJECT;

	lock_monitor_notify(thread, o, true);
}


/* lock_notify_all_object ******************************************************

   Notify all threads waiting on the given object.

   IN:
	  o............the object
   
*******************************************************************************/

void lock_notify_all_object(java_object_t *o)
{
	threadobject *thread;

	thread = THREADOBJECT;

	lock_monitor_notify(thread, o, false);
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
