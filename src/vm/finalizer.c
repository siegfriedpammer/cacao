/* src/vm/finalizer.c - finalizer linked list and thread

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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

   Authors: Christian Thalinger

   Changes:

   $Id: finalizer.c 3686 2005-11-16 13:29:58Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "mm/memory.h"
#include "native/jni.h"
#include "native/include/java_lang_Thread.h"
#include "native/include/java_lang_VMThread.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"


/* local structures ***********************************************************/

typedef struct finalizer_entry finalizer_entry;

struct finalizer_entry {
	java_objectheader *o;
	listnode           linkage;
};


/* global variables ***********************************************************/

#if defined(USE_THREADS)
static java_lang_VMThread *finalizer_vmthread;
static java_objectheader *lock_finalizer_thread;
static list *finalizer_list;
#endif


/* finalizer_init **************************************************************

   Initializes the finalizer global lock and the linked list.

*******************************************************************************/

bool finalizer_init(void)
{
#if defined(USE_THREADS)
	lock_finalizer_thread = NEW(java_objectheader);

# if defined(NATIVE_THREADS)
	initObjectLock(lock_finalizer_thread);
# endif

	/* initialize the finalizer list */

	finalizer_list = NEW(list);
	list_init(finalizer_list, OFFSET(finalizer_entry, linkage));
#endif

	/* everything's ok */

	return true;
}


/* finalizer_thread ************************************************************

   This thread waits on an object for a notification and the runs the
   finalizers (finalizer thread).  This is necessary because of a
   possible deadlock in the GC.

*******************************************************************************/

#if defined(USE_THREADS)
static void finalizer_thread(void)
{
	finalizer_entry   *fi;
	java_objectheader *o;

	/* get the lock on the finalizer lock object, so we can call wait */

	while (true) {
		/* wait forever (0, 0) on that object till we are signaled */
	
		wait_cond_for_object(lock_finalizer_thread, 0, 0);

		/* now handle all finalizers stored in the list */

		fi = (finalizer_entry *) list_first(finalizer_list);

		/* release the lock so other threads, or the finalizer thread
		   itself, can add new finalizers to the list */

		builtin_monitorexit(lock_finalizer_thread);

		/* is there actually a finalizer in the list? */

		if (fi) {
			do {
				/* just for simpler code */

				o = fi->o;

				/* call the finalizer function */

				asm_calljavafunction(o->vftbl->class->finalizer, o,
									 NULL, NULL, NULL);

				/* if we had an exception in the finalizer, ignore it */

				*exceptionptr = NULL;

				/* enter the monitor again, so we don't get finalizer
				   list race conditions */

				builtin_monitorenter(lock_finalizer_thread);

				/* remove and clear finalized entry */

				list_remove(finalizer_list, fi);

				fi->o = NULL;
				FREE(fi, finalizer_entry);

				/* get next finalizer from the list */

				fi = list_first(finalizer_list);

				/* leave the monitor again */

				builtin_monitorexit(lock_finalizer_thread);
			} while (fi != NULL);
		}

		builtin_monitorenter(lock_finalizer_thread);
	}
}
#endif

/* finalizer_start_thread ******************************************************

   Starts the finalizer thread.

*******************************************************************************/

#if defined(USE_THREADS)
bool finalizer_start_thread(void)
{
	java_lang_Thread *t;

	/* create the finalizer object */

	finalizer_vmthread =
		(java_lang_VMThread *) builtin_new(class_java_lang_VMThread);

	if (!finalizer_vmthread)
		return false;

	t = (java_lang_Thread *) builtin_new(class_java_lang_Thread);

	t->vmThread = finalizer_vmthread;
	t->name     = javastring_new_char("Finalizer");
	t->daemon   = true;
	t->priority = 5;

	finalizer_vmthread->thread = t;

	/* actually start the finalizer thread */

	threads_start_thread(t, finalizer_thread);

	/* everything's ok */

	return true;
}
#endif


/* finalizer_add ***************************************************************

   Adds a finalizer to be called to the linked list of finalizers.

*******************************************************************************/

void finalizer_add(void *o, void *p)
{
#if defined(USE_THREADS)
	java_objectheader *ob;
	finalizer_entry   *fi;

	ob = (java_objectheader *) o;

	/* wait for the finalizer thread to finish finalizer list operations */

	builtin_monitorenter(lock_finalizer_thread);

	/* create finalizer entry, fill it and add it to the list */

	fi = NEW(finalizer_entry);
	fi->o = ob;

	list_addlast(finalizer_list, fi);

	/* wakeup the finalizer thread */

	signal_cond_for_object(lock_finalizer_thread);

	/* release the lock after the entry is added */

	builtin_monitorexit(lock_finalizer_thread);
#else
	java_objectheader *ob;

	ob = (java_objectheader *) o;

	/* call the finalizer function */

	asm_calljavafunction(ob->vftbl->class->finalizer, ob, NULL, NULL, NULL);

	/* if we had an exception in the finalizer, ignore it */

	*exceptionptr = NULL;
#endif
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
 */
