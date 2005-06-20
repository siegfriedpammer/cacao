/* src/threads/native/threads.h - native threads header

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Stefan Ring

   Changes: Christian Thalinger

   $Id: threads.h 2739 2005-06-20 09:53:53Z twisti $

*/


#ifndef _THREADS_H
#define _THREADS_H

#include <pthread.h>
#include <semaphore.h>

#include "mm/memory.h"
#include "native/jni.h"
#include "native/include/java_lang_Object.h" /* required by java/lang/VMThread*/
#include "native/include/java_lang_Thread.h"
#include "native/include/java_lang_VMThread.h"

#if defined(__DARWIN__)
#include <mach/mach.h>

/* We need to emulate recursive mutexes. */
#define MUTEXSIM
#endif


#if defined(HAVE___THREAD)

#define THREADSPECIFIC    __thread
#define THREADOBJECT      ((java_lang_VMThread*) threadobj)
#define THREADINFO        (&threadobj->info)

extern __thread threadobject *threadobj;

#else /* defined(HAVE___THREAD) */

#define THREADSPECIFIC
#define THREADOBJECT ((java_lang_VMThread*) pthread_getspecific(tkey_threadinfo))
#define THREADINFO (&((threadobject*) pthread_getspecific(tkey_threadinfo))->info)

extern pthread_key_t tkey_threadinfo;

#endif /* defined(HAVE___THREAD) */


/* typedefs *******************************************************************/

typedef struct ExecEnvironment ExecEnvironment;
typedef struct nativethread nativethread;
typedef struct threadobject threadobject;
typedef struct monitorLockRecord monitorLockRecord;
typedef struct lockRecordPoolHeader lockRecordPoolHeader;
typedef struct lockRecordPool lockRecordPool;
typedef java_lang_Thread thread;


/* ExecEnvironment *************************************************************

   Monitor lock implementation

*******************************************************************************/

struct ExecEnvironment {
	monitorLockRecord *firstLR;
	lockRecordPool    *lrpool;
	int                numlr;
};


struct nativethread {
	threadobject      *next;
	threadobject      *prev;
	java_objectheader *_exceptionptr;
	u1                 _dontfillinexceptionstacktrace;
	methodinfo        *_threadrootmethod;
	void              *_stackframeinfo;
	pthread_t          tid;
#if defined(__DARWIN__)
	mach_port_t        mach_thread;
#endif
	pthread_mutex_t    joinMutex;
	pthread_cond_t     joinCond;
};


/* threadobject ****************************************************************

   DOCUMENT ME!

*******************************************************************************/

struct threadobject {
	java_lang_VMThread  o;
	nativethread        info;
	ExecEnvironment     ee;

	pthread_mutex_t     waitLock;
	pthread_cond_t      waitCond;
	bool                interrupted;
	bool                signaled;
	bool                isSleeping;

	dumpinfo            dumpinfo;       /* dump memory info structure         */
};


struct monitorLockRecord {
	threadobject      *ownerThread;
	java_objectheader *o;
	int                lockCount;
	monitorLockRecord *nextFree;
	int                queuers;
	monitorLockRecord *waiter;
	monitorLockRecord *incharge;
	bool               waiting;
	sem_t              queueSem;
	pthread_mutex_t    resolveLock;
	pthread_cond_t     resolveWait;
};



struct lockRecordPoolHeader {
	lockRecordPool *next;
	int             size;
}; 

struct lockRecordPool {
	lockRecordPoolHeader header;
	monitorLockRecord    lr[1];
};


monitorLockRecord *monitorEnter(threadobject *, java_objectheader *);
bool monitorExit(threadobject *, java_objectheader *);

bool threadHoldsLock(threadobject *t, java_objectheader *o);
void signal_cond_for_object (java_objectheader *obj);
void broadcast_cond_for_object (java_objectheader *obj);
void wait_cond_for_object (java_objectheader *obj, s8 time, s4 nanos);

void initThreadsEarly();
void initThreads(u1 *stackbottom);
void initObjectLock(java_objectheader *);
monitorLockRecord *get_dummyLR(void);
void initLocks();
void initThread(java_lang_VMThread *);
void startThread(thread *t);
void joinAllThreads();

void sleepThread(s8 millis, s4 nanos);
void yieldThread();

void setPriorityThread(thread *t, s4 priority);

void interruptThread(java_lang_VMThread *);
bool interruptedThread();
bool isInterruptedThread(java_lang_VMThread *);

/* This must not be changed, it is used in asm_criticalsections */
typedef struct {
	u1 *mcodebegin;
	u1 *mcodeend;
	u1 *mcoderestart;
} threadcritnode;

void thread_registercritical(threadcritnode *);
u1 *thread_checkcritical(u1*);

extern volatile int stopworldwhere;

void cast_stopworld();
void cast_startworld();

/* dumps all threads */
void thread_dump(void);

#endif /* _THREADS_H */


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
 */
