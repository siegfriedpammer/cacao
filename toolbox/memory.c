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

   $Id: memory.c 662 2003-11-21 18:06:25Z jowenn $

*/


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "global.h"
#include "loging.h"
#include "memory.h"


/********* general types, variables and auxiliary functions *********/

#define DUMPBLOCKSIZE  (2<<21)
#define ALIGNSIZE           8

typedef struct dumplist {
	struct dumplist *prev;
	char *dumpmem;
} dumplist;



long int memoryusage = 0;

long int dumpsize = 0;
long int dumpspace = 0;
dumplist *topdumpblock = NULL;

long int maxmemusage = 0;
long int maxdumpsize = 0;

#define TRACECALLARGS

#ifdef TRACECALLARGS
static char *nomallocmem = NULL;
static char *nomalloctop;
static char *nomallocptr;


static void *lit_checked_alloc (int length)
{
	void *m;

	if (!nomallocmem) {
		nomallocmem = malloc(16777216); 
		nomalloctop = nomallocmem + 16777216;
		nomallocptr = nomallocmem;
	}

	nomallocptr = (void*) ALIGN((long) nomallocptr, ALIGNSIZE);
	
	m = nomallocptr;
	nomallocptr += length;
	if (nomallocptr > nomalloctop)
		panic("Out of memory");

	return m;
}

#else

static void *lit_checked_alloc(int length)
{
	void *m = malloc(length);
	if (!m) panic ("Out of memory");
	return m;
}

#endif


static void *checked_alloc(int length)
{
	void *m = malloc(length);
	if (!m) panic("Out of memory");
	return m;
}


static int mmapcodesize = 0;
static void *mmapcodeptr = NULL;


void *mem_mmap(int length)
{
	void *retptr;

	length = (ALIGN(length,ALIGNSIZE));
	if (length > mmapcodesize) {
		mmapcodesize = 0x10000;
		if (length > mmapcodesize)
			mmapcodesize = length;
		mmapcodesize = (ALIGN(mmapcodesize, getpagesize()));
		mmapcodeptr = mmap (NULL, (size_t) mmapcodesize,
							PROT_READ | PROT_WRITE | PROT_EXEC,
							MAP_PRIVATE | MAP_ANONYMOUS, -1, (off_t) 0);
		if (mmapcodeptr == (void*) -1)
			panic ("Out of memory");
	}
	retptr = mmapcodeptr;
	mmapcodeptr = (void*) ((char*) mmapcodeptr + length);
	mmapcodesize -= length;
	return retptr;
}


#ifdef DEBUG

/************ Memory manager, safe version **************/


typedef struct memblock {
	struct memblock *prev, *next;
	int length;
} memblock;

#define BLOCKOFFSET    (ALIGN(sizeof(memblock),ALIGNSIZE))

struct memblock *firstmemblock;



void *mem_alloc(int length)
{
	memblock *mb;

	if (length == 0) return NULL;
	mb = checked_alloc(length + BLOCKOFFSET);

	mb->prev = NULL;
	mb->next = firstmemblock;	
	mb->length = length;

	if (firstmemblock) firstmemblock->prev = mb;
	firstmemblock = mb;

	memoryusage += length;
	if (memoryusage > maxmemusage)
		maxmemusage = memoryusage;

	return ((char*) mb) + BLOCKOFFSET;
}


void *lit_mem_alloc(int length)
{
	memblock *mb;

	if (length == 0) return NULL;
	mb = lit_checked_alloc(length + BLOCKOFFSET);

	mb->prev = NULL;
	mb->next = firstmemblock;	
	mb->length = length;

	if (firstmemblock) firstmemblock -> prev = mb;
	firstmemblock = mb;

	memoryusage += length;
	if (memoryusage > maxmemusage) maxmemusage = memoryusage;

	return ((char*) mb) + BLOCKOFFSET;
}


void mem_free(void *m, int length)
{
	memblock *mb;
	if (!m) {
		if (length == 0) return;
		panic("returned memoryblock with address NULL, length != 0");
	}

	mb = (memblock*) (((char*) m) - BLOCKOFFSET);
	
	if (mb->length != length) {
		sprintf(logtext, 
				"Memory block of size %d has been return as size %d",
				mb->length, length);
		error();
	}
		
	if (mb->prev) mb->prev->next = mb->next;
	else firstmemblock = mb->next;
	if (mb->next) mb->next->prev = mb->prev;

	free (mb);

	memoryusage -= length;
}


void lit_mem_free(void *m, int length)
{
	memblock *mb;
	if (!m) {
		if (length==0) return;
		panic("returned memoryblock with address NULL, length != 0");
	}

	mb = (memblock*) (((char*) m) - BLOCKOFFSET);
	
	if (mb->length != length) {
		sprintf(logtext, 
				"Memory block of size %d has been return as size %d",
				mb->length, length);
		error();
	}
		
	if (mb->prev) mb->prev->next = mb->next;
	else firstmemblock = mb->next;
	if (mb->next) mb->next->prev = mb->prev;

#ifdef TRACECALLARGS
#else
	free(mb);
#endif

	memoryusage -= length;
}


