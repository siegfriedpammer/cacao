/* mm/cacao-gc/mark.c - GC module for marking heap objects

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
#include "heap.h"
#include "mark.h"
#include "mm/memory.h"
#include "toolbox/logging.h"
#include "vm/global.h"
/*#include "vm/options.h"*/
#include "vm/jit/replace.h"
#include "vm/jit/stacktrace.h"
#include "vmcore/linker.h"


/* Debugging ******************************************************************/

/* TODO: move this whole thing to gc.c */
int mark_depth;
int mark_depth_max;
int mark_count;
#define MARK_COUNT_INIT { mark_count = 0; }
#define MARK_COUNT { mark_count++; }
#define MARK_DEPTH_INIT { mark_depth = 0; mark_depth_max = 0; }
#define MARK_DEPTH_INC { mark_depth++; if (mark_depth>mark_depth_max) mark_depth_max=mark_depth; }
#define MARK_DEPTH_DEC { mark_depth--; GC_ASSERT(mark_depth >= 0); }
void mark_println_stats()
{
	printf("Marking Statistics:\n");
	printf("\t# of objects marked: %d\n", mark_count);
	printf("\tMaximal marking depth: %d\n", mark_depth_max);
}


/* mark_rootset_from_globals ***************************************************

   Searches global variables to compile the global root set out of references
   contained in them.

   SEARCHES IN:
     - global reference table (jni.c)

*******************************************************************************/

void mark_rootset_from_globals(rootset_t *rs)
{
	GC_ASSERT(0);
}


/* mark_rootset_from_thread ****************************************************

   Searches the stack of the current thread for references and compiles a
   root set out of them.

   NOTE: uses dump memory!

   IN:
	  TODO!!!

   OUT:
	  TODO!!!

*******************************************************************************/

stacktracebuffer *stacktrace_create(threadobject* thread, bool rplpoints);
void replace_read_executionstate(rplpoint *rp,
                                        executionstate_t *es,
                                        sourcestate_t *ss,
                                        bool topframe);
void replace_write_executionstate(rplpoint *rp,
										 executionstate_t *es,
										 sourcestate_t *ss,
										 bool topframe);

void mark_rootset_from_thread(threadobject *thread, rootset_t *rs)
{
	stacktrace_entry *ste;
	stacktracebuffer *stb;
	executionstate_t *es;
	sourcestate_t    *ss;
	sourceframe_t    *sf;
	rplpoint         *rp;
	rplpoint         *rp_search;
	s4                rpcount;
	int               i;

	/* TODO: remove these */
    java_objectheader *o;
    int refcount;

	GC_LOG( printf("Walking down stack of current thread:\n");
			stacktrace_dump_trace(thread); );

#if 0
	/* get the stacktrace with replacement points */
	/* NOTE: stacktrace_create uses dump memory! */
	stb = stacktrace_create(thread, true);
	GC_ASSERT(stb);

	/* this is where our source state will be placed */
	ss = DNEW(sourcestate_t);
	ss->frames = NULL;

	/* walk through the stacktracebuffer to find source state for all methods */
	ste = &(stb->entries[0]);
	for (i = 0; i < stb->used; i++, ste++) {

		/* try to get an execution state for this entry */
		es = ste->es;
		if (!es)
			continue;

		/* get the replacement points for this code */
		GC_ASSERT(es->code);
		rp = es->code->rplpoints;
		GC_ASSERT(rp);

		/* search for the replacement point right in front of the return
		   address. this should be the one associated with the taken call. */
		rp_search = rp;
		rp = NULL;
		for (rpcount = 0; rpcount < es->code->rplpointcount; rpcount++) {

			GC_LOG2( printf("(%d) Checking %p > %p\n",
					rpcount, es->pc, rp_search->pc); );

			if (rp_search->pc < es->pc)
				rp = rp_search;

			rp_search++;
		}

		/* this is the replacement point of our call */
		GC_ASSERT(rp);
		GC_ASSERT(rp->type == RPLPOINT_TYPE_CALL);
		GC_ASSERT(rp->pc < es->pc);
		GC_ASSERT(rp->pc + rp->callsize >= es->pc);

		/* add this execution state to the sourcestate */
		replace_read_executionstate(rp, es, ss, false);

		/* print our results */
		GC_LOG(
				printf("Found RPL point:\n");
				replace_replacement_point_println(rp, 1);
				replace_executionstate_println(es);
		);

	}
#else
	/* create empty execution state */
	es = DNEW(executionstate_t);
	es->pc = 0;
	es->sp = 0;
	es->pv = 0;
	es->code = NULL;

	/* TODO: we assume we are in a native and in current thread! */
	ss = replace_recover_source_state(NULL, NULL, es);
#endif

	/* print our full source state */
	GC_LOG( replace_sourcestate_println(ss); );

	/* initialize the rootset struct */
	GC_ASSERT(rs);
	rs->thread = thread;
	rs->ss = ss;
	rs->es = es;
	rs->stb = stb;
	rs->refcount = 0;

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

			refcount++;

		}

	}

	/* remeber how many references there are inside this root set */
	rs->refcount = refcount;

	GC_LOG( printf("Walking done.\n"); );
}


