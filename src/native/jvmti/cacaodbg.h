/* src/native/jvmti/cacaodbg.h - contains cacao specifics for debugging support                                  

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Martin Platter

   Changes: 


   $Id: cacao.c,v 3.165 2006/01/03 23:44:38 twisti Exp $

*/

#ifndef _CACAODBG_H
#define _CACAODBG_H

#include "threads/native/threads.h"
#include "native/jvmti/jvmti.h"

#define MSGQEVENT        1
#define MSGQPTRACEREQ    2
#define MSGQPTRACEANS    3

typedef struct {
	jvmtiEvent ev;
	jvmtiEnv *jvmti_env;
	jthread thread;
	jmethodID method;
	jlocation location;
	jclass klass;
	jobject object;
	jfieldID field;
	char signature_type;
	jvalue value;
	jboolean b;
	void* address;
	void** new_address_ptr;
	jmethodID catch_method;
	jlocation catch_location;
	char* name;
	jobject protection_domain;
	jint jint1;
	jint jint2;
	unsigned char* class_data;
	jint* new_class_data_len;
	unsigned char** new_class_data;
	jvmtiAddrLocationMap* map;
	void* compile_info;
	jlong jlong;
} genericEventData;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
struct _jrawMonitorID {
    java_lang_String *name;
};

struct _threadmap {
	pthread_t tid;
	threadobject* cacaothreadobj;	
};

struct threadmap {
	struct _threadmap* map;
	int num;
	int size;
};

struct threadmap thmap;
#endif


/* constants where system breakpoints are stored in the breakpoint table     */
#define HEREWEGOBRK           0 /* used for suspend VM on startup            */
#define SETTHREADOBJECTBRK    1 /* used for EVENT_THREAD_START               */
#define BEGINUSERBRK          2 /* here is where the first user breakpoint is 
								   stored                                    */
struct _brkpt {
    jmethodID method;
    jlocation location;
    void* addr;
    long orig; /* original memory content */
};


struct brkpts {
	struct _brkpt* brk;
	int num;
	int size;
};

struct brkpts jvmtibrkpt;

bool jdwp;                  /* debugger via jdwp                              */
bool jvmti;                 /* jvmti agent                                    */
bool dbgprocess;            /* ture if debugger else debuggee process         */
pid_t debuggee;             /* PID of debuggee                                */

char *transport, *agentarg; /* arguments for jdwp transport and agent load    */
bool suspend;               /* should the virtual machine suspend on startup? */


bool cacaodbgfork();
void cacaodbglisten(char* transport);
jvmtiEnv* new_jvmtienv();
void set_jvmti_phase(jvmtiPhase p);
bool contdebuggee(int signal);
void stopdebuggee();
void fireEvent(genericEventData* data);
bool VMjdwpInit(jvmtiEnv *jvmti_env);
jvmtiEnv* remotedbgjvmtienv;
jvmtiEventCallbacks jvmti_jdwp_EventCallbacks;
void agentload(char* opt_arg);
void agentunload();

void getchildproc (char **ptr, void* addr, int count);
void addbrkpt(void* addr, jmethodID method, jlocation location);
void setsysbrkpt(int sysbrk, void* addr);
jthread threadobject2jthread(threadobject* thread);
jvmtiError allthreads (jint * threads_count_ptr, threadobject ** threads_ptr);
jthread getcurrentthread();

void ipcrm();

#endif

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
