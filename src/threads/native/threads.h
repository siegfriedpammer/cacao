#ifndef _NATIVETHREAD_H
#define _NATIVETHREAD_H

#include "jni.h"
#include "nat/java_lang_Object.h"
#include "nat/java_lang_Throwable.h"
#include "nat/java_lang_Thread.h"

#if defined(__DARWIN__)
#include <mach/mach.h>

/* We need to emulate recursive mutexes. */
#define MUTEXSIM
#endif

struct _threadobject;

typedef struct monitorLockRecord {
	struct _threadobject *owner;
	int lockCount;
	long storedBits;
	struct monitorLockRecord *queue;
	struct monitorLockRecord *nextFree;
} monitorLockRecord;

struct _lockRecordPool;

typedef struct {
	struct _lockRecordPool *next;
	int size;
} lockRecordPoolHeader; 

typedef struct _lockRecordPool {
	lockRecordPoolHeader header;
	monitorLockRecord lr[1];
} lockRecordPool;

#define INITIALLOCKRECORDS 8

/* Monitor lock implementation */
typedef struct {
	pthread_mutex_t metaLockMutex;
	pthread_cond_t metaLockCond;
	bool gotMetaLockSlow;
	bool bitsForGrab;
	long metaLockBits;
	struct _threadobject *succ;
	pthread_mutex_t monitorLockMutex;
	pthread_cond_t monitorLockCond;
	bool isWaitingForNotify;

	monitorLockRecord *firstLR;
	monitorLockRecord lr[INITIALLOCKRECORDS];
	lockRecordPool *lrpool;
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

typedef struct _threadobject {
	java_lang_Thread o;
	nativethread info;
	ExecEnvironment ee;
} threadobject, thread;

void monitorEnter(threadobject *, java_objectheader *);
void monitorExit(threadobject *, java_objectheader *);

void signal_cond_for_object (java_objectheader *obj);
void broadcast_cond_for_object (java_objectheader *obj);
void wait_cond_for_object (java_objectheader *obj, s8 time);

void initThreadsEarly();
void initThreads(u1 *stackbottom);
void initLocks();
void initThread(java_lang_Thread *);
void joinAllThreads();

bool aliveThread(java_lang_Thread *);
void sleepThread (s8);
void yieldThread();

#if !defined(HAVE___THREAD)
extern pthread_key_t tkey_threadinfo;
#define THREADOBJECT ((java_lang_Thread*) pthread_getspecific(tkey_threadinfo))
#define THREADINFO (&((threadobject*) pthread_getspecific(tkey_threadinfo))->info)
#else
extern __thread threadobject *threadobj;
#define THREADOBJECT ((java_lang_Thread*) threadobj)
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

