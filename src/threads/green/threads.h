/* -*- mode: c; tab-width: 4; c-basic-offset: 4 -*- */
/*
 * thread.h
 * Thread support.
 *
 * Copyright (c) 1996 T. J. Wilkinson & Associates, London, UK.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Written by Tim Wilkinson <tim@tjwassoc.demon.co.uk>, 1996.
 */


#ifndef _THREADS_H
#define _THREADS_H

#include "config.h"
#include "mm/memory.h"
#include "vm/global.h"                                 /* for native includes */
#include "native/include/java_lang_ClassLoader.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Throwable.h"


#define	THREADCLASS         "java/lang/Thread"
#define	THREADGROUPCLASS    "java/lang/ThreadGroup"
#define	THREADDEATHCLASS    "java/lang/ThreadDeath"

#define	MIN_THREAD_PRIO		1
#define	NORM_THREAD_PRIO	5
#define	MAX_THREAD_PRIO		10

#define	THREAD_SUSPENDED	0
#define	THREAD_RUNNING		1
#define	THREAD_DEAD		2
#define	THREAD_KILL		3

#define	THREAD_FLAGS_GENERAL		0
#define	THREAD_FLAGS_NOSTACKALLOC	1
#define THREAD_FLAGS_USER_SUSPEND       2  /* Flag explicit suspend() call */
#define	THREAD_FLAGS_KILLED		4


#define MAXTHREADS              256          /* schani */

#if 1
#define DBG(s)
#define SDBG(s)
#else
#define DBG(s)                 s
#define SDBG(s)                s
#endif

struct _thread;

#if 0
typedef struct _ctx
{
    struct _thread    *thread;
    bool               free;
    u1                 status;
    u1                 priority;
    u1*                restorePoint;
    u1*                stackMem;           /* includes guard page */
    u1*                stackBase;
    u1*                stackEnd;
    u1*                usedStackTop;
    s8                 time;
    java_objectheader *texceptionptr;
    struct _thread    *nextlive;
    u1                 flags;
} ctx;
#endif

/*
struct _stringClass;
struct _object;
*/

/* This structure mirrors java.lang.ThreadGroup.h */

typedef struct _threadGroup {
   java_objectheader header;
   struct _threadGroup* parent;
   struct java_objectheader* name;
   struct java_objectheader* threads;
   struct java_objectheader* groups;
   s4 daemon_flag;
   s4 maxpri;
} threadGroup;


typedef struct thread thread;
typedef struct vmthread vmthread;

/* This structure mirrors java.lang.Thread.h */

struct thread {
   java_objectheader      header;
   vmthread              *vmThread;
   threadGroup           *group;
   struct java_lang_Runnable* runnable;
   java_lang_String      *name;
   s4                     daemon;
   s4                     priority;
   s8                     stacksize;
   java_lang_Throwable   *stillborn;
   java_lang_ClassLoader *contextClassLoader;
};


/* This structure mirrors java.lang.VMThread.h */

struct vmthread {
	java_objectheader  header;
	thread            *thread;
	s4                 running;
	s4                 status;
	s4                 priority;
	void              *restorePoint;
	void              *stackMem;
	void              *stackBase;
	void              *stackEnd;
	void              *usedStackTop;
	s8                 time;
	java_objectheader *texceptionptr;
	struct _thread    *nextlive;
	struct _thread    *next;
	s4                 flags;

#if 0
	dumpinfo          *dumpinfo;        /* dump memory info structure         */
#endif
};


extern int blockInts;
extern bool needReschedule;
extern thread *currentThread;
extern thread *mainThread;
/*extern ctx contexts[];*/
extern int threadStackSize;

extern thread *liveThreads;
extern thread *sleepThreads;

extern thread *threadQhead[MAX_THREAD_PRIO + 1];


/*#define CONTEXT(_t)     (contexts[(_t)->PrivateInfo - 1])*/
#define CONTEXT(_t)	(*(_t->vmThread))

#define	intsDisable()	blockInts++

#define	intsRestore()   if (blockInts == 1 && needReschedule) { \
                            reschedule(); \
                        } \
                        blockInts--


/* access macros */

#define	THREADSTACKSIZE         (32 * 1024)

#define	THREADSWITCH(to, from) \
    do { \
	void *from1; \
	void *from2; \
        asm_perform_threadswitch((from?&(from)->restorePoint:&from1),\
                                 &(to)->restorePoint, (from?&(from)->usedStackTop:&from2)); \
    } while (0)


#define THREADINIT(to, func) \
    do { \
        (to)->restorePoint = asm_initialize_thread_stack((u1*)(func), \
                                                         (to)->stackEnd); \
    } while (0)


#define	THREADINFO(e) \
    do { \
        (e)->restorePoint = 0; \
        (e)->flags = THREAD_FLAGS_NOSTACKALLOC; \
    } while(0)


/* function prototypes ********************************************************/

void initThreads (u1 *stackbottom);
void clear_thread_flags (void);
void startThread (thread*);
void resumeThread (thread*);
void iresumeThread (thread*);
void suspendThread (thread*);
void suspendOnQThread (thread*, thread**);
void yieldThread (void);
void killThread (thread*);
void setPriorityThread (thread*, int);

s8 currentTime (void);
void sleepThread(s8 millis, s4 nanos);
bool aliveThread (thread*);
long framesThread (thread*);

void reschedule (void);

void checkEvents (bool block);

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
