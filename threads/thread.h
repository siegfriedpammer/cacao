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

#ifndef _THREAD_H
#define _THREAD_H

#include "config.h"

#ifdef USE_THREADS

#include "global.h"

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

typedef struct {
	int super_baseval, super_diffval, sub_baseval;
} castinfo;

#if !defined(NATIVE_THREADS)

#define MAXTHREADS              256          /* schani */

#if 1
#define DBG(s)
#define SDBG(s)
#else
#define DBG(s)                 s
#define SDBG(s)                s
#endif

struct _thread;

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


/* This structure mirrors java.lang.Thread.h */
typedef struct _thread {
   java_objectheader header;
   struct _threadGroup* group;
   struct java_objectheader* toRun;
   struct java_objectheader* name;
   s8 PrivateInfo;
   struct _thread* next;
   s4 daemon;
   s4 priority;
   struct java_objectheader* contextClassLoader;
} thread;

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
void sleepThread (s8);
bool aliveThread (thread*);
long framesThread (thread*);

void reschedule (void);

void checkEvents (bool block);

extern int blockInts;
extern bool needReschedule;
extern thread *currentThread;
extern thread *mainThread;
extern ctx contexts[];
extern int threadStackSize;

extern thread *liveThreads;
extern thread *sleepThreads;

extern thread *threadQhead[MAX_THREAD_PRIO + 1];


#define CONTEXT(_t)     (contexts[(_t)->PrivateInfo - 1])

#define	intsDisable()	blockInts++

#define	intsRestore()   if (blockInts == 1 && needReschedule) { \
                            reschedule(); \
                        } \
                        blockInts--


/* access macros */

#define	THREADSTACKSIZE         (32 * 1024)

#define	THREADSWITCH(to, from) \
    do { \
        asm_perform_threadswitch(&(from)->restorePoint,\
                                 &(to)->restorePoint, &(from)->usedStackTop); \
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


/* function prototypes */
void asm_perform_threadswitch(u1 **from, u1 **to, u1 **stackTop);
u1*  asm_initialize_thread_stack(void *func, u1 *stack);

#else // defined(NATIVE_THREADS)
#include "nativethread.h"
#endif

#else

#define intsDisable()
#define intsRestore()

#endif /* USE_THREADS */

#endif /* _THREAD_H */

