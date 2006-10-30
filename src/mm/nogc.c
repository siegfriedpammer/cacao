/* src/mm/nogc.c - allocates memory through malloc (no GC)

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

   Authors: Christian Thalinger

   Changes:

   $Id: nogc.c 5868 2006-10-30 11:21:36Z edwin $

*/


#include "config.h"

#include <stdlib.h>
#include <sys/mman.h>

#include "vm/types.h"

#include "boehm-gc/include/gc.h"

#include "mm/boehm.h"
#include "mm/memory.h"
#include "toolbox/logging.h"
#include "vm/options.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"


/* global stuff ***************************************************************/

#define MMAP_HEAPADDRESS    0x10000000  /* try to map the heap to this addr.  */
#define ALIGNSIZE           8

static void *mmapptr = NULL;
static int mmapsize = 0;
static void *mmaptop = NULL;


void *heap_allocate(u4 size, bool references, methodinfo *finalizer)
{
	void *m;

	mmapptr = (void *) MEMORY_ALIGN((ptrint) mmapptr, ALIGNSIZE);
	
	m = mmapptr;
	mmapptr = (void *) ((ptrint) mmapptr + size);

	if (mmapptr > mmaptop)
		exceptions_throw_outofmemory_exit();

	MSET(m, 0, u1, size);

	return m;
}


void *heap_alloc_uncollectable(u4 size)
{
	return heap_allocate(size, false, NULL);
}


void *nogc_realloc(void *src, s4 len1, s4 len2)
{
	void *p;

	p = heap_allocate(len2, false, NULL);

	MCOPY(p, src, u1, len1);

	return p;
}


void heap_free(void *p)
{
	/* nop */
}



void nogc_init(u4 heapmaxsize, u4 heapstartsize)
{
	heapmaxsize = MEMORY_ALIGN(heapmaxsize, ALIGNSIZE);

	mmapptr = mmap((void *) MMAP_HEAPADDRESS,
				   (size_t) heapmaxsize,
				   PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS,
				   -1,
				   (off_t) 0);

	if (mmapptr == MAP_FAILED)
		exceptions_throw_outofmemory_exit();

	mmapsize = heapmaxsize;
	mmaptop = (void *) ((ptrint) mmapptr + mmapsize);
}


void gc_init(u4 heapmaxsize, u4 heapstartsize)
{
	/* nop */
}


void gc_call(void)
{
	log_text("GC call: nothing done...");
	/* nop */
}


s8 gc_get_heap_size(void)
{
	return 0;
}


s8 gc_get_free_bytes(void)
{
	return 0;
}


s8 gc_get_max_heap_size(void)
{
	return 0;
}


void gc_invoke_finalizers(void)
{
	/* nop */
}


void gc_finalize_all(void)
{
	/* nop */
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
