/* src/native/localref.h - Management of local reference tables

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

*/


#ifndef _LOCALREF_H
#define _LOCALREF_H

/* forward typedefs ***********************************************************/

typedef struct localref_table localref_table;

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"


/* localref_table **************************************************************

   ATTENTION: keep this structure a multiple of 8-bytes!!! This is
   essential for the native stub on 64-bit architectures.

*******************************************************************************/

#define LOCALREFTABLE_CAPACITY    16

struct localref_table {
	s4                 capacity;        /* table size                         */
	s4                 used;            /* currently used references          */
	s4                 localframes;     /* number of current frames           */
	s4                 PADDING;         /* 8-byte padding                     */
	localref_table    *prev;            /* link to prev table (LocalFrame)    */
	java_object_t     *refs[LOCALREFTABLE_CAPACITY]; /* references            */
};


#if defined(ENABLE_THREADS)
#define LOCALREFTABLE    (THREADOBJECT->_localref_table)
#else
extern localref_table *_no_threads_localref_table;

#define LOCALREFTABLE    (_no_threads_localref_table)
#endif


/* function prototypes ********************************************************/

bool localref_table_init(void);
void localref_table_add(localref_table *lrt);
void localref_table_remove();
bool localref_frame_push(int32_t capacity);
void localref_frame_pop_all(void);

#if !defined(NDEBUG)
void localref_dump(void);
#endif


#endif /* _LOCALREF_H */


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
