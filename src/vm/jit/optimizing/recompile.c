/* src/vm/jit/optimizing/recompile.c - recompilation system

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

*/


#include "config.h"

#include <assert.h>
#include <stdlib.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "threads/lock-common.h"
#include "threads/threads-common.h"

#include "toolbox/list.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/stringlocal.h"

#include "vm/jit/code.h"
#include "vm/jit/jit.h"

#include "vm/jit/optimizing/recompile.h"

#include "vmcore/classcache.h"


/* global variables ***********************************************************/

static java_object_t *lock_thread_recompile;
static list_t        *list_recompile_methods;


/* recompile_init **************************************************************

   Initializes the recompilation system.

*******************************************************************************/

bool recompile_init(void)
{
	/* initialize the recompile lock object */

	lock_thread_recompile = NEW(java_object_t);

	LOCK_INIT_OBJECT_LOCK(lock_thread_recompile);

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

		LOCK_MONITOR_ENTER(lock_thread_recompile);

		/* wait forever on that object till we are signaled */
	
		LOCK_WAIT_FOREVER(lock_thread_recompile);

		/* leave the lock */

		LOCK_MONITOR_EXIT(lock_thread_recompile);

		/* get the next method and recompile it */

		while ((lme = list_first(list_recompile_methods)) != NULL) {
			/* recompile this method */

			if (jit_recompile(lme->m) != NULL) {
				/* replace in vftbl's */

				recompile_replace_vftbl(lme->m);
			}
			else {
				/* XXX what is the right-thing(tm) to do here? */

				exceptions_print_current_exception();
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
	utf *name;

	name = utf_new_char("Recompiler");

	if (!threads_thread_start_internal(name, recompile_thread))
		return false;

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

	LOCK_MONITOR_ENTER(lock_thread_recompile);

	/* signal the recompiler thread */
	
	LOCK_NOTIFY(lock_thread_recompile);

	/* leave the lock */

	LOCK_MONITOR_EXIT(lock_thread_recompile);
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
