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
#include "native/include/java_lang_String.h"
#include <ltdl.h>




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


struct _brkpt {
    jmethodID method;
    jlocation location;
    void* addr; /* memory address          */
    long orig;  /* original memory content */
};


struct brkpts {
	struct _brkpt* brk;
	int num;
	int size;
};


typedef struct {
	int running;
	void* breakpointhandler;
	bool setbrkpt;
	void* brkaddr;
	long brkorig;
	struct brkpts jvmtibrkpt;
} cacaodbgcommunication;

cacaodbgcommunication *dbgcom;

#if defined(ENABLE_THREADS)
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
#define SETTHREADOBJECTBRK    0 /* used for EVENT_THREAD_START               */
#define BEGINUSERBRK          1 /* here is where the first user breakpoint is 
								   stored                                    */


bool jdwp;                  /* debugger via jdwp                              */
bool jvmti;                 /* jvmti agent                                    */

char *transport, *agentarg; /* arguments for jdwp transport and agent load    */
bool suspend;               /* should the virtual machine suspend on startup? */

extern pthread_mutex_t dbgcomlock;

void setup_jdwp_thread(char* transport);
void cacaobreakpointhandler();
jvmtiEnv* new_jvmtienv();
void set_jvmti_phase(jvmtiPhase p);
void fireEvent(genericEventData* data);
bool VMjdwpInit();
void agentload(char* opt_arg, bool agentbypath, lt_dlhandle  *handle, char **libname);
void agentunload();
void addbrkpt(void* addr, jmethodID method, jlocation location);
void setsysbrkpt(int sysbrk, void* addr);
jvmtiError allthreads (jint * threads_count_ptr, threadobject *** threads_ptr);
jthread getcurrentthread();

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
