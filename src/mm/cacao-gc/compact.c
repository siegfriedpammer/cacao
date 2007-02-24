/* mm/cacao-gc/compact.c - GC module for compacting heap regions

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
#include "vm/types.h"

#include "gc.h"
#include "heap.h"
#include "mark.h"
#include "mm/memory.h"
#include "toolbox/logging.h"


/* Threading macros ***********************************************************/

#define GC_THREAD_BIT 0x01

#define GC_IS_THREADED(ptr)       (((ptrint) ptr) & GC_THREAD_BIT)
#define GC_REMOVE_THREAD_BIT(ptr) (((ptrint) ptr) & ~GC_THREAD_BIT)
#define GC_SET_THREAD_BIT(ptr)    (((ptrint) ptr) | GC_THREAD_BIT)

#define GC_THREAD(ref, refptr, start, end) \
	if (POINTS_INTO(ref, start, end)) { \
		GC_ASSERT(GC_IS_MARKED(ref)); \
		*refptr = (java_objectheader *) ref->vftbl; \
		ref->vftbl = (struct _vftbl *)  GC_SET_THREAD_BIT(refptr); \
	}


/* compact_thread_rootset ******************************************************

   Threads all the references in the rootset

   IN:
      rs........Rootset containing the references to be threaded.
      start.....Region to be compacted start here
      end.......Region to be compacted ends here 

*******************************************************************************/

void compact_thread_rootset(rootset_t *rs, void *start, void *end)
{
	java_objectheader  *ref;
	java_objectheader **refptr;
	int i;

	GC_LOG2( printf("threading in rootset\n"); );

	/* walk through the references of this rootset */
	for (i = 0; i < rs->refcount; i++) {

		/* load the reference */
		refptr = rs->refs[i];
		ref = *( refptr );

		GC_LOG2( printf("\troot pointer to %p\n", (void *) ref); );

		/* thread the references */
		GC_THREAD(ref, refptr, start, end);

	}
}


/* compact_thread_classes ******************************************************

   Threads all the references from classinfo structures (static fields and
   classloaders)

   IN:
      start.....Region to be compacted start here
      end.......Region to be compacted ends here 

*******************************************************************************/

void compact_thread_classes(void *start, void *end)
{
	java_objectheader  *ref;
	java_objectheader **refptr;
	classinfo          *c;
	fieldinfo          *f;
	hashtable_classloader_entry *cle;
	void *sys_start, *sys_end;
	int i;

	GC_LOG2( printf("threading in classes\n"); );

	/* TODO: cleanup!!! */
	sys_start = heap_region_sys->base;
	sys_end = heap_region_sys->ptr;

	/* walk through all classloaders */
	for (i = 0; i < hashtable_classloader->size; i++) {
		cle = hashtable_classloader->ptr[i];

		while (cle) {

			/* thread the classloader */
			refptr = &( cle->object );
			ref = *( refptr );
			GC_LOG2( printf("\tclassloader from hashtable at %p\n", (void *) ref); );
			GC_THREAD(ref, refptr, start, end);

			cle = cle->hashlink;
		}
	}

	/* walk through all classinfo blocks */
	for (c = sys_start; c < (classinfo *) sys_end; c++) {

		/* thread the classloader */
		/*refptr = &( c->classloader );
		ref = *( refptr );
		GC_LOG2( printf("\tclassloader from classinfo at %p\n", (void *) ref); );
		GC_THREAD(ref, refptr, start, end);*/

		/* walk through all fields */
		f = c->fields;
		for (i = 0; i < c->fieldscount; i++, f++) {

			/* check if this is a static reference */
			if (!IS_ADR_TYPE(f->type) || !(f->flags & ACC_STATIC))
				continue;

			/* load the reference */
			refptr = (java_objectheader **) &(f->value);
			ref = *( refptr );

			GC_LOG2( printf("\tclass-field points to %p\n", (void *) ref); );
			/*GC_LOG2(
				printf("\tfield: "); field_print(f); printf("\n");
				printf("\tclass-field points to ");
				if (ref == NULL) {
					printf("(NULL)\n");
				} else if (GC_IS_THREADED(ref->vftbl)) {
					printf("(threaded)\n");
				} else {
					heap_print_object(ref); printf("\n");
				}
			);*/

			/* thread the reference */
			GC_THREAD(ref, refptr, start, end);

		}

	}

}


