/* -*- mode: c; tab-width: 4; c-basic-offset: 4 -*- */
/*
 * locks.c
 * Manage locking system
 * This include the mutex's and cv's.
 *
 * Copyright (c) 1996 T. J. Wilkinson & Associates, London, UK.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Written by Tim Wilkinson <tim@tjwassoc.demon.co.uk>, 1996.
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>

#include "thread.h"
#include "locks.h"

#include "tables.h"
#include "native.h"
#include "loader.h"

static classinfo *class_java_lang_IllegalMonitorStateException;

extern thread* currentThread;

mutexHashEntry *mutexHashTable;
int mutexHashTableSize;
long mutexHashMask;

mutexHashEntry *mutexOverflowTable;
int mutexOverflowTableSize;
mutexHashEntry *firstFreeOverflowEntry = 0;

conditionHashEntry *conditionHashTable;
int conditionHashTableSize;
long conditionHashMask;

/*
 * Init the tables.
 */
void
initLocks (void)
{
    int i;

    mutexHashTableSize = MUTEX_HASH_TABLE_SIZE;
    mutexHashTable = (mutexHashEntry*)malloc(sizeof(mutexHashEntry) * mutexHashTableSize);
    mutexHashMask = (mutexHashTableSize - 1) << 3;

    for (i = 0; i < mutexHashTableSize; ++i)
    {
		mutexHashTable[i].object = 0;
		mutexHashTable[i].mutex.holder = 0;
		mutexHashTable[i].mutex.count = 0;
		mutexHashTable[i].mutex.muxWaiters = 0;
		mutexHashTable[i].conditionCount = 0;
		mutexHashTable[i].next = 0;
    }

    mutexOverflowTableSize = MUTEX_OVERFLOW_TABLE_SIZE;
    mutexOverflowTable = (mutexHashEntry*)malloc(sizeof(mutexHashEntry)
												 * mutexOverflowTableSize);

    firstFreeOverflowEntry = &mutexOverflowTable[0];

    for (i = 0; i < mutexOverflowTableSize; ++i)
    {
		mutexOverflowTable[i].object = 0;
		mutexOverflowTable[i].mutex.holder = 0;
		mutexOverflowTable[i].mutex.count = 0;
		mutexOverflowTable[i].mutex.muxWaiters = 0;
		mutexOverflowTable[i].conditionCount = 0;
		mutexOverflowTable[i].next = &mutexOverflowTable[i + 1];
    }
    mutexOverflowTable[i - 1].next = 0;

    conditionHashTableSize = CONDITION_HASH_TABLE_SIZE;
    conditionHashTable = (conditionHashEntry*)malloc(sizeof(conditionHashEntry)
													 * conditionHashTableSize);
    conditionHashMask = (conditionHashTableSize - 1) << 3;

    for (i = 0; i < conditionHashTableSize; ++i)
    {
		conditionHashTable[i].object = 0;
		conditionHashTable[i].condition.cvWaiters = 0;
		conditionHashTable[i].condition.mux = 0;
    }

	/* Load exception classes */
	class_java_lang_IllegalMonitorStateException =
		loader_load(utf_new_char("java/lang/IllegalMonitorStateException"));
}

/*
 * Reorders part of the condition hash table. Must be called after an entry has been deleted.
 */
void
reorderConditionHashTable (int begin)
{
    while (conditionHashTable[begin].object != 0)
    {
	int hashValue = CONDITION_HASH_VALUE(conditionHashTable[begin].object);

	if (hashValue != begin)
	{
	    while (conditionHashTable[hashValue].object != 0)
	    {
		hashValue = CONDITION_HASH_SUCCESSOR(hashValue);
		if (hashValue == begin)
		    break;
	    }
	    if (hashValue != begin)
	    {
		conditionHashTable[hashValue] = conditionHashTable[begin];
		conditionHashTable[begin].object = 0;
		conditionHashTable[begin].condition.cvWaiters = 0;
		conditionHashTable[begin].condition.mux = 0;
	    }
	}

	begin = CONDITION_HASH_SUCCESSOR(begin);
    }
}

/*
 * Looks up an entry in the condition hash table.
 */
iCv*
conditionForObject (java_objectheader *object)
{
    int hashValue;

    intsDisable();

    hashValue = CONDITION_HASH_VALUE(object);
    while (conditionHashTable[hashValue].object != object
		   && conditionHashTable[hashValue].object != 0)
		hashValue = CONDITION_HASH_SUCCESSOR(hashValue);

    if (conditionHashTable[hashValue].object == 0)
    {
		intsRestore();
		return 0;
    }
    
    intsRestore();
    return &conditionHashTable[hashValue].condition;
}

/*
 * Adds a new entry in the condition hash table and returns a pointer to the condition
 */
