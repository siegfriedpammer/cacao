/* src/mm/memory.c - 

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

   Authors: Reinhard Grafl

   Changes: Christian Thalinger
   			Edwin Steiner

   $Id: memory.c 5158 2006-07-18 14:05:43Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#if defined(__DARWIN__)
/* If we compile with -ansi on darwin, <sys/types.h> is not included. So      */
/* let's do it here.                                                          */
# include <sys/types.h>
#endif

#include "vm/types.h"

#include "arch.h"

#include "mm/memory.h"
#include "native/native.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
# include "threads/native/threads.h"
#else
# include "threads/none/lock.h"
#endif

#include "toolbox/logging.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"


/*******************************************************************************

  This structure is used for dump memory allocation if cacao
  runs without threads.

*******************************************************************************/

#if !defined(ENABLE_THREADS)
static dumpinfo _no_threads_dumpinfo;
#endif

#if defined(ENABLE_THREADS)
#define DUMPINFO    &((threadobject *) THREADOBJECT)->dumpinfo
#else
#define DUMPINFO    &_no_threads_dumpinfo
#endif


/* global code memory variables ***********************************************/

#define DEFAULT_CODEMEM_SIZE    128 * 1024  /* defaulting to 128kB            */

#if defined(ENABLE_THREADS)
static java_objectheader *codememlock = NULL;
#endif
static int                codememsize = 0;
static void              *codememptr  = NULL;


/* memory_init *****************************************************************

   Initialize the memory subsystem.

*******************************************************************************/

bool memory_init(void)
{
#if defined(ENABLE_THREADS)
	codememlock = NEW(java_objectheader);

	lock_init_object_lock(codememlock);
#endif

	/* everything's ok */

	return true;
}


/* memory_checked_alloc ********************************************************

   Allocated zeroed-out memory and does an OOM check.

   ERROR HANDLING:
      XXX If no memory could be allocated, this function justs *exists*.

*******************************************************************************/

static void *memory_checked_alloc(s4 size)
{
	/* always allocate memory zeroed out */

	void *p = calloc(size, 1);

	if (p == NULL)
		exceptions_throw_outofmemory_exit();

	return p;
}


/* memory_cnew *****************************************************************

   Allocates memory from the heap, aligns it to architecutres PAGESIZE
   and make the memory read-, write-, and executeable.

*******************************************************************************/

void *memory_cnew(s4 size)
{
	void *p;
	int   pagesize;

	LOCK_MONITOR_ENTER(codememlock);

	size = ALIGN(size, ALIGNSIZE);

	/* check if enough memory is available */

	if (size > codememsize) {
		/* set default code size */

		codememsize = DEFAULT_CODEMEM_SIZE;

		/* do we need more? */

		if (size > codememsize)
			codememsize = size;

		/* get the pagesize of this architecture */

		pagesize = getpagesize();

		/* allocate normal heap memory */

		if ((p = memory_checked_alloc(codememsize + pagesize - 1)) == NULL)
			return NULL;

#if defined(ENABLE_STATISTICS)
		if (opt_stat) {
			codememusage += codememsize + pagesize - 1;

			if (codememusage > maxcodememusage)
				maxcodememusage = codememusage;
		}
#endif

		/* align the memory allocated to a multiple of PAGESIZE,
		   mprotect requires this */

		p = (void *) (((ptrint) p + pagesize - 1) & ~(pagesize - 1));

		/* make the memory read-, write-, and executeable */

		if (mprotect(p, codememsize, PROT_READ | PROT_WRITE | PROT_EXEC) == -1)
			vm_abort("mprotect failed: %s", strerror(errno));

		/* set global code memory pointer */

		codememptr = p;
	}

	/* get a memory chunk of the allocated memory */

	p = codememptr;
	codememptr = (void *) ((ptrint) codememptr + size);
	codememsize -= size;

	LOCK_MONITOR_EXIT(codememlock);

	return p;
}


/* memory_cfree ****************************************************************

   Frees the code memory pointed to.

   ATTENTION: This function currently does NOTHING!  Because we don't
   have a memory management for code memory.

*******************************************************************************/

void memory_cfree(void *p, s4 size)
{
	/* do nothing */
}


void *mem_alloc(s4 size)
{
	if (size == 0)
		return NULL;

#if defined(ENABLE_STATISTICS)
	if (opt_stat) {
		memoryusage += size;

		if (memoryusage > maxmemusage)
			maxmemusage = memoryusage;
	}
#endif

	return memory_checked_alloc(size);
}


void *mem_realloc(void *src, s4 len1, s4 len2)
{
	void *dst;

	if (!src) {
		if (len1 != 0) {
			log_text("reallocating memoryblock with address NULL, length != 0");
			assert(0);
		}
	}

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		memoryusage = (memoryusage - len1) + len2;
#endif

	dst = realloc(src, len2);

	if (dst == NULL)
		exceptions_throw_outofmemory_exit();

	return dst;
}


void mem_free(void *m, s4 size)
{
	if (!m) {
		if (size == 0)
			return;

		log_text("returned memoryblock with address NULL, length != 0");
		assert(0);
	}

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		memoryusage -= size;
#endif

	free(m);
}


