#ifndef _NATIVETHREAD_H
#define _NATIVETHREAD_H

#include <semaphore.h>

#include "jni.h"
#include "nat/java_lang_Object.h"
#include "nat/java_lang_Throwable.h"
#include "nat/java_lang_Thread.h"
#include "nat/java_lang_VMThread.h"
#include "toolbox/memory.h"

#if defined(__DARWIN__)
#include <mach/mach.h>

/* We need to emulate recursive mutexes. */
#define MUTEXSIM
#endif

struct _threadobject;


typedef struct monitorLockRecord monitorLockRecord;

struct monitorLockRecord {
	struct _threadobject *ownerThread;
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


struct _lockRecordPool;

typedef struct {
	struct _lockRecordPool *next;
	int size;
} lockRecordPoolHeader; 

typedef struct _lockRecordPool {
	lockRecordPoolHeader header;
	monitorLockRecord lr[1];
} lockRecordPool;

/* Monitor lock implementation */
typedef struct {
	monitorLockRecord *firstLR;
	lockRecordPool *lrpool;
	int numlr;
} ExecEnvironment;

typedef struct {
	struct _threadobject *next, *prev;
	java_objectheader *_exceptionptr;
	methodinfo *_threadrootmethod;
	void *_stackframeinfo;
	pthread_t tid;
#if defined(__DARWIN__)
	mach_port_t mach_thread;
#endif
	pthread_mutex_t joinMutex;
	pthread_cond_t joinCond;
} nativethread;

typedef java_lang_Thread thread;


/* threadobject ****************************************************************

   TODO

*******************************************************************************/

typedef struct _threadobject {
	java_lang_VMThread  o;
	nativethread        info;
	ExecEnvironment     ee;

	pthread_mutex_t     waitLock;
	pthread_cond_t      waitCond;
	bool                interrupted;
	bool                signaled;
	bool                isSleeping;

	dumpinfo            dumpinfo;       /* dump memory info structure         */
} threadobject;


monitorLockRecord *monitorEnter(threadobject *, java_objectheader *);
bool monitorExit(threadobject *, java_objectheader *);

bool threadHoldsLock(threadobject *t, java_objectheader *o);
void signal_cond_for_object (java_objectheader *obj);
void broadcast_cond_for_object (java_objectheader *obj);
void wait_cond_for_object (java_objectheader *obj, s8 time, s4 nanos);

void initThreadsEarly();
void initThreads(u1 *stackbottom);
void initObjectLock(java_objectheader *);
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

#if !defined(HAVE___THREAD)
extern pthread_key_t tkey_threadinfo;
#define THREADOBJECT ((java_lang_VMThread*) pthread_getspecific(tkey_threadinfo))
#define THREADINFO (&((threadobject*) pthread_getspecific(tkey_threadinfo))->info)
#else
extern __thread threadobject *threadobj;
#define THREADOBJECT ((java_lang_VMThread*) threadobj)
#define THREADINFO (&threadobj->info)
#endif

#include "builtin.h"

/* This must not be changed, it is used in asm_criticalsections */
typedef struct {
	u1 *mcodebegin, *mcodeend, *mcoderestart;
} threadcritnode;

void thread_registercritical(threadcritnode *);
u1 *thread_checkcritical(u1*);

extern volatile int stopworldwhere;

void cast_stopworld();
void cast_startworld();

#endif /* _NATIVETHREAD_H */


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