iCv*
addConditionForObject (java_objectheader *object)
{
    int hashValue;

    intsDisable();

    hashValue = CONDITION_HASH_VALUE(object);
    while (conditionHashTable[hashValue].object != 0)
		hashValue = CONDITION_HASH_SUCCESSOR(hashValue);

    conditionHashTable[hashValue].object = object;

    intsRestore();

    return &conditionHashTable[hashValue].condition;
}

/*
 * Removes an entry from the condition hash table.
 */
void
removeConditionForObject (java_objectheader *object)
{
    int hashValue;

    intsDisable();

    hashValue = CONDITION_HASH_VALUE(object);
    while (conditionHashTable[hashValue].object != object)
		hashValue = CONDITION_HASH_SUCCESSOR(hashValue);

    conditionHashTable[hashValue].object = 0;
    conditionHashTable[hashValue].condition.cvWaiters = 0;
    conditionHashTable[hashValue].condition.mux = 0;

    reorderConditionHashTable(CONDITION_HASH_SUCCESSOR(hashValue));

    intsRestore();
}

/*
 * Returns the mutex entry for the specified object and increments its conditionCount.
 */
mutexHashEntry*
conditionLockedMutexForObject (java_objectheader *object)
{
    int hashValue;
    mutexHashEntry *entry;

    assert(object != 0);

    intsDisable();

    hashValue = MUTEX_HASH_VALUE(object);
    entry = &mutexHashTable[hashValue];

    if (entry->object != 0)
    {
		if (entry->mutex.count == 0 && entry->conditionCount == 0)
		{
			entry->object = 0;
			entry->mutex.holder = 0;
			entry->mutex.count = 0;
			entry->mutex.muxWaiters = 0;
		}
		else
		{
			while (entry->next != 0 && entry->object != object)
				entry = entry->next;

			if (entry->object != object)
			{
				entry->next = firstFreeOverflowEntry;
				firstFreeOverflowEntry = firstFreeOverflowEntry->next;
				
				entry = entry->next;
				entry->object = 0;
				entry->next = 0;
				assert(entry->conditionCount == 0);
			}
		}
    }

    if (entry->object == 0)
        entry->object = object;

    ++entry->conditionCount;

    intsRestore();

    return entry;
}

/*
 * Wait for the condition of an object to be signalled
 */
void
wait_cond_for_object (java_objectheader *obj, s8 time)
{
    iCv *condition;
    mutexHashEntry *mutexEntry;

    intsDisable();

    mutexEntry = conditionLockedMutexForObject(obj);

    condition = conditionForObject(obj);
    if (condition == 0)
		condition = addConditionForObject(obj);

    DBG( fprintf(stderr, "condition of %p is %p\n", obj, condition); );

    internal_wait_cond(&mutexEntry->mutex, condition, time);

    if (condition->cvWaiters == 0 && condition->mux == 0)
		removeConditionForObject(obj);
    --mutexEntry->conditionCount;

    intsRestore();
}

/*
 * Signal the condition of an object
 */
void
signal_cond_for_object (java_objectheader *obj)
{
    iCv *condition;

    intsDisable();

    condition = conditionForObject(obj);
    if (condition == 0)
		condition = addConditionForObject(obj);

    DBG( fprintf(stderr, "condition of %p is %p\n", obj, condition); );
    
    internal_signal_cond(condition);

    if (condition->cvWaiters == 0 && condition->mux == 0)
		removeConditionForObject(obj);

    intsRestore();
}

/*
 * Broadcast the condition of an object.
 */
void
broadcast_cond_for_object (java_objectheader *obj)
{
	intsDisable();
	internal_broadcast_cond_for_object(obj);
	intsRestore();
}

/*
 * Internal: Broadcast the condition of an object.
 */
void
internal_broadcast_cond_for_object (java_objectheader *obj)
{
    iCv *condition;

    condition = conditionForObject(obj);
    if (condition == 0)
		condition = addConditionForObject(obj);

    internal_broadcast_cond(condition);

    if (condition->cvWaiters == 0 && condition->mux == 0)
		removeConditionForObject(obj);
}

/*
 * Lock a mutex.
 */
void
lock_mutex (iMux *mux)
{
	intsDisable();
	internal_lock_mutex(mux);
	intsRestore();
}

/*
 * Lock the mutex for an object.
 */
void
lock_mutex_for_object (java_objectheader *obj)
{
	intsDisable();
	internal_lock_mutex_for_object(obj);
	intsRestore();
}

/*
 * Unlock a mutex.
 */
void
unlock_mutex (iMux *mux)
{
	intsDisable();
	internal_unlock_mutex(mux);
	intsRestore();
}

