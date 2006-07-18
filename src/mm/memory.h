/* src/mm/memory.h - macros for memory management

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

   $Id: memory.h 5158 2006-07-18 14:05:43Z twisti $

*/


#ifndef _MEMORY_H
#define _MEMORY_H

/* forward typedefs ***********************************************************/

typedef struct dumpblock dumpblock;
typedef struct dumpinfo dumpinfo;

#include "config.h"

#include <string.h>

#include "vm/types.h"

#include "mm/boehm.h"


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


#define DUMPBLOCKSIZE    2 << 13    /* 2 * 8192 bytes */
#define ALIGNSIZE        8


/* dumpblock ******************************************************************/

struct dumpblock {
	dumpblock *prev;
	u1        *dumpmem;
	s4         size;
};


/* dumpinfo *******************************************************************/

struct dumpinfo {
	dumpblock *currentdumpblock;
	s4         allocateddumpsize;
	s4         useddumpsize;
};


#define ALIGN(pos,size)       ((((pos) + (size) - 1) / (size)) * (size))
#define PADDING(pos,size)     (ALIGN((pos),(size)) - (pos))
#define OFFSET(s,el)          ((s4) ((ptrint) &(((s*) 0)->el)))

#if !defined(DISABLE_GC)

#define NEW(type)             ((type *) mem_alloc(sizeof(type)))
#define FREE(ptr,type)        mem_free((ptr), sizeof(type))

#define MNEW(type,num)        ((type *) mem_alloc(sizeof(type) * (num)))
#define MFREE(ptr,type,num)   mem_free((ptr), sizeof(type) * (num))

#define MREALLOC(ptr,type,num1,num2) mem_realloc((ptr), sizeof(type) * (num1), \
                                                        sizeof(type) * (num2))

#else

#define NEW(type)             GCNEW(type)
#define FREE(ptr,type)        GCFREE(ptr)

#define MNEW(type,num)        GCMNEW(type,num)
#define MFREE(ptr,type,num)   GCFREE(ptr)

#define MREALLOC(ptr,type,num1,num2) nogc_realloc((ptr), sizeof(type) * (num1), \
                                                        sizeof(type) * (num2))

#endif

#define DNEW(type)            ((type *) dump_alloc(sizeof(type)))
#define DMNEW(type,num)       ((type *) dump_alloc(sizeof(type) * (num)))
#define DMREALLOC(ptr,type,num1,num2) dump_realloc((ptr), sizeof(type) * (num1), \
                                                          sizeof(type) * (num2))

#define MCOPY(dest,src,type,num) memcpy((dest), (src), sizeof(type) * (num))
#define MSET(ptr,byte,type,num) memset((ptr), (byte), sizeof(type) * (num))
#define MZERO(ptr,type,num)     MSET(ptr,0,type,num)
#define MMOVE(dest,src,type,num) memmove((dest), (src), sizeof(type) * (num))

#define CNEW(type,num)        ((type *) memory_cnew(sizeof(type) * (num)))
#define CFREE(ptr,num)        memory_cfree((ptr),(num))


/* GC macros ******************************************************************/

/* Uncollectable memory which can contain references */

#define GCNEW_UNCOLLECTABLE(type,num) ((type *) heap_alloc_uncollectable(sizeof(type) * (num)))

#define GCNEW(type)           heap_allocate(sizeof(type), true, NULL)
#define GCMNEW(type,num)      heap_allocate(sizeof(type) * (num), true, NULL)

#define GCFREE(ptr)           heap_free((ptr))


/* function prototypes ********************************************************/

/* initializes the memory subsystem */
bool memory_init(void);

void *memory_cnew(s4 size);
void  memory_cfree(void *p, s4 size);

void *mem_alloc(s4 size);
void  mem_free(void *m, s4 size);
void *mem_realloc(void *src, s4 len1, s4 len2);

void *dump_alloc(s4 size);
void *dump_realloc(void *src, s4 len1, s4 len2);
s4    dump_size(void);
void  dump_release(s4 size);

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
