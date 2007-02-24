/* src/mm/cacao-gc/gc.c - main garbage collector methods

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
#include <signal.h>
#include "vm/types.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#else
/*# include "threads/none/threads.h"*/
#endif

#include "compact.h"
#include "gc.h"
#include "heap.h"
#include "mark.h"
#include "region.h"
#include "mm/memory.h"
#include "toolbox/logging.h"
#include "vm/exceptions.h"
/*#include "vm/options.h"*/


/* gc_init *********************************************************************

   Initializes the garbage collector.

*******************************************************************************/

#define GC_SYS_SIZE (20*1024*1024)

void gc_init(u4 heapmaxsize, u4 heapstartsize)
{
	if (opt_verbosegc)
		dolog("GC: Initialising with heap-size %d (max. %d)",
			heapstartsize, heapmaxsize);

	/* region for uncollectable objects */
	heap_region_sys = NEW(regioninfo_t);
	if (!region_create(heap_region_sys, GC_SYS_SIZE))
		exceptions_throw_outofmemory_exit();

	/* region for java objects */
	heap_region_main = NEW(regioninfo_t);
	if (!region_create(heap_region_main, heapstartsize))
		exceptions_throw_outofmemory_exit();

	heap_current_size = heapstartsize;
	heap_maximal_size = heapmaxsize;

	/* TODO: remove these */
	heap_free_size = heap_current_size;
	heap_used_size = 0;
}


/* gc_collect ******************************************************************

   This is the main machinery which manages a collection. It should be run by
   the thread which triggered the collection.

   IN:
     XXX

   STEPS OF A COLLECTION:
     XXX

*******************************************************************************/

void gc_collect(s4 level)
{
	rootset_t    *rs;
	regioninfo_t *src, *dst;
	s4            dumpsize;

	/* remember start of dump memory area */
	dumpsize = dump_size();

	GC_LOG( heap_println_usage(); );
	/*GC_LOG( heap_dump_region(heap_base, heap_ptr, false); );*/

	/* find the global rootset and the rootset for the current thread */
	rs = DNEW(rootset_t);
	/*TODO: mark_rootset_create(rs);*/
	/*TODO: mark_rootset_from_globals(rs);*/
	mark_rootset_from_thread(THREADOBJECT, rs);
	GC_LOG( mark_rootset_print(rs); );

#if 1

	/* mark the objects considering the given rootset */
	mark_me(rs);
	/*GC_LOG( heap_dump_region(heap_region_main, true); );*/

	/* compact the heap */
	compact_me(rs, heap_region_main);
	/*GC_LOG( heap_dump_region(heap_region_main, false); );
	GC_LOG( mark_rootset_print(rs); );*/

#if defined(ENABLE_MEMCHECK)
	/* invalidate the rest of the main region */
	region_invalidate(heap_region_main);
#endif

#else

	/* copy the heap to new region */
	dst = DNEW(regioninfo_t);
	region_init(dst, heap_current_size);
	gc_copy(heap_region_main, dst, rs);

	/* invalidate old heap */
	/*memset(heap_base, 0x5a, heap_current_size);*/

#endif

	/* TODO: check my return value! */
	/*heap_increase_size();*/

	/* write back the rootset to update root references */
	GC_LOG( mark_rootset_print(rs); );
	mark_rootset_writeback(rs);

#if defined(ENABLE_STATISTICS)
	if (opt_verbosegc)
		gcstat_println();
#endif

    /* free dump memory area */
    dump_release(dumpsize);

}


/* gc_call *********************************************************************

   Forces a full collection of the whole Java Heap.
   This is the function which is called by System.VMRuntime.gc()

*******************************************************************************/

void gc_call(void)
{
	if (opt_verbosegc)
		dolog("GC: Forced Collection ...");

	gc_collect(0);

	if (opt_verbosegc)
		dolog("GC: Forced Collection finished.");
}


/* Informational getter functions *********************************************/

s8 gc_get_heap_size(void)     { return heap_current_size; }
s8 gc_get_free_bytes(void)    { return heap_free_size; }
s8 gc_get_total_bytes(void)   { return heap_used_size; }
s8 gc_get_max_heap_size(void) { return heap_maximal_size; }


/* Thread specific stuff ******************************************************/

#if defined(ENABLE_THREADS)
int GC_signum1()
{
	return SIGUSR1;
}

int GC_signum2()
{
	return SIGUSR2;
}
#endif


/* Statistics *****************************************************************/

#if defined(ENABLE_STATISTICS)
int gcstat_mark_depth;
int gcstat_mark_depth_max;
int gcstat_mark_count;

void gcstat_println()
{
    printf("\nGCSTAT - Marking Statistics:\n");
    printf("\t# of objects marked: %d\n", gcstat_mark_count);
    printf("\tMaximal marking depth: %d\n", gcstat_mark_depth_max);

	printf("\nGCSTAT - Compaction Statistics:\n");

	printf("\n");
}
#endif /* defined(ENABLE_STATISTICS) */


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
