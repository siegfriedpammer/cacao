/* src/mm/boehm.c - interface for boehm gc

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
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

   Authors: Stefan Ring

   Changes: Christian Thalinger

   $Id: boehm.c 5144 2006-07-17 11:09:21Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#if defined(ENABLE_THREADS) && defined(__LINUX__)
#define GC_LINUX_THREADS
#endif
#if defined(ENABLE_THREADS) && defined(__IRIX__)
#define GC_IRIX_THREADS
#endif

#include "boehm-gc/include/gc.h"
#include "mm/boehm.h"
#include "mm/memory.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "toolbox/logging.h"
#include "vm/options.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/finalizer.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"


/* global variables ***********************************************************/

static bool in_gc_out_of_memory = false;    /* is GC out of memory?           */


/* prototype static functions *************************************************/

static void gc_ignore_warnings(char *msg, GC_word arg);


/* gc_init *********************************************************************

   Initializes the boehm garbage collector.

*******************************************************************************/

void gc_init(u4 heapmaxsize, u4 heapstartsize)
{
	size_t heapcurrentsize;

	GC_INIT();

	/* set the maximal heap size */

	GC_set_max_heap_size(heapmaxsize);

	/* set the initial heap size */

	heapcurrentsize = GC_get_heap_size();

	if (heapstartsize > heapcurrentsize) {
		GC_expand_hp(heapstartsize - heapcurrentsize);
	}

	/* define OOM function */

	GC_oom_fn = gc_out_of_memory;

	/* just to be sure (should be set to 1 by JAVA_FINALIZATION macro) */

	GC_java_finalization = 1;

	/* suppress warnings */

	GC_set_warn_proc(gc_ignore_warnings);

	/* install a GC notifier */

	GC_finalize_on_demand = 1;
	GC_finalizer_notifier = finalizer_notify;
}


static void gc_ignore_warnings(char *msg, GC_word arg)
{
}


void *heap_alloc_uncollectable(u4 bytelength)
{
	void *p;

	p = GC_MALLOC_UNCOLLECTABLE(bytelength);

	/* clear allocated memory region */

	MSET(p, 0, u1, bytelength);

	return p;
}


/* heap_allocate ***************************************************************

   Allocates memory on the Java heap.

*******************************************************************************/

void *heap_allocate(u4 bytelength, u4 references, methodinfo *finalizer)
{
	void *p;

	/* We can't use a bool here for references, as it's passed as a
	   bitmask in builtin_new.  Thus we check for != 0. */

	if (references != 0)
		p = GC_MALLOC(bytelength);
	else
		p = GC_MALLOC_ATOMIC(bytelength);

	if (p == NULL)
		return NULL;

	if (finalizer != NULL)
		GC_REGISTER_FINALIZER(p, finalizer_run, 0, 0, 0);

	/* clear allocated memory region */

	MSET(p, 0, u1, bytelength);

	return p;
}


void heap_free(void *p)
{
	GC_FREE(p);
}

void gc_call(void)
{
  	if (opt_verbosegc)
		dolog("Garbage Collection:  previous/now = %d / %d ",
			  0, 0);

	GC_gcollect();
}


s8 gc_get_heap_size(void)
{
	return GC_get_heap_size();
}


s8 gc_get_free_bytes(void)
{
	return GC_get_free_bytes();
}


s8 gc_get_max_heap_size(void)
{
	return GC_get_max_heap_size();
}


void gc_invoke_finalizers(void)
{
	GC_invoke_finalizers();
}


void gc_finalize_all(void)
{
	GC_finalize_all();
}


/* gc_out_of_memory ************************************************************

   This function is called when boehm detects that it is OOM.

*******************************************************************************/

void *gc_out_of_memory(size_t bytes_requested)
{
	/* if this happens, we are REALLY out of memory */

	if (in_gc_out_of_memory) {
		/* this is all we can do... */
		exceptions_throw_outofmemory_exit();
	}

	in_gc_out_of_memory = true;

	/* try to release some memory */

	gc_call();

	/* now instantiate the exception */

	*exceptionptr = new_exception(string_java_lang_OutOfMemoryError);

	in_gc_out_of_memory = false;

	return NULL;
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
