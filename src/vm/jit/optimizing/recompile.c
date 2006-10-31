/* src/vm/jit/optimizing/recompile.c - recompilation system

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, J. Wenninger, Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Christian Thalinger

   Changes:

   $Id: cacao.c 4357 2006-01-22 23:33:38Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <stdlib.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "native/jni.h"
#include "native/include/java_lang_Thread.h"
#include "native/include/java_lang_VMThread.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
# include "threads/native/threads.h"
#endif

#include "toolbox/list.h"
#include "vm/builtin.h"
#include "vm/classcache.h"
#include "vm/exceptions.h"
#include "vm/stringlocal.h"
#include "vm/jit/optimizing/recompile.h"


/* global variables ***********************************************************/

static java_lang_VMThread *recompile_vmthread;
static java_objectheader *lock_recompile_thread;
static list *list_recompile_methods;


/* recompile_init **************************************************************

   Initializes the recompilation system.

*******************************************************************************/

bool recompile_init(void)
{
	/* initialize the recompile lock object */

	lock_recompile_thread = NEW(java_objectheader);

	lock_init_object_lock(lock_recompile_thread);

	/* create method list */

	list_recompile_methods = list_create(OFFSET(list_method_entry, linkage));

	/* everything's ok */

	return true;
}


/* recompile_replace_vftbl *****************************************************

   XXX

*******************************************************************************/

static void recompile_replace_vftbl(methodinfo *m)
{
	codeinfo               *code;
	codeinfo               *pcode;
	u4                      slot;
	classcache_name_entry  *nmen;
	classcache_class_entry *clsen;
	classinfo              *c;
	vftbl_t                *vftbl;
	s4                      i;

	/* get current and previous codeinfo structure */

	code  = m->code;
	pcode = code->prev;

	assert(pcode);

	/* iterate over all classes */

	for (slot = 0; slot < hashtable_classcache.size; slot++) {
		nmen = (classcache_name_entry *) hashtable_classcache.ptr[slot];

		for (; nmen; nmen = nmen->hashlink) {
			/* iterate over all class entries */

			for (clsen = nmen->classes; clsen; clsen = clsen->next) {
				c = clsen->classobj;

				if (c == NULL)
					continue;

				/* Search for entrypoint of the previous codeinfo in
				   the vftbl and replace it with the current one. */

				vftbl = c->vftbl;

				/* Is the class linked? Means, is the vftbl finished? */

				if (!(c->state & CLASS_LINKED))
					continue;

				/* Does the class have a vftbl? Some internal classes
				   (e.g. $NEW$) are linked, but do not have a
				   vftbl. */

				if (vftbl == NULL)
					continue;

				for (i = 0; i < vftbl->vftbllength; i++) {
					if (vftbl->table[i] == pcode->entrypoint) {
#if !defined(NDEBUG)
						printf("replacing vftbl in: ");
						class_println(c);
#endif
						vftbl->table[i] = code->entrypoint;
					}
				}
			}
		}
	}
}


/* recompile_thread ************************************************************

   XXX

*******************************************************************************/

static void recompile_thread(void)
{
	list_method_entry *lme;

	while (true) {
		/* get the lock on the recompile lock object, so we can call wait */

		lock_monitor_enter(lock_recompile_thread);

		/* wait forever (0, 0) on that object till we are signaled */
	
		lock_wait_for_object(lock_recompile_thread, 0, 0);

		/* leave the lock */

		lock_monitor_exit(lock_recompile_thread);

		/* get the next method and recompile it */

		while ((lme = list_first(list_recompile_methods)) != NULL) {
			/* recompile this method */

			if (jit_recompile(lme->m) != NULL) {
				/* replace in vftbl's */

				recompile_replace_vftbl(lme->m);
			}
			else {
				/* XXX what is the right-thing(tm) to do here? */

				exceptions_print_exception(*exceptionptr);
			}

			/* remove the compiled method */

			list_remove(list_recompile_methods, lme);

			/* free the entry */

			FREE(lme, list_method_entry);
		}
	}
}


/* recompile_start_thread ******************************************************

   Starts the recompilation thread.

*******************************************************************************/

bool recompile_start_thread(void)
{
	java_lang_Thread *t;

	/* create the profile object */

	recompile_vmthread =
		(java_lang_VMThread *) builtin_new(class_java_lang_VMThread);

	if (recompile_vmthread == NULL)
		return false;

	t = (java_lang_Thread *) builtin_new(class_java_lang_Thread);

	t->vmThread = recompile_vmthread;
	t->name     = javastring_new_from_ascii("Recompiler");
	t->daemon   = true;
	t->priority = 5;

	recompile_vmthread->thread = t;

	/* actually start the recompilation thread */

	threads_start_thread(t, recompile_thread);

	/* everything's ok */

	return true;
}


/* recompile_queue_method ******************************************************

   Adds a method to the recompilation list and signal the
   recompilation thread that there is some work to do.

*******************************************************************************/

void recompile_queue_method(methodinfo *m)
{
	list_method_entry *lme;

	/* create a method entry */

	lme = NEW(list_method_entry);
	lme->m = m;

	/* and add it to the list */

	list_add_last(list_recompile_methods, lme);

	/* get the lock on the recompile lock object, so we can call notify */

	lock_monitor_enter(lock_recompile_thread);

	/* signal the recompiler thread */
	
	lock_notify_object(lock_recompile_thread);

	/* leave the lock */

	lock_monitor_exit(lock_recompile_thread);
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