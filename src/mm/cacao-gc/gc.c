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
#include "final.h"
#include "gc.h"
#include "heap.h"
#include "mark.h"
#include "region.h"
#include "rootset.h"
#include "mm/memory.h"
#include "toolbox/logging.h"
#include "vm/finalizer.h"
#include "vm/vm.h"


/* Global Variables ***********************************************************/

bool gc_running;
bool gc_notify_finalizer;


/* gc_init *********************************************************************

   Initializes the garbage collector.

*******************************************************************************/

#define GC_SYS_SIZE (20*1024*1024)

void gc_init(u4 heapmaxsize, u4 heapstartsize)
{
	if (opt_verbosegc)
		dolog("GC: Initialising with heap-size %d (max. %d)",
			heapstartsize, heapmaxsize);

	/* finalizer stuff */
	final_init();

	/* set global variables */
	gc_running = false;

	/* region for uncollectable objects */
	heap_region_sys = NEW(regioninfo_t);
	if (!region_create(heap_region_sys, GC_SYS_SIZE))
		vm_abort("gc_init: region_create failed: out of memory");

	/* region for java objects */
	heap_region_main = NEW(regioninfo_t);
	if (!region_create(heap_region_main, heapstartsize))
		vm_abort("gc_init: region_create failed: out of memory");

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
	s4            dumpsize;

	/* remember start of dump memory area */
	dumpsize = dump_size();

	/* finalizer is not notified, unless marking tells us to do so */
	gc_notify_finalizer = false;

	GC_LOG( heap_println_usage(); );
	/*GC_LOG( heap_dump_region(heap_base, heap_ptr, false); );*/

	/* find the global rootset and the rootset for the current thread */
	rs = rootset_create();
	rootset_from_globals(rs);
	rs->next = rootset_create();
	rootset_from_thread(THREADOBJECT, rs->next);
	GC_LOG( rootset_print(rs); );

	/* check for reentrancy here */
	if (gc_running) {
		dolog("GC: REENTRANCY DETECTED, aborting ...");
		goto gc_collect_abort;
	}

	/* once the rootset is complete, we consider ourselves running */
	gc_running = true;

#if 1

	/* mark the objects considering the given rootset */
	mark_me(rs);
	/*GC_LOG( heap_dump_region(heap_region_main, true); );*/

	/* compact the heap */
	compact_me(rs, heap_region_main);
	/*GC_LOG( heap_dump_region(heap_region_main, false); );
	GC_LOG( rootset_print(rs); );*/

#if defined(ENABLE_MEMCHECK)
	/* invalidate the rest of the main region */
	region_invalidate(heap_region_main);
#endif

#else

	/* copy the heap to new region */
	{
		regioninfo_t *src, *dst;

		dst = DNEW(regioninfo_t);
		region_init(dst, heap_current_size);
		gc_copy(heap_region_main, dst, rs);
	}

	/* invalidate old heap */
	/*memset(heap_base, 0x5a, heap_current_size);*/

#endif

	/* TODO: check my return value! */
	/*heap_increase_size();*/

	/* write back the rootset to update root references */
	GC_LOG( rootset_print(rs); );
	rootset_writeback(rs);

#if defined(ENABLE_STATISTICS)
	if (opt_verbosegc)
		gcstat_println();
#endif

#if defined(GCCONF_FINALIZER)
	/* does the finalizer need to be notified */
	if (gc_notify_finalizer)
		finalizer_notify();
#endif

	/* we are no longer running */
	/* REMEBER: keep this below the finalizer notification */
	gc_running = false;

gc_collect_abort:
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


/* gc_invoke_finalizers ********************************************************

   Forces invocation of all the finalizers for objects which are reclaimable.
   This is the function which is called by the finalizer thread.

*******************************************************************************/

void gc_invoke_finalizers(void)
{
	if (opt_verbosegc)
		dolog("GC: Invoking finalizers ...");

	final_invoke();

	if (opt_verbosegc)
		dolog("GC: Invoking finalizers finished.");
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
