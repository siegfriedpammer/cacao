/* toolbox/memory.c - 

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   Authors: Reinhard Grafl

   $Id: memory.c 1672 2004-12-03 16:41:52Z twisti $

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__DARWIN__)
/* If we compile with -ansi on darwin, <sys/types.h> is not included. So      */
/* let's do it here.                                                          */
# include <sys/types.h>
#endif

#include <sys/mman.h>
#include <unistd.h>

#include "config.h"
#include "mm/memory.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "native/native.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
# endif
#endif

#include "toolbox/logging.h"


/********* general types, variables and auxiliary functions *********/

static int mmapcodesize = 0;
static void *mmapcodeptr = NULL;


/*******************************************************************************

    This structure is used for dump memory allocation if cacao runs without
    threads.

*******************************************************************************/

#if !defined(USE_THREADS) || (defined(USE_THREADS) && !defined(NATIVE_THREADS))
static dumpinfo nothreads_dumpinfo;
#endif


void *mem_mmap(s4 size)
{
	void *m;

	size = ALIGN(size, ALIGNSIZE);

	if (size > mmapcodesize) {
		mmapcodesize = 0x10000;

		if (size > mmapcodesize)
			mmapcodesize = size;

		mmapcodesize = ALIGN(mmapcodesize, getpagesize());
		mmapcodeptr = mmap(NULL,
						   (size_t) mmapcodesize,
						   PROT_READ | PROT_WRITE | PROT_EXEC,
						   MAP_PRIVATE |
#if defined(HAVE_MAP_ANONYMOUS)
						   MAP_ANONYMOUS,
#elif defined(HAVE_MAP_ANON)
						   MAP_ANON,
#else
						   0,
#endif
						   -1,
						   (off_t) 0);

		if (mmapcodeptr == MAP_FAILED)
			throw_cacao_exception_exit(string_java_lang_InternalError,
									   "Out of memory");
	}

	m = mmapcodeptr;
	mmapcodeptr = (void *) ((char *) mmapcodeptr + size);
	mmapcodesize -= size;

	return m;
}


static void *checked_alloc(s4 size)
{
	/* always allocate memory zeroed out */
	void *m = calloc(size, 1);

	if (!m)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Out of memory");

	return m;
}


void *mem_alloc(s4 size)
{
	if (size == 0)
		return NULL;

	if (opt_stat) {
		memoryusage += size;

		if (memoryusage > maxmemusage)
			maxmemusage = memoryusage;
	}
	
	return checked_alloc(size);
}


void *mem_realloc(void *src, s4 len1, s4 len2)
{
	void *dst;

	if (!src) {
		if (len1 != 0)
			panic("reallocating memoryblock with address NULL, length != 0");
	}

	if (opt_stat)
		memoryusage = (memoryusage - len1) + len2;

	dst = realloc(src, len2);

	if (!dst)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Out of memory");

	return dst;
}


void mem_free(void *m, s4 size)
{
	if (!m) {
		if (size == 0)
			return;
		panic("returned memoryblock with address NULL, length != 0");
	}

	if (opt_stat)
		memoryusage -= size;

	free(m);
}


void *dump_alloc(s4 size)
{
	void *m;
	dumpinfo *di;

	/* If no threads are used, the dumpinfo structure is a static structure   */
	/* defined at the top of this file.                                       */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	di = &((threadobject *) THREADOBJECT)->dumpinfo;
#else
	di = &nothreads_dumpinfo;
#endif

	if (size == 0)
		return NULL;

	size = ALIGN(size, ALIGNSIZE);

	if (di->useddumpsize + size > di->allocateddumpsize) {
		dumpblock *newdumpblock;
		s4 newdumpblocksize;

		/* allocate a new dumplist structure */
		newdumpblock = checked_alloc(sizeof(dumpblock));

		/* If requested size is greater than the default, make the new dump   */
		/* block as big as the size requested. Else use the default size.     */
		if (size > DUMPBLOCKSIZE) {
			newdumpblocksize = size;

		} else {
			newdumpblocksize = DUMPBLOCKSIZE;
		}

		/* allocate dumpblock memory */
		/*printf("new dumpblock: %d\n", newdumpblocksize);*/
		newdumpblock->dumpmem = checked_alloc(newdumpblocksize);

		newdumpblock->prev = di->currentdumpblock;
		newdumpblock->size = newdumpblocksize;
		di->currentdumpblock = newdumpblock;

		/* Used dump size is previously allocated dump size, because the      */
		/* remaining free memory of the previous dump block cannot be used.   */
		/*printf("unused memory: %d\n", allocateddumpsize - useddumpsize);*/
		di->useddumpsize = di->allocateddumpsize;

		/* increase the allocated dump size by the size of the new dump block */
		di->allocateddumpsize += newdumpblocksize;

		/* the amount of globally allocated dump memory (thread save)         */
		if (opt_stat)
			globalallocateddumpsize += newdumpblocksize;
	}

	/* current dump block base address + the size of the current dump block - */
	/* the size of the unused memory = new start address                      */
	m = di->currentdumpblock->dumpmem + di->currentdumpblock->size -
		(di->allocateddumpsize - di->useddumpsize);

	/* increase used dump size by the allocated memory size                   */
	di->useddumpsize += size;

	if (opt_stat) {
		if (di->useddumpsize > maxdumpsize) {
			maxdumpsize = di->useddumpsize;
		}
	}
		
	return m;
}   


void *dump_realloc(void *src, s4 len1, s4 len2)
{
	void *dst = dump_alloc(len2);

	memcpy(dst, src, len1);

	return dst;
}


void dump_release(s4 size)
{
	dumpinfo *di;

	/* If no threads are used, the dumpinfo structure is a static structure   */
	/* defined at the top of this file.                                       */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	di = &((threadobject *) THREADOBJECT)->dumpinfo;
#else
	di = &nothreads_dumpinfo;
#endif

	if (size < 0 || size > di->useddumpsize)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Illegal dump release size %d", size);

	/* reset the used dump size to the size specified                         */
	di->useddumpsize = size;

	while (di->currentdumpblock && di->allocateddumpsize - di->currentdumpblock->size >= di->useddumpsize) {
		dumpblock *tmp = di->currentdumpblock;

#if 0
		/* XXX TWISTI: can someone explain this to me? */
#ifdef TRACECALLARGS
		/* Keep the first dumpblock if we don't free memory. Otherwise
		 * a new dumpblock is allocated each time and we run out of
		 * memory.
		 */
		if (!oldtop->prev) break;
#endif
#endif

		di->allocateddumpsize -= tmp->size;
		di->currentdumpblock = tmp->prev;

		/* the amount of globally allocated dump memory (thread save)         */
		if (opt_stat)
			globalallocateddumpsize -= tmp->size;

		/* release the dump memory and the dumpinfo structure                 */
		free(tmp->dumpmem);
		free(tmp);
	}
}


s4 dump_size()
{
	dumpinfo *di;

	/* If no threads are used, the dumpinfo structure is a static structure   */
	/* defined at the top of this file.                                       */
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	di = &((threadobject *) THREADOBJECT)->dumpinfo;
#else
	di = &nothreads_dumpinfo;
#endif

	if (!di)
		return 0;

	return di->useddumpsize;
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
