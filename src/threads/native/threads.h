/* src/threads/native/threads.h - native threads header

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

   $Id: threads.h 4854 2006-04-27 23:03:37Z twisti $

*/


#ifndef _THREADS_H
#define _THREADS_H

#include "config.h"

#include <pthread.h>
#include <semaphore.h>
#include <ucontext.h>

#include "vm/types.h"

#include "config.h"
#include "vm/types.h"

#include "mm/memory.h"
#include "native/jni.h"
#include "native/include/java_lang_Object.h" /* required by java/lang/VMThread*/
#include "native/include/java_lang_Thread.h"
#include "native/include/java_lang_VMThread.h"
#include "vm/global.h"

#if defined(__DARWIN__)
#include <mach/mach.h>

/* We need to emulate recursive mutexes. */
#define MUTEXSIM
#endif


#if defined(HAVE___THREAD)

#define THREADSPECIFIC    __thread
#define THREADOBJECT      threadobj
#define THREADINFO        (&threadobj->info)

extern __thread threadobject *threadobj;

#else /* defined(HAVE___THREAD) */

#define THREADSPECIFIC
#define THREADOBJECT      pthread_getspecific(tkey_threadinfo)
#define THREADINFO        (&((threadobject*) pthread_getspecific(tkey_threadinfo))->info)

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
	stackframeinfo    *_stackframeinfo;
	localref_table    *_localref_table; /* JNI local references               */
#if defined(ENABLE_INTRP)
	void              *_global_sp;
#endif
	pthread_t          tid;
#if defined(__DARWIN__)
	mach_port_t        mach_thread;
#endif
	pthread_mutex_t    joinMutex;
	pthread_cond_t     joinCond;
};


/* threadobject ****************************************************************

   Every java.lang.VMThread object is actually an instance of this
   structure.

*******************************************************************************/

struct threadobject {
	java_lang_VMThread  o;
	nativethread        info;           /* some general pthreads stuff        */
	ExecEnvironment     ee;             /* contains our lock record pool      */

	/* these are used for the wait/notify implementation                      */
	pthread_mutex_t     waitLock;
	pthread_cond_t      waitCond;
	bool                interrupted;
	bool                signaled;
	bool                isSleeping;

	dumpinfo            dumpinfo;       /* dump memory info structure         */
};

/* monitorLockRecord ***********************************************************

   This is the really interesting stuff.
   See handbook for a detailed description.

*******************************************************************************/

struct monitorLockRecord {
	threadobject      *ownerThread;
	java_objectheader *o;
	s4                 lockCount;
	monitorLockRecord *nextFree;
	s4                 queuers;
	monitorLockRecord *waiter;
	monitorLockRecord *incharge;
	java_objectheader *waiting;
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

void wait_cond_for_object(java_objectheader *o, s8 millis, s4 nanos);
void signal_cond_for_object(java_objectheader *o);
void broadcast_cond_for_object(java_objectheader *o);

void *thread_getself(void);

void threads_preinit(void);
bool threads_init(u1 *stackbottom);

void initObjectLock(java_objectheader *);
monitorLockRecord *get_dummyLR(void);
void initLocks();
void initThread(java_lang_VMThread *);

/* start a thread */
void threads_start_thread(thread *t, functionptr function);

void joinAllThreads();

void thread_sleep(s8 millis, s4 nanos);
void yieldThread();

void setPriorityThread(thread *t, s4 priority);

void interruptThread(java_lang_VMThread *);
bool interruptedThread();
bool isInterruptedThread(java_lang_VMThread *);

#if defined(ENABLE_JVMTI)
void setthreadobject(threadobject *thread);
#endif


/* This must not be changed, it is used in asm_criticalsections */
typedef struct {
	u1 *mcodebegin;
	u1 *mcodeend;
	u1 *mcoderestart;
} threadcritnode;

void thread_registercritical(threadcritnode *);
u1 *thread_checkcritical(u1*);

extern volatile int stopworldwhere;
extern threadobject *mainthreadobj;

extern pthread_mutex_t pool_lock;
extern lockRecordPool *global_pool;


void cast_stopworld();
void cast_startworld();

/* dumps all threads */
void threads_dump(void);

/* this is a machine dependent functions (src/vm/jit/$(ARCH_DIR)/md.c) */
void thread_restartcriticalsection(ucontext_t *);

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
