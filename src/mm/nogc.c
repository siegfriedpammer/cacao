/* src/mm/nogc.c - allocates memory through malloc (no GC)

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Christian Thalinger

   Changes:

   $Id: nogc.c 3532 2005-11-02 13:33:26Z twisti $

*/


#include <stdlib.h>
#include <sys/mman.h>

#include "config.h"
#include "vm/types.h"

#include "boehm-gc/include/gc.h"

#include "mm/boehm.h"
#include "toolbox/logging.h"
#include "vm/options.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"


/* global stuff ***************************************************************/

#define MMAP_HEAPADDRESS    0x10000000  /* try to map the heap to this addr.  */
#define ALIGNSIZE           8

static void *mmapptr = NULL;
static int mmapsize = 0;
static void *mmaptop = NULL;


void *heap_allocate(u4 size, bool references, methodinfo *finalizer)
{
	void *m;

	mmapptr = (void *) ALIGN((ptrint) mmapptr, ALIGNSIZE);
	
	m = mmapptr;
	mmapptr = (void *) ((ptrint) mmapptr + size);

	if (mmapptr > mmaptop)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Out of memory");

	MSET(m, 0, u1, size);

	return m;
}


void *heap_alloc_uncollectable(u4 size)
{
	return heap_allocate(size, false, NULL);
}


void gc_init(u4 heapmaxsize, u4 heapstartsize)
{
	heapmaxsize = ALIGN(heapmaxsize, ALIGNSIZE);

	mmapptr = mmap((void *) MMAP_HEAPADDRESS,
				   (size_t) heapmaxsize,
				   PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS,
				   -1,
				   (off_t) 0);

	if (mmapptr == MAP_FAILED)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Out of memory");

	mmapsize = heapmaxsize;
	mmaptop = (void *) ((ptrint) mmapptr + mmapsize);
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
