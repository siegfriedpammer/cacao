/* mm/cacao-gc/heap.c - GC module for heap management

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

   $Id$

*/


#include "config.h"
#include "vm/types.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#else
# include "threads/none/lock.h"
#endif

#include "gc.h"
#include "heap.h"
#include "mark.h"
#include "region.h"
#include "mm/memory.h"
#include "src/native/include/java_lang_String.h" /* TODO: fix me! */
#include "toolbox/logging.h"
#include "vm/global.h"
#include "vmcore/rt-timing.h"


/* Global Variables ***********************************************************/

s4 heap_current_size;  /* current size of the heap */
s4 heap_maximal_size;  /* maximal size of the heap */
regioninfo_t *heap_region_sys;
regioninfo_t *heap_region_main;


/* Helper Macros **************************************************************/

#define GC_ALIGN_SIZE SIZEOF_VOID_P
#define GC_ALIGN(length,size) ((((length) + (size) - 1) / (size)) * (size))



void heap_init_objectheader(java_objectheader *o, u4 bytelength)
{
	u4 wordcount;

	/* initialize the header flags */
	o->hdrflags = 0;

	/* align the size */
	/* TODO */

	/* calculate the wordcount as stored in the header */
    /* TODO: improve this to save wordcount and without header bytes */
    if ((bytelength & 0x03) == 0) {
        GC_ASSERT((bytelength & 0x03) == 0);
        wordcount = (bytelength >> 2);
        GC_ASSERT(wordcount != 0);
    } else {
        wordcount = GC_SIZE_DUMMY;
    }

	/* set the wordcount in the header */
    if (wordcount >= GC_SIZE_DUMMY) {
        GC_SET_SIZE(o, GC_SIZE_DUMMY);
    } else {
        GC_SET_SIZE(o, wordcount);
    }

}


s4 heap_increase_size() {
	void *p;
	s4    increasesize;
	s4    newsize;

	/* TODO: locking for threads!!! */

	/* only a quick sanity check */
	GC_ASSERT(heap_current_size <= heap_maximal_size);

	/* check if we are allowed to enlarge the heap */
	if (heap_current_size == heap_maximal_size)
		vm_abort("heap_increase_size: reached maximal heap size: out of memory");

	/* TODO: find out how much to increase the heap??? */
	increasesize = heap_maximal_size - heap_current_size;
	GC_LOG( dolog("GC: Increasing Heap Size by %d", increasesize); );

	/* allocate new heap from the system */
	newsize = heap_current_size + increasesize;
	p = malloc(newsize);

	/* check if the newly allocated heap exists */
	if (p == NULL)
		vm_abort("heap_increase_size: malloc failed: out of memory");

	/* TODO: copy the old content to the new heap */
	/* TODO: find a complete rootset and update it to the new position */
	/* TODO: free the old heap */

	/* set the new values */
	/*heap_ptr = p + (heap_ptr - heap_base);
	heap_base = p;*/
	heap_current_size = newsize;

	GC_LOG( dolog("GC: Increasing Heap Size was successful");
			heap_println_usage(); );

	/* only a quick sanity check */
	GC_ASSERT(heap_current_size <= heap_maximal_size);

	return increasesize;
}


s4 heap_get_hashcode(java_objectheader *o)
{
	s4 hashcode;

	if (!o)
		return 0;

	/* TODO: we need to lock the object here i think!!! */

	/* check if there is a hash attached to this object */
	if (GC_TEST_FLAGS(o, HDRFLAG_HASH_ATTACHED)) {

		hashcode = *( (s4 *) ( ((u1 *) o) + get_object_size(o) - SIZEOF_VOID_P ) ); /* TODO: clean this up!!! */
		GC_LOG2( dolog("GC: Hash re-taken: %d (0x%08x)", hashcode, hashcode); );

	} else {

		GC_SET_FLAGS(o, HDRFLAG_HASH_TAKEN);

		hashcode = (s4) (ptrint) o;
		GC_LOG2( dolog("GC: Hash taken: %d (0x%08x)", hashcode, hashcode); );

	}

	return hashcode;
}