/*
 * Unlock the mutex for an object.
 */
void
unlock_mutex_for_object (java_objectheader *obj)
{
	intsDisable();
	internal_unlock_mutex_for_object(obj);
	intsRestore();
}

/*
 * Wait on a condition variable.
 */
void
wait_cond (iMux *mux, iCv *cond, s8 timeout)
{
	intsDisable();
	internal_wait_cond(mux, cond, timeout);
	intsRestore();
}

/*
 * Signal a condition variable.
 */
void
signal_cond (iCv *cond)
{
	intsDisable();
	internal_signal_cond(cond);
	intsRestore();
}

/*
 * Broadcast a condition variable.
 */
void
broadcast_cond (iCv *cond)
{
	intsDisable();
	internal_broadcast_cond(cond);
	intsRestore();
}

/*
 * Internal: Lock a mutex.
 */
void
internal_lock_mutex(iMux* mux)
{
	assert(blockInts > 0);

    if (mux->holder == 0)
    {
		mux->holder = currentThread;
		mux->count = 1;
		DBG( fprintf(stderr, "set holder of %p to %p\n", mux, mux->holder); )
    }
    else if (mux->holder == currentThread)
    {
		mux->count++;
    }
    else
    {
		while (mux->holder != 0)
		{
			suspendOnQThread(currentThread, &mux->muxWaiters);
		}
		mux->holder = currentThread;
		mux->count = 1;
    }
}

/*
 * Internal: Release a mutex.
 */
void
internal_unlock_mutex(iMux* mux)
{
    thread* tid;

	assert(blockInts > 0);

    assert(mux->holder == currentThread);
    
    mux->count--;
    if (mux->count == 0)
    {
		mux->holder = 0;
		if (mux->muxWaiters != 0)
		{
			tid = mux->muxWaiters;
			mux->muxWaiters = tid->next;
			iresumeThread(tid);
		}
    }
}

/*
 * Internal: Wait on a conditional variable.
 *  (timeout currently ignored)
 */
void
internal_wait_cond(iMux* mux, iCv* cv, s8 timeout)
{
    int count;
    thread* tid;

    DBG( fprintf(stderr, "waiting on %p\n", cv); );

    if (mux->holder != currentThread) {
		exceptionptr = native_new_and_init(class_java_lang_IllegalMonitorStateException);
    }

	assert(blockInts > 0);

    count = mux->count;
    mux->holder = 0;
    mux->count = 0;
    cv->mux = mux;

    /* If there's anyone waiting here, wake them up */
    if (mux->muxWaiters != 0) {
		tid = mux->muxWaiters;
		mux->muxWaiters = tid->next;
		iresumeThread(tid);
    }

    /* Suspend, and keep suspended until I re-get the lock */
    suspendOnQThread(currentThread, &cv->cvWaiters);
    while (mux->holder != 0) {
		DBG( fprintf(stderr, "woke up\n"); );
		suspendOnQThread(currentThread, &mux->muxWaiters);
    }

    mux->holder = currentThread;
    mux->count = count;
}

/*
 * Internal: Wake one thread on a conditional variable.
 */
void
internal_signal_cond (iCv* cv)
{
    thread* tid;

    DBG( fprintf(stderr, "signalling on %p\n", cv); );

    /* If 'mux' isn't set then we've never waited on this object. */
    if (cv->mux == 0) {
		return;
    }

    if (cv->mux->holder != currentThread) {
		exceptionptr = native_new_and_init(class_java_lang_IllegalMonitorStateException);
    }

	assert(blockInts > 0);

    /* Remove one thread from cv list */
    if (cv->cvWaiters != 0) {
		DBG( fprintf(stderr, "releasing a waiter\n"); );

		tid = cv->cvWaiters;
		cv->cvWaiters = tid->next;

		/* Place it on mux list */
		tid->next = cv->mux->muxWaiters;
		cv->mux->muxWaiters = tid;
    }
}

/*
 * Internal: Wake all threads on a conditional variable.
 */
void
internal_broadcast_cond (iCv* cv)
{
    thread** tidp;

    /* If 'mux' isn't set then we've never waited on this object. */
    if (cv->mux == 0) {
		return;
    }

    if (cv->mux->holder != currentThread) {
		exceptionptr = native_new_and_init(class_java_lang_IllegalMonitorStateException);
    }

	assert(blockInts > 0);

    /* Find the end of the cv list */
    if (cv->cvWaiters) {
		for (tidp = &cv->cvWaiters; *tidp != 0; tidp = &(*tidp)->next)
			;

		/* Place entire cv list on mux list */
		(*tidp) = cv->mux->muxWaiters;
		cv->mux->muxWaiters = cv->cvWaiters;
		cv->cvWaiters = 0;
    }
}
