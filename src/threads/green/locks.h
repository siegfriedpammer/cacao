/*
 * locks.h
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

#ifndef __locks_h
#define __locks_h

#ifdef USE_THREADS

#include "../global.h"
#include "sysdep/defines.h"

#define	WAITFOREVER	-1

#if defined(USE_INTERNAL_THREADS)

struct _thread;

typedef struct _iMux {
    struct _thread *holder;
    int	count;
    struct _thread *muxWaiters;
} iMux;

typedef struct _iCv {
	struct _thread*		cvWaiters;
	struct _iMux*		mux;
} iCv;

#define MAX_MUTEXES             256

typedef struct _mutexHashEntry
{
    java_objectheader *object;
    iMux mutex;
    struct _mutexHashEntry *next;
    int conditionCount;
} mutexHashEntry;

#define MUTEX_HASH_TRASH_BITS                3
#define MUTEX_HASH_SIGN_BITS                10

#define MUTEX_HASH_TABLE_SIZE             1024
#define MUTEX_OVERFLOW_TABLE_SIZE         1024
/*
#define MAX_MUTEX_HASH_TABLE_SIZE        65536
*/

/*
#define MUTEX_USE_THRESHOLD               1024
*/

extern long mutexHashMask;
extern int mutexHashTableSize;
extern mutexHashEntry *mutexHashTable;

extern mutexHashEntry *mutexOverflowTable;
extern int mutexOverflowTableSize;
extern mutexHashEntry *firstFreeOverflowEntry;

#define MUTEX_HASH_MASK                  ((MUTEX_HASH_TABLE_SIZE - 1) << 3)

#if 0
#define MUTEX_HASH_VALUE(a)      ((((long)(a)) & MUTEX_HASH_MASK) >> MUTEX_HASH_TRASH_BITS)
#else
#define MUTEX_HASH_VALUE(a)      (( (((long)(a)) ^ ((long)(a) >> MUTEX_HASH_SIGN_BITS)) & mutexHashMask) >> MUTEX_HASH_TRASH_BITS)
#endif
#define MUTEX_HASH_SUCCESSOR(h)                  (((h) + 7) & (mutexHashTableSize - 1))

typedef struct _conditionHashEntry
{
    java_objectheader *object;
    iCv condition;
} conditionHashEntry;

#define CONDITION_HASH_TABLE_SIZE                1024

#define CONDITION_HASH_VALUE(a)                  ((((long)(a)) & conditionHashMask) >> 3)
#define CONDITION_HASH_SUCCESSOR(h)              (((h) + 7) & (conditionHashTableSize - 1))

typedef struct
{
    bool free;
    java_objectheader *object;
    iMux mutex;
    iCv condition;
} object_mutex;

extern void initLocks (void);

mutexHashEntry* conditionLockedMutexForObject (java_objectheader *object);

void reorderConditionHashTable (int begin);
iCv* conditionForObject (java_objectheader *object);
iCv* addConditionForObject (java_objectheader *object);
void removeConditionForObject (java_objectheader *object);

/*
 * use these functions only outside critical sections (intsEnable/intsRestore).
 */

void signal_cond_for_object (java_objectheader *obj);
void broadcast_cond_for_object (java_objectheader *obj);
void wait_cond_for_object (java_objectheader *obj, s8 time);
void lock_mutex_for_object (java_objectheader *obj);
void unlock_mutex_for_object (java_objectheader *obj);

void lock_mutex (iMux*);
void unlock_mutex (iMux*);
void wait_cond (iMux*, iCv*, s8);
void signal_cond (iCv*);
void broadcast_cond (iCv*);

/*
 * use these internal functions only in critical sections. blockInts must be exactly
 * 1.
 */
void internal_lock_mutex (iMux*);
void internal_unlock_mutex (iMux*);
void internal_wait_cond (iMux*, iCv*, s8);
void internal_signal_cond (iCv*);
void internal_broadcast_cond (iCv*);

void internal_lock_mutex_for_object (java_objectheader *obj);
void internal_unlock_mutex_for_object (java_objectheader *obj);
void internal_broadcast_cond_for_object (java_objectheader *obj);

#endif

#endif /* USE_THREADS */

#endif /* __locks_h */