void *mem_realloc(void *m1, int len1, int len2)
{
	void *m2;
	
	m2 = mem_alloc(len2);
	memcpy(m2, m1, len1);
	mem_free(m1, len1);

	return m2;
}




static void mem_characterlog(unsigned char *m, int len)
{
#	define LINESIZE 16
	int z, i;
	
	for (z = 0; z < len; z += LINESIZE) {
		sprintf(logtext, "   ");
			
		for (i = z; i < (z + LINESIZE) && i < len; i++) {
			sprintf(logtext + strlen(logtext), "%2x ", m[i]);
		}
		for (; i < (z + LINESIZE); i++) {
			sprintf(logtext + strlen(logtext), "   ");
		}
					
		sprintf(logtext + strlen(logtext),"   ");
		for (i = z; i < (z + LINESIZE) && i < len; i++) {
			sprintf(logtext+strlen(logtext),
					"%c", (m[i] >= ' ' && m[i] <= 127) ? m[i] : '.');
		}
			
		dolog();
	}
}

#else
/******* Memory manager, fast version ******/


void *mem_alloc(int length)
{
	if (length == 0) return NULL;

	memoryusage += length;
	if (memoryusage > maxmemusage) maxmemusage = memoryusage;
	
	return checked_alloc(length);
}


void *lit_mem_alloc(int length)
{
	if (length == 0) return NULL;

	memoryusage += length;
	if (memoryusage > maxmemusage) maxmemusage = memoryusage;
	
	return lit_checked_alloc(length);
}


void mem_free(void *m, int length)
{
	if (!m) {
		if (length == 0) return;
		panic("returned memoryblock with address NULL, length != 0");
	}

	memoryusage -= length;

	free(m);
}


void lit_mem_free(void *m, int length)
{
	if (!m) {
		if (length == 0) return;
		panic("returned memoryblock with address NULL, length != 0");
	}

	memoryusage -= length;

#ifdef TRACECALLARGS
#else
	free(m);
#endif
}


void *mem_realloc (void *m1, int len1, int len2)
{
	void *m2;

	if (!m1) {
		if (len1!=0) 
			panic ("reallocating memoryblock with address NULL, length != 0");
	}
		
	memoryusage = (memoryusage - len1) + len2;

	m2 = realloc (m1, len2);
	if (!m2) panic ("Out of memory");
	return m2;
}


#endif

/******* common memory manager parts ******/

long int mem_usage()
{
	return memoryusage;
}


void *dump_alloc(int length)
{
	void *m;

	if (length == 0) return NULL;
	
	length = ALIGN(length, ALIGNSIZE);

	assert(length <= DUMPBLOCKSIZE);
	assert(length > 0);

	if (dumpsize + length > dumpspace) {
		dumplist *newdumpblock = checked_alloc(sizeof(dumplist));

		newdumpblock->prev = topdumpblock;
		topdumpblock = newdumpblock;

		newdumpblock->dumpmem = checked_alloc(DUMPBLOCKSIZE);

		dumpsize = dumpspace;
		dumpspace += DUMPBLOCKSIZE;		
	}
	
	m = topdumpblock->dumpmem + DUMPBLOCKSIZE - (dumpspace - dumpsize);
	dumpsize += length;
	
	if (dumpsize > maxdumpsize) {
		maxdumpsize = dumpsize;
	}
		
	return m;
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
	assert(size >= 0 && size <= dumpsize);

	dumpsize = size;
	
	while (dumpspace > dumpsize + DUMPBLOCKSIZE) {
		dumplist *oldtop = topdumpblock;
		
		topdumpblock = oldtop->prev;
		dumpspace -= DUMPBLOCKSIZE;
		
#ifdef TRACECALLARGS
#else
		free(oldtop->dumpmem);
		free(oldtop);
#endif
	}		
}


void mem_usagelog (int givewarnings)
{
	if ((memoryusage!=0) && givewarnings) {
		sprintf (logtext, "Allocated memory not returned: %d",
				 (int)memoryusage);
		dolog();

#ifdef DEBUG
		{ 
			memblock *mb = firstmemblock;
			while (mb) {
				sprintf (logtext, "   Memory block size: %d", 
						 (int)(mb->length) );
				dolog();
				mem_characterlog ( ((unsigned char*)mb) + BLOCKOFFSET, mb->length);
				mb = mb->next;
			}
		}
#endif
			
	}

	if ((dumpsize!=0) && givewarnings) {
		sprintf (logtext, "Dump memory not returned: %d",(int)dumpsize);
		dolog();
	}


	sprintf(logtext, "Random/Dump - memory usage: %dK/%dK", 
			(int)((maxmemusage+1023)/1024), 
			(int)((maxdumpsize+1023)/1024) );
	dolog();
	
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
