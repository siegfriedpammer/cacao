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

#ifndef __thread_h
#define __thread_h

#ifdef USE_THREADS

#include "../global.h"

#define MAXTHREADS              256          /* schani */

#define	THREADCLASS		"java/lang/Thread"
#define	THREADGROUPCLASS	"java/lang/ThreadGroup"
#define	THREADDEATHCLASS	"java/lang/ThreadDeath"

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

struct _thread;

typedef struct _ctx
{
    bool               free;           /* schani */
    u1                 status;
    u1                 priority;
    u1*                restorePoint;
    u1*                stackMem;           /* includes guard page */
    u1*                stackBase;
    u1*                stackEnd;
    u1*                usedStackTop;
    s8                 time;
    java_objectheader *exceptionptr;
    struct _thread    *nextlive;
    u1                 flags;
} ctx;

/*
struct _stringClass;
struct _object;
*/

/* This structure mirrors java.lang.ThreadGroup.h */

typedef struct _threadGroup
{
    java_objectheader    header;
    struct _threadGroup* parent;
    java_objectheader*   name;
    s4                   maxPrio;
    s4                   destroyed;
    s4                   daemon;
    s4                   nthreads;
    java_objectheader*   threads;
    s4                   ngroups;
    java_objectheader*   groups;
} threadGroup;

/* This structure mirrors java.lang.Thread.h */
typedef struct _thread
{
    java_objectheader        header;
    java_objectheader*       name;
    s4                       priority;
    struct _thread*          next;
    s8                       PrivateInfo;
    struct java_lang_Object* eetop;           /* ??? */
    s4                       single_step;
    s4                       daemon;
    s4                       stillborn;
    java_objectheader*       target;
    s4                       interruptRequested;
    threadGroup*             group;
} thread;

void initThreads(u1 *stackbottom);
void startThread(thread*);
void resumeThread(thread*);
void iresumeThread(thread*);
void suspendThread(thread*);
void suspendOnQThread(thread*, thread**);
void yieldThread(void);
void killThread(thread*);
void setPriorityThread(thread*, int);

void sleepThread(s8);
bool aliveThread(thread*);
long framesThread(thread*);

void reschedule(void);

void checkEvents(bool block);

extern int blockInts;
extern bool needReschedule;
extern thread *currentThread;
extern thread *mainThread;
extern ctx contexts[];
extern int threadStackSize;

extern thread *liveThreads;

extern thread *threadQhead[MAX_THREAD_PRIO + 1];

#define CONTEXT(_t)                                                     \
        (contexts[(_t)->PrivateInfo - 1])

#if 1
#define	intsDisable()	blockInts++
#define	intsRestore()   if (blockInts == 1 && needReschedule) {         \
			    reschedule();				\
			}						\
			blockInts--
#else
#define	intsDisable()	do { blockInts++; fprintf(stderr, "++: %d (%s:%d)\n", blockInts, __FILE__, __LINE__); } while (0)
#define	intsRestore()	do { \
                            if (blockInts == 1 && needReschedule) {	\
			        reschedule();				\
			    }						\
			    blockInts--;                                \
                            fprintf(stderr, "--: %d (%s:%d)\n", blockInts, __FILE__, __LINE__);     \
                        } while (0)
#endif

#else

#define intsDisable()
#define intsRestore()

#endif

#endif
