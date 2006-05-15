/* src/native/jvmti/cacaodbg.c - contains entry points for debugging support 
                                 in cacao.

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

   Changes: Edwin Steiner
            Samuel Vinson

   $Id: cacao.c,v 3.165 2006/01/03 23:44:38 twisti Exp $

*/

#include "native/jvmti/jvmti.h"
#include "native/jvmti/cacaodbg.h"
#include "native/jvmti/cacaodbgserver.h"
#include "native/jvmti/dbg.h"
#include "vm/vm.h"
#include "vm/loader.h"
#include "vm/exceptions.h"
#include "vm/builtin.h"
#include "vm/jit/asmpart.h"
#include "vm/stringlocal.h"
#include "toolbox/logging.h"
#include "threads/native/threads.h"

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>


/* allthreads *****************************************************************

   Gets an array of threadobjects of all threads

*******************************************************************************/
jvmtiError allthreads (jint * threads_count_ptr, threadobject*** threads_ptr) {
    int i = 0, cnt = 8; 
    threadobject *thread, **tthreads;
	
#if defined(ENABLE_THREADS)
	tthreads = MNEW(threadobject*, (sizeof(threadobject*) * cnt));

	thread = mainthreadobj;
    do {
        if(thread->o.thread != NULL) {
			fflush(stderr);	

		   /* count and copy only live threads */
		   if (i>=cnt) {
			   MREALLOC(tthreads,threadobject*,cnt,cnt+8);
			   cnt += 8;
		   }
		   tthreads[i] = thread;
		   i++;
		}
		thread = thread->info.prev;

		/* repeat until we got the pointer to the mainthread twice */
	} while (mainthreadobj != thread);

	fflush(stderr);	

    *threads_count_ptr = i;
	*threads_ptr = tthreads;

    return JVMTI_ERROR_NONE;
#else
	return JVMTI_ERROR_NOT_AVAILABLE;
#endif
}


/* getcurrentthread ***********************************************************

   Get jthread structure of current thread. 

*******************************************************************************/
jthread getcurrentthread() {
	return (jthread)(threads_get_current_threadobject())->o.thread;
}



/* brktablecreator*************************************************************

   helper function to enlarge the breakpoint table if needed

*******************************************************************************/

static void brktablecreator() {
	struct _brkpt* tmp;
	struct brkpts *jvmtibrkpt;

	jvmtibrkpt = &dbgcom->jvmtibrkpt;;
	if (jvmtibrkpt->size == 0) {
		jvmtibrkpt->brk = MNEW(struct _brkpt, 16);
		memset(jvmtibrkpt->brk, 0, sizeof(struct _brkpt)*16);
		jvmtibrkpt->size = 16;
		jvmtibrkpt->num = BEGINUSERBRK;
	} else {
		jvmtibrkpt->size += 16;
		tmp = jvmtibrkpt->brk;
		jvmtibrkpt->brk = MNEW(struct _brkpt, jvmtibrkpt->size);
		memset(jvmtibrkpt->brk, 0, sizeof(struct _brkpt)*(jvmtibrkpt->size));
		memcpy((void*)jvmtibrkpt->brk,(void*)tmp,jvmtibrkpt->size);
		MFREE(tmp,struct _brkpt,jvmtibrkpt->size-16);
	}	
}


/* setsysbrkpt ****************************************************************

   sets a system breakpoint in breakpoint table and calls set breakpoint

*******************************************************************************/

void setsysbrkpt(int sysbrk, void* addr) {	
	struct brkpts *jvmtibrkpt;

	pthread_mutex_lock(&dbgcomlock);
	jvmtibrkpt = &dbgcom->jvmtibrkpt;;

	if (jvmtibrkpt->size == jvmtibrkpt->num)
		brktablecreator();

	assert (sysbrk < BEGINUSERBRK);
	jvmtibrkpt->brk[sysbrk].addr = addr;


	dbgcom->setbrkpt = true;
	dbgcom->brkaddr = addr;
	jvmtibrkpt->brk[sysbrk].orig = dbgcom->brkorig;
	pthread_mutex_unlock(&dbgcomlock);

	/* call cacaodbgserver */
	TRAP; 

	fprintf (stderr,"setsysbrk %d %X  done\n",sysbrk, addr);
}