/* compact_thread_references ***************************************************

   Threads all the references of an object.

   IN:
      o.........Object containing the references to be threaded.
      start.....Region to be compacted start here
      end.......Region to be compacted ends here 

*******************************************************************************/

void compact_thread_references(java_objectheader *o, void *start, void *end)
{
	java_objectheader  *ref;
	java_objectheader **refptr;

	GC_LOG2( printf("threading in ");
			heap_print_object(o); printf("\n"); );

	if (IS_ARRAY(o)) {

		/* walk through the references of an Array */
		FOREACH_ARRAY_REF(o,ref,refptr,

			GC_LOG2( printf("\tarray-entry points to %p\n", (void *) ref); );

			GC_THREAD(ref, refptr, start, end);

		);

	} else {

		/* walk through the references of an Object */
		FOREACH_OBJECT_REF(o,ref,refptr,

			GC_LOG2( printf("\tobject-field points to %p\n", (void *) ref); );

			GC_THREAD(ref, refptr, start, end);

		);

	}
}


/* compact_unthread_references *************************************************

   Unthreads all the references which previousely pointed to an object. The
   object itself is the head of the threading chain. All the references in the
   chain once pointed to the object and will point to it's new location
   afterwards.

   IN:
      o.......Head of the threading chain
      new.....New Location of the object after compaction

*******************************************************************************/

void compact_unthread_references(java_objectheader *o, void *new)
{
	java_objectheader **refptr;
	ptrint tmp;

	GC_LOG2( printf("unthreading in ...\n"); );

	/* some quick sanity checks */
	GC_ASSERT(o);
	GC_ASSERT(GC_IS_THREADED(o->vftbl));

	/* walk down the threaded chain */
	refptr = (java_objectheader **) (ptrint) o->vftbl;
	while (GC_IS_THREADED(refptr)) {

		/* remove the threading bit */
		refptr = (java_objectheader **) GC_REMOVE_THREAD_BIT(refptr);

		GC_LOG2( printf("\treference at %p\n", (void *) refptr); );

		/* update the reference in the chain */
		tmp = (ptrint) *refptr;
		*refptr = (java_objectheader *) (ptrint) new;

		/* skip to the next chain value */
		refptr = (java_objectheader **) tmp;

	}

	/* finally restore the original vftbl pointer */
	o->vftbl = (struct _vftbl *) refptr;
	GC_ASSERT(o->vftbl);

	GC_LOG2( printf("\t... pointed to "); heap_print_object(o); printf("\n"); );
	GC_LOG2( printf("\t... now points to %p\n", (void *) new); );

}


/* compact_move ****************************************************************

   Moves the content (including header) of an object around in memory

   NOTE: Memory locations may overlap!
   NOTE: The size of the object can change by moving it around (hashcode)!

   IN:
      old......Old Location of the object before compaction
      new......New Location of the object after compaction
      size.....Size of the object in bytes

   OUT:
      New size of the object after moving it

*******************************************************************************/

u4 compact_move(u1 *old, u1 *new, u4 size)
{
	s4 hashcode;
	u4 new_size;

	GC_ASSERT(new < old);

	/* check if locations overlap */
	if (new + size < old) {
		/* overlapping: NO */

		/* copy old object content to new location */
		MCOPY(new, old, u1, size);

#if !defined(NDEBUG)
		/* invalidate old object */
		MSET(old, MEMORY_CLEAR_BYTE, u1, size);
#endif

	} else {
		/* overlapping: YES */

		GC_LOG2( printf("\tcompact_move(old=%p, new=%p, size=0x%x) overlapps!", old, new, size) );

		/* copy old object content to new location */
		MMOVE(new, old, u1, size);

	}

	new_size = size;

	/* check if we need to attach the hashcode to the object */
	if (GC_TEST_FLAGS((java_objectheader *) new, HDRFLAG_HASH_TAKEN)) {

		/* change the flags accordingly */
		GC_CLEAR_FLAGS((java_objectheader *) new, HDRFLAG_HASH_TAKEN);
		GC_SET_FLAGS((java_objectheader *) new, HDRFLAG_HASH_ATTACHED);

		/* attach the hashcode at the end of the object */
		new_size += SIZEOF_VOID_P;
		hashcode = (s4) (ptrint) old;
		*( (s4 *) (new + new_size - SIZEOF_VOID_P) ) = hashcode; /* TODO: clean this up */

		GC_ASSERT(new + SIZEOF_VOID_P < old);
		GC_LOG( dolog("GC: Hash attached: %d (0x%08x) to new object at %p", hashcode, hashcode, new); );

	}

	return new_size;
}


