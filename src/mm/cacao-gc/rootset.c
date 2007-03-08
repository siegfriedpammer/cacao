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
     - thread objects (threads.c)
     - classloader objects (loader.c)
     - global reference table (jni.c)
     - finalizer entries (final.c)

*******************************************************************************/

#define ROOTSET_ADD(adr,marks,type) \
	GC_ASSERT(refcount < RS_REFS); /* TODO: UGLY!!! */ \
	rs->refs[refcount]      = (adr); \
	rs->ref_marks[refcount] = (marks); \
	rs->ref_type[refcount]  = (type); \
	refcount++;

void rootset_from_globals(rootset_t *rs)
{
	threadobject                *thread;
	hashtable_classloader_entry *cle;
	hashtable_global_ref_entry  *gre;
	final_entry                 *fe;
	int refcount;
	int i;

	GC_LOG( dolog("GC: Acquiring Root-Set from globals ..."); );

	/* initialize the rootset struct */
	GC_ASSERT(rs);
	GC_ASSERT(rs->refcount == 0);
	rs->thread = ROOTSET_DUMMY_THREAD;
	rs->ss     = NULL;
	rs->es     = NULL;

	refcount = rs->refcount;

#if defined (ENABLE_THREADS)
	/* walk through all threadobjects */
	thread = mainthreadobj;
	do {

		GC_LOG2( printf("Found Java Thread Objects: %p\n", (void *) thread->object); );

		/* add this javathreadobject to the root set */
		ROOTSET_ADD(&( thread->object ), true, REFTYPE_THREADOBJECT);

		thread = thread->next;
	} while ((thread != NULL) && (thread != mainthreadobj));
#endif

    /* walk through all classloaders */
	for (i = 0; i < hashtable_classloader->size; i++) {
		cle = hashtable_classloader->ptr[i];

		while (cle) {

			GC_LOG2( printf("Found Classloader: %p\n", (void *) cle->object); );

			/* add this classloader to the root set */
			ROOTSET_ADD(&( cle->object ), true, REFTYPE_CLASSLOADER);

			cle = cle->hashlink;
		}
	}

	/* walk through all global references */
	for (i = 0; i < hashtable_global_ref->size; i++) {
		gre = hashtable_global_ref->ptr[i];

		while (gre) {

			GC_LOG2( printf("Found Global Reference: %p\n", (void *) gre->o); );

			/* add this global reference to the root set */
			ROOTSET_ADD(&( gre->o ), true, REFTYPE_GLOBALREF);

			gre = gre->hashlink;
		}
	}

	/* walk through all finalizer entries */
	/* REMEMBER: all threads are stopped, so we can use unsynced access here */
	fe = list_first_unsynced(final_list);
	while (fe) {

		GC_LOG2( printf("Found Finalizer Entry: %p\n", (void *) fe->o); );

		/* add this object with finalizer to the root set */
		ROOTSET_ADD(&( fe->o ), false, REFTYPE_FINALIZER)

		fe = list_next_unsynced(final_list, fe);
	}

	/* remeber how many references there are inside this root set */
	rs->refcount = refcount;

}