/* addbrkpt *******************************************************************

   adds a breakpoint to breakpoint table and calls set breakpoint

*******************************************************************************/

void addbrkpt(void* addr, jmethodID method, jlocation location) {
	struct brkpts *jvmtibrkpt;

	pthread_mutex_lock(&dbgcomlock);
	jvmtibrkpt = &dbgcom->jvmtibrkpt;;

	if (jvmtibrkpt->size == jvmtibrkpt->num)
		brktablecreator();

	assert (jvmtibrkpt->size > jvmtibrkpt->num);
	fprintf (stderr,"add brk add: %X\n",addr);
	jvmtibrkpt->brk[jvmtibrkpt->num].addr = addr;
	jvmtibrkpt->brk[jvmtibrkpt->num].method = method;
	jvmtibrkpt->brk[jvmtibrkpt->num].location = location;

	/* todo: set breakpoint */
/*	jvmtibrkpt.brk[jvmtibrkpt.num].orig = */
	jvmtibrkpt->num++;
	pthread_mutex_unlock(&dbgcomlock);

	fprintf (stderr,"add brk done\n");
}


/* setup_jdwp_thread *****************************************************

   Helper function to start JDWP threads

*******************************************************************************/

void setup_jdwp_thread(char* transport) {
	java_objectheader *o;
	methodinfo *m;
	java_lang_String  *s;
	classinfo *class;

	/* new gnu.classpath.jdwp.Jdwp() */
	class = load_class_from_sysloader(
            utf_new_char("gnu.classpath.jdwp.Jdwp"));
	if (!class)
		throw_main_exception_exit();

	o = builtin_new(class);

	if (!o)
		throw_main_exception_exit();
	
	m = class_resolveclassmethod(class,
                                     utf_init, 
                                     NULL,
                                     class_java_lang_Object,
                                     true);
	if (!m)
            throw_main_exception_exit();
        
	vm_call_method(m,o);
        
	/* configure(transport,NULL) */
	m = class_resolveclassmethod(
            class, utf_new_char("configure"), 
            utf_new_char("(Ljava/lang/String;)V"),
            class_java_lang_Object,
            false);

	s = javastring_new_from_ascii(&transport[1]);

	vm_call_method(m,o,s);

	if (!m)
		throw_main_exception_exit();


	/* _doInitialization */
	m = class_resolveclassmethod(class,
                                     utf_new_char("_doInitialization"), 
                                     utf_new_char("()V"),
                                     class,
                                     false);
	
	if (!m)
            throw_main_exception_exit();
        
	vm_call_method(m,o);
}

/* cacaobreakpointhandler **********************************************************

   handles breakpoints. called by cacaodbgserver.

*******************************************************************************/

void cacaobreakpointhandler() {
	basic_event ev;
	genericEventData data;
	int i;

	/* XXX to be continued :-) */

	fprintf(stderr,"breakpoint handler called\n");
	log_text(" - signal %d", ev.signal);
	switch (ev.signal) {
	case SIGTRAP:
		/* search the breakpoint that has been triggered */
		i=0;
		while ((ev.ip!=dbgcom->jvmtibrkpt.brk[i].addr) && (i<dbgcom->jvmtibrkpt.num)) i++;
		
		fprintf(stderr,"cacaodbglisten SIGTRAP switch after while loop i %d\n",i);
		
		switch (i) {
		case SETTHREADOBJECTBRK:
			/* threads_set_current_threadobject */
			fprintf(stderr,"IP %X == threads_set_current_threadobject\n",ev.ip);
			data.ev=JVMTI_EVENT_THREAD_START;
			fireEvent(&data);
			break;
		default:
			if ((i >= BEGINUSERBRK) && (i<dbgcom->jvmtibrkpt.num)) {
				log_text("todo: user defined breakpoints are not handled yet");
			} else 
				log_text("breakpoint not handled - continue anyway");
		}
		break;
	case SIGQUIT:
		log_text("debugger process SIGQUIT");
		data.ev=JVMTI_EVENT_VM_DEATH;
		fireEvent(&data);		
		break;
	default:
		log_text("signal not handled");
	}
}


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
 * vim:noexpandtab:sw=4:ts=4:
 */