/* compact_me ******************************************************************

   This function actually does the compaction in two passes. Look at the source
   for further details about the passes.

   IN:
      rs.........Rootset, needed to update the root references
      region.....Region to be compacted

*******************************************************************************/

void compact_me(rootset_t *rs, regioninfo_t *region)
{
	u1 *ptr;
	u1 *ptr_new;
	java_objectheader *o;
	u4 o_size;
	u4 o_size_new;
	u4 used;

	GC_LOG( dolog("GC: Compaction Phase 1 started ..."); );

	/* Phase 0:
	 *  - thread all references in classes
	 *  - thread all references in the rootset */
	compact_thread_classes(region->base, region->ptr);
	compact_thread_rootset(rs, region->base, region->ptr);

	/* Phase 1:
	 *  - scan the heap
	 *  - thread all references
	 *  - update forward references */
	ptr = region->base; ptr_new = region->base;
	while (ptr < region->ptr) {
		o = (java_objectheader *) ptr;

		/* uncollectable items should never be compacted */
		GC_ASSERT(!GC_TEST_FLAGS(o, GC_FLAG_UNCOLLECTABLE));

		/* if this object is already part of a threaded chain ... */
		if (GC_IS_THREADED(o->vftbl)) {

			/* ... unthread the reference chain */
			compact_unthread_references(o, ptr_new);

		}

		/* get the object size (objectheader is now unthreaded) */
		o_size = get_object_size(o);

		/* only marked objects survive */
		if (GC_IS_MARKED(o)) {

			/* thread all the references in this object */
			compact_thread_references(o, region->base, region->ptr);

			/* object survives, place next object behind it */
			ptr_new += o_size;

			/* size might change because of attached hashcode */
			if (GC_TEST_FLAGS(o, HDRFLAG_HASH_TAKEN))
				ptr_new += SIZEOF_VOID_P;
		}

		/* skip to next object */
		ptr += o_size;
	}

	GC_LOG( dolog("GC: Compaction Phase 2 started ..."); );

	/* Phase 2:
	 *  - scan the heap again
	 *  - update backward references
	 *  - move the objects */
	used = 0;
	ptr = region->base; ptr_new = region->base;
	while (ptr < region->ptr) {
		o = (java_objectheader *) ptr;

		/* if this object is still part of a threaded chain ... */
		if (GC_IS_THREADED(o->vftbl)) {

			/* ... unthread the reference chain */
			compact_unthread_references(o, ptr_new);

		}

		/* get the object size (objectheader is now unthreaded) */
		o_size = get_object_size(o);

		/* move the surviving objects */
		if (GC_IS_MARKED(o)) {

			GC_LOG2( printf("moving: %08x -> %08x (%d bytes)\n",
					(ptrint) ptr, (ptrint) ptr_new, o_size); );

			/* unmark the object */
			GC_CLEAR_MARKED(o);

			/* move the object (size can change) */
			o_size_new = compact_move(ptr, ptr_new, o_size);

			/* object survives, place next object behind it */
			ptr_new += o_size_new;
			used += o_size_new;
		}

		/* skip to next object */
		ptr += o_size;
	}

	GC_LOG( dolog("GC: Compaction finished."); );

	GC_LOG( printf("Region-Used: %d -> %d\n", region->size - region->free, used); )
	GC_LOG( printf("Region-Free: %d -> %d\n", region->free, region->size - used); )

	/* update the region information */
	region->ptr = ptr_new;
	region->free = region->size - used;

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