void mark_rootset_writeback(rootset_t *rs)
{
	sourcestate_t    *ss;
	executionstate_t *es;
	rplpoint         *rp;
	stacktracebuffer *stb;
	stacktrace_entry *ste;
	int i;

#if 0
	ss = rs->ss;
	stb = rs->stb;

	/* walk through the stacktracebuffer to find execution state
	 * for all methods */
	i = stb->used;
	while (i > 0) {
		i--;
		ste = &(stb->entries[i]);

		/* try to get an execution state for this entry */
		es = ste->es;
		if (!es)
			continue;

		/* write back the execution state at this replacement point */
		rp = ss->frames->fromrp;
		replace_write_executionstate(rp, es, ss, false);

	}
#else
	ss = rs->ss;
	es = rs->es;

	replace_build_execution_state_intern(ss, es);
#endif

}


/* mark_recursice **************************************************************

   Recursively mark all objects (including this) which are referenced.

   IN:
	  o.....heap-object to be marked (either OBJECT or ARRAY)

*******************************************************************************/

void mark_recursive(java_objectheader *o)
{
	vftbl_t           *t;
	classinfo         *c;
	fieldinfo         *f;
	java_objectarray  *oa;
	arraydescriptor   *desc;
	java_objectheader *ref;
	void *start, *end;
	int i;

	/* TODO: this needs cleanup!!! */
	start = heap_region_main->base;
	end = heap_region_main->ptr;

	/* uncollectable objects should never get marked this way */
	/* the reference should point into the heap */
	GC_ASSERT(o);
	GC_ASSERT(!GC_TEST_FLAGS(o, GC_FLAG_UNCOLLECTABLE));
	GC_ASSERT(POINTS_INTO(o, start, end));

	/* mark this object */
	GC_SET_MARKED(o);
	MARK_COUNT;

	/* get the class of this object */
	/* TODO: maybe we do not need this yet, look to move down! */
	t = o->vftbl;
	GC_ASSERT(t);
	c = t->class;
	GC_ASSERT(c);

	/* TODO: should we mark the class of the object as well? */
	/*GC_ASSERT(GC_IS_MARKED((java_objectheader *) c));*/

	/* does this object has pointers? */
	/* TODO: check how often this happens, maybe remove this check! */
	/*if (!GC_IS_REFERENCING(o))
		return;*/

	/* check if we are marking an array */
	if ((desc = t->arraydesc) != NULL) {
		/* this is an ARRAY */

		/* check if the array contains references */
		if (desc->arraytype != ARRAYTYPE_OBJECT)
			return;

		/* for object-arrays we need to check every entry */
		oa = (java_objectarray *) o;
		for (i = 0; i < oa->header.size; i++) {

			/* load the reference value */
			ref = (java_objectheader *) (oa->data[i]);

			/* check for outside or null pointers */
			if (!POINTS_INTO(ref, start, end))
				continue;

			GC_LOG2( printf("Found (%p) from Array\n", (void *) ref); );

			/* do the recursive marking */
			if (!GC_IS_MARKED(ref)) {
				MARK_DEPTH_INC;
				mark_recursive(ref);
				MARK_DEPTH_DEC;
			}

		}

	} else {
		/* this is an OBJECT */

		/* for objects we need to check all (non-static) fields */
		for (; c; c = c->super.cls) {
		for (i = 0; i < c->fieldscount; i++) {
			f = &(c->fields[i]);

			/* check if this field contains a non-static reference */
			if (!IS_ADR_TYPE(f->type) || (f->flags & ACC_STATIC))
				continue;

			/* load the reference value */
			ref = *( (java_objectheader **) ((s1 *) o + f->offset) );

			/* check for outside or null pointers */
			if (!POINTS_INTO(ref, start, end))
				continue;

			GC_LOG2( printf("Found (%p) from Field ", (void *) ref);
					field_print(f); printf("\n"); );

			/* do the recursive marking */
			if (!GC_IS_MARKED(ref)) {
				MARK_DEPTH_INC;
				mark_recursive(ref);
				MARK_DEPTH_DEC;
			}

		}
		}

	}

}