static java_objectheader *heap_alloc_intern(u4 bytelength, regioninfo_t *region)
{
	java_objectheader *p;

	/* only a quick sanity check */
	GC_ASSERT(region);
	GC_ASSERT(bytelength >= sizeof(java_objectheader));

	/* align objects in memory */
	bytelength = GC_ALIGN(bytelength, GC_ALIGN_SIZE);

	/* lock the region */
	LOCK_MONITOR_ENTER(region);

	/* check for sufficient free space */
	if (bytelength > region->free) {
		dolog("GC: Region out of memory!");
		gc_collect();
		return NULL;
	}

	/* allocate the object in this region */
	p = (java_objectheader *) region->ptr;
	region->ptr += bytelength;
	region->free -= bytelength;

	/* unlock the region */
	LOCK_MONITOR_EXIT(region);
	GC_LOG( region = NULL; );

	/* clear allocated memory region */
	GC_ASSERT(p);
	MSET(p, 0, u1, bytelength);

	/* set the header information */
	heap_init_objectheader(p, bytelength);

	return p;
}


/* heap_allocate ***************************************************************

   Allocates memory on the Java heap.

*******************************************************************************/

void *heap_allocate(u4 bytelength, u4 references, methodinfo *finalizer)
{
	java_objectheader *p;
#if defined(ENABLE_RT_TIMING)
	struct timespec time_start, time_end;
#endif

	RT_TIMING_GET_TIME(time_start);

	p = heap_alloc_intern(bytelength, heap_region_main);

	if (p == NULL)
		return NULL;

#if defined(GCCONF_HDRFLAG_REFERENCING)
	/* We can't use a bool here for references, as it's passed as a
	   bitmask in builtin_new.  Thus we check for != 0. */
	if (references != 0) {
		GC_SET_FLAGS(p, HDRFLAG_REFERENCING);
	}
#endif

	/* take care of finalization stuff */
	if (finalizer != NULL) {

		/* set the header bit */
		/* TODO: do we really need this??? */
		/* TODO: can this be overwritten by cloning??? */
		GC_SET_FLAGS(p, GC_FLAG_FINALIZER);

		/* register the finalizer for this object */
		final_register(p, finalizer);
	}

	RT_TIMING_GET_TIME(time_end);
	RT_TIMING_TIME_DIFF(time_start, time_end, RT_TIMING_GC_ALLOC);

	return p;
}


void *heap_alloc_uncollectable(u4 bytelength)
{
	java_objectheader *p;

	/* loader.c does this a lot for classes with fieldscount equal zero */
	if (bytelength == 0)
		return NULL;

	p = heap_alloc_intern(bytelength, heap_region_sys);

	if (p == NULL)
		return NULL;

	/* TODO: can this be overwritten by cloning??? */
	/* remember this object as uncollectable */
	GC_SET_FLAGS(p, HDRFLAG_UNCOLLECTABLE);

	return p;
}


void heap_free(void *p)
{
	GC_LOG( dolog("GC: Free %p", p); );
	GC_ASSERT(0);
}


/* Debugging ******************************************************************/

#if !defined(NDEBUG)
void heap_println_usage()
{
	printf("Current Heap Usage: Size=%d Free=%d\n",
			heap_current_size, heap_region_main->free);

	GC_ASSERT(heap_current_size == heap_region_main->size);
}
#endif


#if !defined(NDEBUG)
void heap_print_object_flags(java_objectheader *o)
{
	printf("0x%02x [%s%s%s%s%s]",
		GC_GET_SIZE(o),
		GC_TEST_FLAGS(o, GC_FLAG_FINALIZER)     ? "F" : " ",
		GC_TEST_FLAGS(o, HDRFLAG_HASH_ATTACHED) ? "A" : " ",
		GC_TEST_FLAGS(o, HDRFLAG_HASH_TAKEN)    ? "T" : " ",
		GC_TEST_FLAGS(o, HDRFLAG_UNCOLLECTABLE) ? "U" : " ",
		GC_TEST_FLAGS(o, GC_FLAG_MARKED)        ? "M" : " ");
}
#endif