/* dump_alloc ******************************************************************

   Allocate memory in the dump area.

   IN:
      size.........size of block to allocate, in bytes
	  			   may be zero, in which case NULL is returned

   RETURN VALUE:
      pointer to allocated memory, or
	  NULL iff `size` was zero

   ERROR HANDLING:
      XXX This function uses `memory_checked_alloc`, which *exits* if no 
	  memory could be allocated.

   THREADS:
      dump_alloc is thread safe. Each thread has its own dump memory area.

   dump_alloc is a fast allocator suitable for scratch memory that can be
   collectively freed when the current activity (eg. compiling) is done.

   You cannot selectively free dump memory. Before you start allocating it, 
   you remember the current size returned by `dump_size`. Later, when you no 
   longer need the memory, call `dump_release` with the remembered size and
   all dump memory allocated since the call to `dump_size` will be freed.

*******************************************************************************/

void *dump_alloc(s4 size)
{
#if defined(DISABLE_DUMP)

	/* use malloc memory for dump memory (for debugging only!) */

	return mem_alloc(size);

#else /* !defined(DISABLE_DUMP) */

	void     *m;
	dumpinfo *di;

	/* If no threads are used, the dumpinfo structure is a static structure   */
	/* defined at the top of this file.                                       */

	di = DUMPINFO;

	if (size == 0)
		return NULL;

	size = ALIGN(size, ALIGNSIZE);

	if (di->useddumpsize + size > di->allocateddumpsize) {
		dumpblock *newdumpblock;
		s4         newdumpblocksize;

		/* allocate a new dumplist structure */

		newdumpblock = memory_checked_alloc(sizeof(dumpblock));

		/* If requested size is greater than the default, make the new dump   */
		/* block as big as the size requested. Else use the default size.     */

		if (size > DUMPBLOCKSIZE) {
			newdumpblocksize = size;

		} else {
			newdumpblocksize = DUMPBLOCKSIZE;
		}

		/* allocate dumpblock memory */

		newdumpblock->dumpmem = memory_checked_alloc(newdumpblocksize);

		newdumpblock->prev = di->currentdumpblock;
		newdumpblock->size = newdumpblocksize;
		di->currentdumpblock = newdumpblock;

		/* Used dump size is previously allocated dump size, because the      */
		/* remaining free memory of the previous dump block cannot be used.   */

		di->useddumpsize = di->allocateddumpsize;

		/* increase the allocated dump size by the size of the new dump block */

		di->allocateddumpsize += newdumpblocksize;

#if defined(ENABLE_STATISTICS)
		/* the amount of globally allocated dump memory (thread save) */

		if (opt_stat)
			globalallocateddumpsize += newdumpblocksize;
#endif
	}

	/* current dump block base address + the size of the current dump block - */
	/* the size of the unused memory = new start address                      */

	m = di->currentdumpblock->dumpmem + di->currentdumpblock->size -
		(di->allocateddumpsize - di->useddumpsize);

	/* increase used dump size by the allocated memory size */

	di->useddumpsize += size;

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		if (di->useddumpsize > maxdumpsize)
			maxdumpsize = di->useddumpsize;
#endif

	return m;

#endif /* defined(DISABLE_DUMP) */
}


/* dump_realloc ****************************************************************

   Stupid realloc implementation for dump memory. Avoid, if possible.

*******************************************************************************/

void *dump_realloc(void *src, s4 len1, s4 len2)
{
#if defined(DISABLE_DUMP)
	/* use malloc memory for dump memory (for debugging only!) */

	return mem_realloc(src, len1, len2);
#else
	void *dst = dump_alloc(len2);

	memcpy(dst, src, len1);

	return dst;
#endif
}


/* dump_release ****************************************************************

   Release dump memory above the given size.

   IN:
       size........All dump memory above this mark will be freed. Usually
	               `size` will be the return value of a `dump_size` call
				   made earlier.

	ERROR HANDLING:
	   XXX If the given size is invalid, this function *exits* with an
	       error message.
				   
	See `dump_alloc`.

*******************************************************************************/

void dump_release(s4 size)
{
#if defined(DISABLE_DUMP)

	/* use malloc memory for dump memory (for debugging only!) */

	/* do nothing */

#else /* !defined(DISABLE_DUMP) */

	dumpinfo *di;

	/* If no threads are used, the dumpinfo structure is a static structure   */
	/* defined at the top of this file.                                       */

	di = DUMPINFO;

	if ((size < 0) || (size > di->useddumpsize))
		vm_abort("Illegal dump release size: %d", size);

	/* reset the used dump size to the size specified */

	di->useddumpsize = size;

	while (di->currentdumpblock && di->allocateddumpsize - di->currentdumpblock->size >= di->useddumpsize) {
		dumpblock *tmp = di->currentdumpblock;

		di->allocateddumpsize -= tmp->size;
		di->currentdumpblock = tmp->prev;

#if defined(ENABLE_STATISTICS)
		/* the amount of globally allocated dump memory (thread save) */

		if (opt_stat)
			globalallocateddumpsize -= tmp->size;
#endif

		/* release the dump memory and the dumpinfo structure */

		free(tmp->dumpmem);
		free(tmp);
	}

#endif /* defined(DISABLE_DUMP) */
}


/* dump_size *******************************************************************

   Return the current size of the dump memory area. See `dump_alloc`.

*******************************************************************************/

s4 dump_size(void)
{
#if defined(DISABLE_DUMP)
	/* use malloc memory for dump memory (for debugging only!) */

	return 0;

#else /* !defined(DISABLE_DUMP) */

	dumpinfo *di;

	/* If no threads are used, the dumpinfo structure is a static structure   */
	/* defined at the top of this file.                                       */

	di = DUMPINFO;

	if (!di)
		return 0;

	return di->useddumpsize;

#endif /* defined(DISABLE_DUMP) */
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
