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

   $Id: memory.c 1335 2004-07-21 15:57:10Z twisti $

*/


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "exceptions.h"
#include "global.h"
#include "native.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"


/********* general types, variables and auxiliary functions *********/

#define DUMPBLOCKSIZE  (2<<21)
#define ALIGNSIZE           8

typedef struct dumplist {
	struct dumplist *prev;
	char *dumpmem;
	int size;
} dumplist;


static int mmapcodesize = 0;
static void *mmapcodeptr = NULL;

long int memoryusage = 0;

long int dumpsize = 0;
long int dumpspace = 0;
dumplist *topdumpblock = NULL;

long int maxmemusage = 0;
long int maxdumpsize = 0;


static void *lit_checked_alloc(int length)
{
	void *m = malloc(length);

	if (!m)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Out of memory");

	/* not setting to zero causes cacao to segfault (String.hashCode() is
	   completely wrong) */
	memset(m, 0, length);

	return m;
}


static void *checked_alloc(int length)
{
	void *m = malloc(length);

	if (!m)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Out of memory");

	return m;
}


void *mem_mmap(int length)
{
	void *retptr;

	length = ALIGN(length, ALIGNSIZE);

	if (length > mmapcodesize) {
		mmapcodesize = 0x10000;

		if (length > mmapcodesize)
			mmapcodesize = length;

		mmapcodesize = ALIGN(mmapcodesize, getpagesize());
		mmapcodeptr = mmap (NULL,
							(size_t) mmapcodesize,
							PROT_READ | PROT_WRITE | PROT_EXEC,
							MAP_PRIVATE | MAP_ANONYMOUS,
							-1,
							(off_t) 0);

		if (mmapcodeptr == MAP_FAILED)
			throw_cacao_exception_exit(string_java_lang_InternalError,
									   "Out of memory");
	}

	retptr = mmapcodeptr;
	mmapcodeptr = (void *) ((char *) mmapcodeptr + length);
	mmapcodesize -= length;

	return retptr;
}


void *mem_alloc(int length)
{
	if (length == 0)
		return NULL;

	memoryusage += length;
	if (memoryusage > maxmemusage)
		maxmemusage = memoryusage;
	
	return checked_alloc(length);
}


void *lit_mem_alloc(int length)
{
	if (length == 0)
		return NULL;

	memoryusage += length;
	if (memoryusage > maxmemusage)
		maxmemusage = memoryusage;
	
	return lit_checked_alloc(length);
}


void mem_free(void *m, int length)
{
	if (!m) {
		if (length == 0)
			return;
		panic("returned memoryblock with address NULL, length != 0");
	}

	memoryusage -= length;

	free(m);
}


void lit_mem_free(void *m, int length)
{
	if (!m) {
		if (length == 0)
			return;
		panic("returned memoryblock with address NULL, length != 0");
	}

	memoryusage -= length;

	free(m);
}


void *mem_realloc(void *m1, int len1, int len2)
{
	void *m2;

	if (!m1) {
		if (len1 != 0)
			panic("reallocating memoryblock with address NULL, length != 0");
	}
		
	memoryusage = (memoryusage - len1) + len2;

	m2 = realloc(m1, len2);

	if (!m2)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Out of memory");

	return m2;
}


/******* common memory manager parts ******/

long int mem_usage()
{
	return memoryusage;
}


void *dump_alloc(int length)
{
#if 1
	return checked_alloc(length);
#else
	void *m;
	int blocksize = DUMPBLOCKSIZE;

	if (length == 0) return NULL;
	
	length = ALIGN(length, ALIGNSIZE);

	if (length > DUMPBLOCKSIZE)
		blocksize = length;
	assert(length > 0);

	if (dumpsize + length > dumpspace) {
		dumplist *newdumpblock = checked_alloc(sizeof(dumplist));

		newdumpblock->prev = topdumpblock;
		newdumpblock->size = blocksize;
		topdumpblock = newdumpblock;

		newdumpblock->dumpmem = checked_alloc(blocksize);

		dumpsize = dumpspace;
		dumpspace += blocksize;
	}
	
	m = topdumpblock->dumpmem + blocksize - (dumpspace - dumpsize);
	dumpsize += length;
	
	if (dumpsize > maxdumpsize) {
		maxdumpsize = dumpsize;
	}
		
	return m;
#endif
}   


void *dump_realloc(void *ptr, int len1, int len2)
{
	void *p2 = dump_alloc(len2);

	memcpy(p2, ptr, len1);

	return p2;
}


long int dump_size()
{
	return dumpsize;
}


void dump_release(long int size)
{
	return;
	assert(size >= 0 && size <= dumpsize);

	dumpsize = size;
	
	while (topdumpblock && (dumpspace - topdumpblock->size >= dumpsize)) {
		dumplist *oldtop = topdumpblock;

#ifdef TRACECALLARGS
		/* Keep the first dumpblock if we don't free memory. Otherwise
		 * a new dumpblock is allocated each time and we run out of
		 * memory.
		 */
		if (!oldtop->prev) break;
#endif
		
		dumpspace -= oldtop->size;
		topdumpblock = oldtop->prev;
		
		free(oldtop->dumpmem);
		free(oldtop);
	}		
}


void mem_usagelog (int givewarnings)
{
	if ((memoryusage != 0) && givewarnings) {
		dolog("Allocated memory not returned: %d", (s4) memoryusage);
	}

	if ((dumpsize != 0) && givewarnings) {
		dolog("Dump memory not returned: %d", (s4) dumpsize);
	}

	dolog("Random/Dump - memory usage: %dK/%dK", 
		  (s4) ((maxmemusage + 1023) / 1024),
		  (s4) ((maxdumpsize + 1023) / 1024));
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
