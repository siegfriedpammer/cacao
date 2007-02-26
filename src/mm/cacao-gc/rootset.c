/* mm/cacao-gc/rootset.c - GC module for root set management

   Copyright (C) 2006 R. Grafl, A. Krall, C. Kruegel,
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Michael Starzinger

   $Id$

*/


#include "config.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#else
/*# include "threads/none/threads.h"*/
#endif

#include "gc.h"
#include "final.h"
#include "heap.h"
#include "mark.h"
#include "mm/memory.h"
#include "toolbox/logging.h"
#include "vm/global.h"
#include "vm/jit/replace.h"
#include "vm/jit/stacktrace.h"
#include "vmcore/linker.h"
#include "vmcore/loader.h"


rootset_t *rootset_create(void)
{
	rootset_t *rs;

	rs = DNEW(rootset_t);

	rs->next     = NULL;
	rs->refcount = 0;

	return rs;
}


/* rootset_from_globals ********************************************************

   Searches global variables to compile the global root set out of references
   contained in them.

   SEARCHES IN:
     - classloader objects (loader.c)
     - global reference table (jni.c)
     - finalizer entries (final.c)

*******************************************************************************/

void rootset_from_globals(rootset_t *rs)
{
	hashtable_classloader_entry *cle;
	final_entry *fe;
	int refcount;
	int i;

	GC_LOG( dolog("GC: Acquiring Root-Set from globals ..."); );

	/* initialize the rootset struct */
	GC_ASSERT(rs);
	GC_ASSERT(rs->refcount == 0);
	rs->thread = ROOTSET_DUMMY_THREAD;
	rs->ss     = NULL;
	rs->es     = NULL;
	rs->stb    = NULL;

    /* walk through all classloaders */
	refcount = rs->refcount;
	for (i = 0; i < hashtable_classloader->size; i++) {
		cle = hashtable_classloader->ptr[i];

		while (cle) {

			GC_LOG2( printf("Found Classloader: %p\n", (void *) cle->object); );

			/* add this classloader to the root set */
			GC_ASSERT(refcount < RS_REFS); /* TODO: UGLY!!! */
 			rs->refs[refcount] = &( cle->object );
			rs->ref_marks[refcount] = true;
			refcount++;

			cle = cle->hashlink;
		}
	}

	/* walk through all finalizer entries */
	/* REMEMBER: all threads are stopped, so we can use unsynced access here */
	fe = list_first_unsynced(final_list);
	while (fe) {

		GC_LOG2( printf("Found Finalizer Entry: %p\n", (void *) fe->o); );

		/* add this object with finalizer to the root set */
		GC_ASSERT(refcount < RS_REFS); /* TODO: UGLY!!! */
		rs->refs[refcount] = &( fe->o );
		rs->ref_marks[refcount] = false;
		refcount++;

		fe = list_next_unsynced(final_list, fe);
	}

	/* remeber how many references there are inside this root set */
	rs->refcount = refcount;

}


/* rootset_from_thread *********************************************************

   Searches the stack of the passed thread for references and compiles a
   root set out of them.

   NOTE: uses dump memory!

   IN:
	  thread...TODO
      rs.......TODO

   OUT:
	  TODO!!!

*******************************************************************************/

void rootset_from_thread(threadobject *thread, rootset_t *rs)
{
	stacktracebuffer *stb;
	executionstate_t *es;
	sourcestate_t    *ss;
	sourceframe_t    *sf;
	rplpoint         *rp;
	s4                rpcount;
	int               i;

	/* TODO: remove these */
    java_objectheader *o;
    int refcount;

	GC_LOG( dolog("GC: Acquiring Root-Set from thread (%p) ...", (void *) thread); );

	GC_LOG2( printf("Stacktrace of current thread:\n");
			stacktrace_dump_trace(thread); );

	/* create empty execution state */
	es = DNEW(executionstate_t);
	es->pc = 0;
	es->sp = 0;
	es->pv = 0;
	es->code = NULL;

	/* TODO: we assume we are in a native and in current thread! */
	ss = replace_recover_source_state(NULL, NULL, es);

	/* print our full source state */
	GC_LOG2( replace_sourcestate_println(ss); );

	/* initialize the rootset struct */
	GC_ASSERT(rs);
	GC_ASSERT(rs->refcount == 0);
	rs->thread = thread;
	rs->ss = ss;
	rs->es = es;
	rs->stb = stb;

	/* now inspect the source state to compile the root set */
	refcount = rs->refcount;
	for (sf = ss->frames; sf != NULL; sf = sf->down) {

		GC_ASSERT(sf->javastackdepth == 0);

		for (i = 0; i < sf->javalocalcount; i++) {

			if (sf->javalocaltype[i] != TYPE_ADR)
				continue;

			o = sf->javalocals[i].a;

			/* check for outside or null pointer */
			if (!POINTS_INTO(o, heap_region_main->base, heap_region_main->ptr))
				continue;

			/* add this reference to the root set */
			GC_LOG2( printf("Found Reference: %p\n", (void *) o); );
			GC_ASSERT(refcount < RS_REFS); /* TODO: UGLY!!! */
			rs->refs[refcount] = (java_objectheader **) &( sf->javalocals[i] );
			rs->ref_marks[refcount] = true;

			refcount++;

		}

	}

	/* remeber how many references there are inside this root set */
	rs->refcount = refcount;

}


void rootset_writeback(rootset_t *rs)
{
	sourcestate_t    *ss;
	executionstate_t *es;
	rplpoint         *rp;
	stacktracebuffer *stb;

	/* TODO: only a dirty hack! */
	GC_ASSERT(rs->next->thread == THREADOBJECT);
	ss = rs->next->ss;
	es = rs->next->es;

	replace_build_execution_state_intern(ss, es);

}


/* Debugging ******************************************************************/

#if !defined(NDEBUG)
void rootset_print(rootset_t *rs)
{
	java_objectheader *o;
	int i;

	/* walk through all rootsets in the chain */
	printf("Root Set Chain:\n");
	while (rs) {

		/* print the thread this rootset belongs to */
		if (rs->thread == ROOTSET_DUMMY_THREAD) {
			printf("\tGlobal Root Set:\n");
		} else {
#if defined(ENABLE_THREADS)
			printf("\tLocal Root Set with Thread-Id %p:\n", (void *) rs->thread->tid);
#else
			printf("\tLocal Root Set:\n");
#endif
		}

		/* print the references in this rootset */
		printf("\tReferences (%d):\n", rs->refcount);
		for (i = 0; i < rs->refcount; i++) {

			o = *( rs->refs[i] );

			/*printf("\t\tReference at %p points to ...\n", (void *) rs->refs[i]);*/
			printf("\t\t");
			if (rs->ref_marks[i])
				printf("MRK");
			else
				printf("UPD");
			printf(" ");
			heap_print_object(o);
			printf("\n");

		}

		rs = rs->next;

	}

}
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
 * vim:noexpandtab:sw=4:ts=4:
 */