#if !defined(NDEBUG)
void heap_print_object(java_objectheader *o)
{
	java_arrayheader  *a;
	classinfo         *c;

	/* check for null pointers */
	if (o == NULL) {
		printf("(NULL)");
		return;
	}

	/* print general information */
#if SIZEOF_VOID_P == 8
	assert(0);
#else
	printf("0x%08x: ", (void *) o);
#endif
	heap_print_object_flags(o);
	printf(" ");

	/* TODO */
	/* maybe this is not really an object */
	if (/*IS_CLASS*/ o->vftbl->class == class_java_lang_Class) {

		/* get the class information */
		c = (classinfo *) o;

		/* print the class information */
		printf("CLS ");
		class_print(c);

	} else if (/*IS_ARRAY*/ o->vftbl->arraydesc != NULL) {

		/* get the array information */
		a = (java_arrayheader *) o;
		c = o->vftbl->class;

		/* print the array information */
		printf("ARR ");
		/*class_print(c);*/
		utf_display_printable_ascii_classname(c->name);
		printf(" (size=%d)", a->size);

	} else /*IS_OBJECT*/ {

		/* get the object class */
		c = o->vftbl->class;

		/* print the object information */
		printf("OBJ ");
		/*class_print(c);*/
		utf_display_printable_ascii_classname(c->name);
		if (c == class_java_lang_String) {
			printf(" (string=\"");
			utf_display_printable_ascii(
					javastring_toutf((java_lang_String *) o, false));
			printf("\")");
		}

	}
}
#endif

#if !defined(NDEBUG)
void heap_dump_region(regioninfo_t *region, bool marked_only)
{
	java_objectheader *o;
	u4                 o_size;

	/* some basic sanity checks */
	GC_ASSERT(region->base < region->ptr);

	printf("Heap-Dump:\n");

	/* walk the region in a linear style */
	o = (java_objectheader *) region->base;
	while (o < region->ptr) {

		if (!marked_only || GC_IS_MARKED(o)) {
			printf("\t");
			heap_print_object(o);
			printf("\n");
		}

		/* get size of object */
		o_size = get_object_size(o);

		/* walk to next object */
		GC_ASSERT(o_size != 0);
		o = ((u1 *) o) + o_size;
	}

	printf("Heap-Dump finished.\n");
}
#endif


s4 get_object_size(java_objectheader *o)
{
	java_arrayheader *a;
	classinfo        *c;
	s4                o_size;

	/* we can assume someone initialized the header */
	GC_ASSERT(o->hdrflags != 0);

	/* get the wordcount from the header */
	o_size = GC_GET_SIZE(o);

	/* maybe we need to calculate the size by hand */
	if (o_size != GC_SIZE_DUMMY) {
		GC_ASSERT(o_size != 0);
		o_size = o_size << 2;
	} else {

		/* TODO */
		/* maybe this is not really an object */
		if (/*IS_CLASS*/ o->vftbl->class == class_java_lang_Class) {
			/* we know the size of a classinfo */
			o_size = sizeof(classinfo);

		} else if (/*IS_ARRAY*/ o->vftbl->arraydesc != NULL) {
			/* compute size of this array */
			a = (java_arrayheader *) o;
			c = o->vftbl->class;
			o_size = c->vftbl->arraydesc->dataoffset +
					a->size * c->vftbl->arraydesc->componentsize;

		} else /*IS_OBJECT*/ {
			/* get the object size */
			c = o->vftbl->class;
			o_size = c->instancesize;
			GC_LOG( dolog("Got size (from Class): %d bytes", o_size); );
		}
	
	}

	/* align the size */
	o_size = GC_ALIGN(o_size, GC_ALIGN_SIZE);

	/* the hashcode attached to this object might increase the size */
	if (GC_TEST_FLAGS(o, HDRFLAG_HASH_ATTACHED))
		o_size += SIZEOF_VOID_P;

	return o_size;
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
