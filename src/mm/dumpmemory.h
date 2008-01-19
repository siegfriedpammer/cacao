/* src/mm/dumpmemory.h - dump memory management

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zu Foerderung der freien virtuellen Machine CACAO

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

*/


#ifndef _DUMPMEMORY_H
#define _DUMPMEMORY_H

/* forward typedefs ***********************************************************/

typedef struct dumpblock_t dumpblock_t;
typedef struct dumpinfo_t  dumpinfo_t;

#include "config.h"

#include <stdint.h>
#include <string.h>


/* ATTENTION: We need to define dumpblock_t and dumpinfo_t before
   internal includes, as we need dumpinfo_t as nested structure in
   threadobject. */

/* dumpblock ******************************************************************/

#define DUMPBLOCKSIZE    2 << 13    /* 2 * 8192 bytes */
#define ALIGNSIZE        8

struct dumpblock_t {
	dumpblock_t *prev;
	void        *dumpmem;
	int32_t      size;
};


/* dump_allocation *************************************************************

   This struct is used to record dump memory allocations for ENABLE_MEMCHECK.

*******************************************************************************/

#if defined(ENABLE_MEMCHECK)
typedef struct dump_allocation_t dump_allocation_t;

struct dump_allocation_t {
	dump_allocation_t *next;
	void              *mem;
	int32_t            used;
	int32_t            size;
};
#endif


/* dumpinfo *******************************************************************/

struct dumpinfo_t {
	dumpblock_t       *block;                   /* the current top-most block */
	int32_t            allocated;             /* allocated bytes in this area */
	int32_t            used;                  /* used bytes in this dump area */
#if defined(ENABLE_MEMCHECK)
	dump_allocation_t *allocations;       /* list of allocations in this area */
#endif
};


/* convenience macros *********************************************************/

#define DMARKER               dumpmarker = dumpmemory_marker()
#define DRELEASE              dumpmemory_release(dumpmarker)

#define DNEW(type)            ((type *) dumpmemory_get(sizeof(type)))
#define DMNEW(type,num)       ((type *) dumpmemory_get(sizeof(type) * (num)))
#define DMREALLOC(ptr,type,num1,num2) dumpmemory_realloc((ptr), sizeof(type) * (num1), \
                                                          sizeof(type) * (num2))

/* function prototypes ********************************************************/

void    *dumpmemory_get(size_t size);
void    *dumpmemory_realloc(void *src, int32_t len1, int32_t len2);
int32_t  dumpmemory_marker(void);
void     dumpmemory_release(int32_t size);

#endif /* _DUMPMEMORY_H */


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