void mark_classes(void *start, void *end)
{
	java_objectheader *ref;
	classinfo         *c;
	fieldinfo         *f;
	void *sys_start, *sys_end;
	int i;

	GC_LOG( printf("Marking from classes\n"); );

	/* TODO: cleanup!!! */
	sys_start = heap_region_sys->base;
	sys_end = heap_region_sys->ptr;

	/* walk through all classinfo blocks */
	for (c = sys_start; c < (classinfo *) sys_end; c++) {

		/* walk through all fields */
		f = c->fields;
		for (i = 0; i < c->fieldscount; i++, f++) {

			/* check if this is a static reference */
			if (!IS_ADR_TYPE(f->type) || !(f->flags & ACC_STATIC))
				continue;

			/* load the reference */
			ref = (java_objectheader *) (f->value.a);

			/* check for outside or null pointers */
			if (!POINTS_INTO(ref, start, end))
				continue;

			/* mark the reference */
			mark_recursive(ref);

		}

	}

}


/* mark_me *********************************************************************

   Marks all Heap Objects which are reachable from a given root-set.

   IN:
	  rs.....root set containing the references

*******************************************************************************/

void mark_me(rootset_t *rs)
{
	java_objectheader *ref;
	int i;

	MARK_COUNT_INIT;
	MARK_DEPTH_INIT;	

	/* recursively mark all references from classes */
	mark_classes(heap_region_main->base, heap_region_main->ptr);

	GC_LOG( printf("Marking from rootset (%d entries)\n", rs->refcount); );

	/* recursively mark all references of the rootset */
	MARK_DEPTH_INC;
	for (i = 0; i < rs->refcount; i++) {

		ref = *( rs->refs[i] );
		mark_recursive(ref);

	}
	MARK_DEPTH_DEC;

	GC_ASSERT(mark_depth == 0);
	GC_ASSERT(mark_depth_max > 0);

	GC_LOG( mark_println_stats(); );
}


#if !defined(NDEBUG)
void mark_rootset_print(rootset_t *rs)
{
	java_objectheader *o;
	int i;

	printf("Root Set:\n");

	printf("\tThread: %p\n", rs->thread);
	printf("\tReferences (%d):\n", rs->refcount);

	for (i = 0; i < rs->refcount; i++) {

		o = *( rs->refs[i] );

		/*printf("\t\tReference at %p points to ...\n", (void *) rs->refs[i]);*/
		printf("\t\t");
		heap_print_object(o);
		printf("\n");

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
