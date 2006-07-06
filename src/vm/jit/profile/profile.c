/* src/vm/jit/profile.c - runtime profiling

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
# include "threads/native/threads.h"
#endif

#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/classcache.h"
#include "vm/method.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/jit.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/recompile.h"


/* global variables ***********************************************************/

#if defined(ENABLE_THREADS)
static threadobject *profile_threadobject;
#endif


/* profile_init ****************************************************************

   Initializes the profile global lock.

*******************************************************************************/

bool profile_init(void)
{
	/* everything's ok */

	return true;
}


/* profile_thread **************************************************************

   XXX

*******************************************************************************/

static s4 runs = 0;
static s4 hits = 0;
static s4 misses = 0;

#if defined(ENABLE_THREADS)
static void profile_thread(void)
{
	threadobject *t;
	s4            nanos;
	u1           *pc;
	u1           *pv;
	methodinfo   *m;
	codeinfo     *code;

	while (true) {
		/* sleep thread for 0.5-1.0 ms */

		nanos = 500 + (int) (500.0 * (rand() / (RAND_MAX + 1.0)));
/* 		fprintf(stderr, "%d\n", nanos); */

		threads_sleep(0, nanos);
		runs++;

		/* iterate over all started threads */

		t = mainthreadobj;

		do {
			/* is this a Java thread? */

			if (t->flags & THREAD_FLAG_JAVA) {
				/* send SIGUSR2 to thread to get the current PC */

				pthread_kill(t->tid, SIGUSR2);

				/* the thread object now contains the current thread PC */

				pc = t->pc;

				/* get the PV for the current PC */

				pv = codegen_findmethod(pc);

				/* get methodinfo pointer from data segment */

				if (pv == NULL) {
					misses++;
				}
				else {
					code = *((codeinfo **) (pv + CodeinfoPointer));

					/* For asm_vm_call_method the codeinfo pointer is
					   NULL (which is also in the method tree). */

					if (code != NULL) {
						m = code->m;

						/* native methods are never recompiled */

						if (!(m->flags & ACC_NATIVE)) {
							/* increase the method incovation counter */

							code->frequency++;
							hits++;

							if (code->frequency > 500) {
								/* clear frequency count before
								   recompilation */

								code->frequency = 0;

								/* add this method to the method list
								   and start recompilation */

								recompile_queue_method(m);
							}
						}
					}
				}
			}

			t = t->next;
		} while ((t != NULL) && (t != mainthreadobj));
	}
}
#endif


/* profile_start_thread ********************************************************

   Starts the profile sampling thread.

*******************************************************************************/

#if defined(ENABLE_THREADS)
bool profile_start_thread(void)
{
	java_lang_Thread *t;

	/* create the profile object */

	profile_threadobject = NEW(threadobject);

	if (profile_threadobject == NULL)
		return false;

	t = (java_lang_Thread *) builtin_new(class_java_lang_Thread);

	t->vmThread = (java_lang_VMThread *) profile_threadobject;
	t->name     = javastring_new_from_ascii("Profiling Sampler");
	t->daemon   = true;
	t->priority = 5;

	profile_threadobject->o.thread = t;

	/* actually start the profile sampling thread */

	threads_start_thread(t, profile_thread);

	/* everything's ok */

	return true;
}
#endif


/* profile_printstats **********************************************************

   Prints profiling statistics gathered during runtime.

*******************************************************************************/

#if !defined(NDEBUG)
void profile_printstats(void)
{
	list                   *l;
	list_method_entry      *lme;
	list_method_entry      *tlme;
	classinfo              *c;
	methodinfo             *m;
	codeinfo               *code;
	u4                      slot;
	classcache_name_entry  *nmen;
	classcache_class_entry *clsen;
	s4                      i;
	s4                      j;
	u4                      frequency;
	s8                      cycles;

	frequency = 0;
	cycles    = 0;

	/* create new method list */

	l = list_create(OFFSET(list_method_entry, linkage));

	/* iterate through all classes and methods */

	for (slot = 0; slot < hashtable_classcache.size; slot++) {
		nmen = (classcache_name_entry *) hashtable_classcache.ptr[slot];

		for (; nmen; nmen = nmen->hashlink) {
			/* iterate over all class entries */

			for (clsen = nmen->classes; clsen; clsen = clsen->next) {
				c = clsen->classobj;

				if (c == NULL)
					continue;

				/* interate over all class methods */

				for (i = 0; i < c->methodscount; i++) {
					m = &(c->methods[i]);

					code = m->code;

					/* was this method actually called? */

					if ((code != NULL) && (code->frequency > 0)) {
						/* add to overall stats */

						frequency += code->frequency;
						cycles    += code->cycles;

						/* create new list entry */

						lme = NEW(list_method_entry);
						lme->m = m;

						/* sort the new entry into the list */
						
						if ((tlme = list_first(l)) == NULL) {
							list_add_first(l, lme);
						}
						else {
							for (; tlme != NULL; tlme = list_next(l, tlme)) {
								/* check the frequency */

								if (code->frequency > tlme->m->code->frequency) {
									list_add_before(l, tlme, lme);
									break;
								}
							}

							/* if we are at the end of the list, add
							   it as last entry */

							if (tlme == NULL)
								list_add_last(l, lme);
						}
					}
				}
			}
		}
	}

	/* print all methods sorted */

	printf(" frequency     ratio         cycles     ratio   method name\n");
	printf("----------- --------- -------------- --------- -------------\n");

	/* now iterate through the list and print it */

	for (lme = list_first(l); lme != NULL; lme = list_next(l, lme)) {
		/* get method of the list element */

		m = lme->m;

		code = m->code;

		printf("%10d   %.5f   %12ld   %.5f   ",
			   code->frequency,
			   (double) code->frequency / (double) frequency,
			   (long) code->cycles,
			   (double) code->cycles / (double) cycles);

		method_println(m);

		/* print basic block frequencies */

		if (opt_prof_bb) {
			for (j = 0; j < code->basicblockcount; j++)
				printf("                                                    L%03d: %10d\n",
					   j, code->bbfrequency[j]);
		}
	}

	printf("-----------           -------------- \n");
	printf("%10d             %12ld\n", frequency, (long) cycles);

	printf("\nruns  : %10d\n", runs);
	printf("hits  : %10d\n", hits);
	printf("misses: %10d\n", misses);
}
#endif /* !defined(NDEBUG) */


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
