/* toolbox/memory.h - macros for memory management

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

   $Id: memory.h 557 2003-11-02 22:51:59Z twisti $

*/


#ifndef _MEMORY_H
#define _MEMORY_H

#include "types.h"

/* Uncollectable memory which can contain references */
void *heap_alloc_uncollectable(u4 bytelen);
#define GCNEW(type,num)       heap_alloc_uncollectable(sizeof(type) * (num))

#define ALIGN(pos,size)       ((((pos) + (size) - 1) / (size)) * (size))
#define PADDING(pos,size)     (ALIGN((pos),(size)) - (pos))
#define OFFSET(s,el)          ((int) ((size_t) & (((s*) 0)->el)))


#define NEW(type)             ((type*) mem_alloc(sizeof(type)))
#define FREE(ptr,type)        mem_free(ptr, sizeof(type))

#define LNEW(type)            ((type*) lit_mem_alloc(sizeof(type)))
#define LFREE(ptr,type)       lit_mem_free(ptr, sizeof(type))

#define MNEW(type,num)        ((type*) mem_alloc(sizeof(type) * (num)))
#define MFREE(ptr,type,num)   mem_free(ptr, sizeof(type) * (num))
#define MREALLOC(ptr,type,num1,num2) mem_realloc(ptr, sizeof(type) * (num1), \
                                                      sizeof(type) * (num2))

#define DNEW(type)            ((type*) dump_alloc(sizeof(type)))
#define DMNEW(type,num)       ((type*) dump_alloc(sizeof(type) * (num)))
#define DMREALLOC(ptr,type,num1,num2)  dump_realloc(ptr, sizeof(type) * (num1),\
                                                         sizeof(type) * (num2))

#define MCOPY(dest,src,type,num)  memcpy(dest,src, sizeof(type)* (num))

#ifdef USE_CODEMMAP
#define CNEW(type,num)        ((type*) mem_mmap( sizeof(type) * (num)))
#define CFREE(ptr,num)
#else
#define CNEW(type,num)        ((type*) mem_alloc(sizeof(type) * (num)))
#define CFREE(ptr,num)        mem_free(ptr, num)
#endif


void *mem_alloc(int length);
void *mem_mmap(int length);
void *lit_mem_alloc(int length);
void mem_free(void *m, int length);
void lit_mem_free(void *m, int length);
void *mem_realloc(void *m, int len1, int len2);
long int mem_usage();

void *dump_alloc(int length);
void *dump_realloc(void *m, int len1, int len2);
long int dump_size();
void dump_release(long int size);

void mem_usagelog(int givewarnings);
 
 
 
/* 
---------------------------- Interface description -----------------------

There are two possible choices for allocating memory:

	1.   explicit allocating / deallocating

			mem_alloc ..... allocate a memory block 
			mem_free ...... free a memory block
			mem_realloc ... change size of a memory block (position may change)
			mem_usage ..... amount of allocated memory


	2.   explicit allocating, automatic deallocating
	
			dump_alloc .... allocate a memory block in the dump area
			dump_realloc .. change size of a memory block (position may change)
			dump_size ..... marks the current top of dump
			dump_release .. free all memory requested after the mark
			                
	
There are some useful macros:

	NEW (type) ....... allocate memory for an element of type `type`
	FREE (ptr,type) .. free memory
	
	MNEW (type,num) .. allocate memory for an array
	MFREE (ptr,type,num) .. free memory
	
	MREALLOC (ptr,type,num1,num2) .. enlarge the array to size num2
	                                 
These macros do the same except they operate on the dump area:
	
	DNEW,  DMNEW, DMREALLOC   (there is no DFREE)


-------------------------------------------------------------------------------

Some more macros:

	ALIGN (pos, size) ... make pos divisible by size. always returns an
						  address >= pos.
	                      
	
	OFFSET (s,el) ....... returns the offset of 'el' in structure 's' in bytes.
	                      
	MCOPY (dest,src,type,num) ... copy 'num' elements of type 'type'.
	

*/

#endif /* _MEMORY_H */


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
