/*
 * i386/threads.h
 * i386 threading information.
 *
 * Copyright (c) 1996 T. J. Wilkinson & Associates, London, UK.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst
 * 
 * See file COPYRIGHT for information on usage and disclaimer of warranties
 *
 * Written by Tim Wilkinson <tim@tjwassoc.demon.co.uk>, 1996.
 */

#ifndef __alpha_threads_h
#define __alpha_threads_h

/**/
/* Thread handling */
/**/

#include "../threads/thread.h"

void perform_alpha_threadswitch (u1 **from, u1 **to);
u1* initialize_thread_stack (void *func, u1 *stack);
u1* used_stack_top (void);

#define	THREADSTACKSIZE		(64 * 1024)

#define	THREADSWITCH(to, from)	   perform_alpha_threadswitch(&(from)->restorePoint,\
                                                              &(to)->restorePoint)

#define THREADINIT(to, func)       (to)->restorePoint = \
                                     initialize_thread_stack((u1*)(func), \
                                                             (to)->stackEnd)

#define USEDSTACKTOP(top)          (top) = used_stack_top()

#define	THREADINFO(ee)						\
		do {						\
			(ee)->restorePoint = 0;			\
			(ee)->flags = THREAD_FLAGS_NOSTACKALLOC;\
		} while(0)

/*
			void* ptr;				\
			asm("addq $30,$31,%0" : "=r" (ptr));	\
			(ee)->stackEnd = ptr;	                \
			(ee)->stackBase = (ee)->stackEnd - threadStackSize;\
*/

/*
#define	THREADFRAMES(tid, cnt)					\
		do {						\
			void** ptr;				\
			cnt = 0;				\
			if (tid == currentThread) {		\
				asm("movl %%ebp,%0" : "=r" (ptr));\
			}					\
			else {					\
				ptr = ((void***)tid->PrivateInfo->restorePoint)[2];\
			}					\
			while (*ptr != 0) {			\
				cnt++;				\
				ptr = (void**)*ptr;		\
			}					\
		} while (0)
*/

#endif