void rootset_from_classes(rootset_t *rs)
{
	classinfo         *c;
	fieldinfo         *f;
	void *sys_start, *sys_end;
	int refcount;
	int i;

	GC_LOG( dolog("GC: Acquiring Root-Set from classes ..."); );

	/* TODO: cleanup!!! */
	sys_start = heap_region_sys->base;
	sys_end = heap_region_sys->ptr;

	refcount = rs->refcount;

	/* walk through all classinfo blocks */
	for (c = sys_start; c < (classinfo *) sys_end; c++) {

		GC_LOG2( printf("Searching in class "); class_print(c); printf("\n"); );

		/* walk through all fields */
		f = c->fields;
		for (i = 0; i < c->fieldscount; i++, f++) {

			/* check if this is a static reference */
			if (!IS_ADR_TYPE(f->type) || !(f->flags & ACC_STATIC))
				continue;

			/* check for outside or null pointers */
			if (f->value.a == NULL)
				continue;

			GC_LOG2( printf("Found Static Field Reference: %p\n", (void *) f->value.a);
					printf("\tfrom field: "); field_print(f); printf("\n");
					printf("\tto object : "); heap_print_object(f->value.a); printf("\n"); );

			/* add this static field reference to the root set */
			ROOTSET_ADD(&( f->value ), true, REFTYPE_CLASSREF);

		}

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
	stackframeinfo   *sfi;
	executionstate_t *es;
	sourcestate_t    *ss;
	sourceframe_t    *sf;
	int refcount;
	int i;

#if defined(ENABLE_THREADS)
	GC_ASSERT(thread != NULL);
	GC_LOG( dolog("GC: Acquiring Root-Set from thread (tid=%p) ...", (void *) thread->tid); );
#else
	GC_ASSERT(thread == NULL);
	GC_LOG( dolog("GC: Acquiring Root-Set from single-thread ..."); );
#endif

	GC_LOG2( printf("Stacktrace of current thread:\n");
			stacktrace_dump_trace(thread); );

	/* get the stackframeinfo of the thread */
#if defined(ENABLE_THREADS)
	sfi = thread->_stackframeinfo;

	if (sfi == NULL) {
		/* TODO: what do we do here??? */
		GC_LOG( dolog("GC: NO SFI AVAILABLE!!!"); );
	}
#else
	sfi = NULL;
#endif

	/* create empty execution state */
	/* TODO: this is all wrong! */
	es = DNEW(executionstate_t);
	es->pc = 0;
	es->sp = 0;
	es->pv = 0;
	es->code = NULL;

	/* TODO: we assume we are in a native! */
	ss = replace_recover_source_state(NULL, sfi, es);

	/* print our full source state */
	GC_LOG2( replace_sourcestate_println(ss); );

	/* initialize the rootset struct */
	GC_ASSERT(rs);
	GC_ASSERT(rs->refcount == 0);
	rs->thread = thread;
	rs->ss = ss;
	rs->es = es;

	/* now inspect the source state to compile the root set */
	refcount = rs->refcount;
	for (sf = ss->frames; sf != NULL; sf = sf->down) {

		GC_ASSERT(sf->javastackdepth == 0);

		for (i = 0; i < sf->javalocalcount; i++) {

			/* we only need to consider references */
			if (sf->javalocaltype[i] != TYPE_ADR)
				continue;

			/* check for null pointer */
			if (sf->javalocals[i].a == NULL)
				continue;

			GC_LOG2( printf("Found Reference: %p\n", (void *) sf->javalocals[i].a); );

			/* add this reference to the root set */
			ROOTSET_ADD((java_objectheader **) &( sf->javalocals[i] ), true, REFTYPE_STACK);

		}
	}

	/* remeber how many references there are inside this root set */
	rs->refcount = refcount;

}


rootset_t *rootset_readout()
{
	rootset_t    *rs_top;
	rootset_t    *rs;
	threadobject *thread;

	/* find the global rootset ... */
	rs_top = rootset_create();
	rootset_from_globals(rs_top);
	rootset_from_classes(rs_top);

	/* ... and the rootsets for the threads */
	rs = rs_top;
#if defined(ENABLE_THREADS)
	thread  = mainthreadobj; /*THREADOBJECT*/
	do {
		rs->next = rootset_create();
		rs = rs->next;

		rootset_from_thread(thread, rs);

		thread = thread->next;
	} while ((thread != NULL) && (thread != mainthreadobj));
#else
	thread = THREADOBJECT;

	rs->next = rootset_create();
	rs = rs->next;

	rootset_from_thread(thread, rs);
#endif

	return rs_top;
}


void rootset_writeback(rootset_t *rs)
{
	threadobject     *thread;
	sourcestate_t    *ss;
	executionstate_t *es;

	/* TODO: only a dirty hack! */
	/*GC_ASSERT(rs->next->thread == THREADOBJECT);
	ss = rs->next->ss;
	es = rs->next->es;*/

	/* walk through all rootsets */
	while (rs) {
		thread = rs->thread;

		/* does this rootset belong to a thread? */
		if (thread != ROOTSET_DUMMY_THREAD) {
#if defined(ENABLE_THREADS)
			GC_ASSERT(thread != NULL);
			GC_LOG( dolog("GC: Writing back Root-Set to thread (tid=%p) ...", (void *) thread->tid); );
#else
			GC_ASSERT(thread == NULL);
			GC_LOG( dolog("GC: Writing back Root-Set to single-thread ..."); );
#endif

			/* now write back the modified sourcestate */
			ss = rs->ss;
			es = rs->es;
			replace_build_execution_state_intern(ss, es);
		}

		rs = rs->next;
	}

}


/* Debugging ******************************************************************/

#if !defined(NDEBUG)
const char* ref_type_names[] = {
		"XXXXXX", "THREAD", "CLASSL",
		"GLOBAL", "FINAL ", "LOCAL ",
		"STACK ", "STATIC"
};

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
			printf("%s ", ref_type_names[rs->ref_type[i]]);
			if (rs->ref_marks[i])
				printf("MARK+UPDATE");
			else
				printf("     UPDATE");
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
