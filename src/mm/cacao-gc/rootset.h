/* mm/cacao-gc/rootset.h - GC header for root set management

   Copyright (C) 2006 R. Grafl, A. Krall, C. Kruegel,
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

   $Id$

*/


#ifndef _ROOTSET_H
#define _ROOTSET_H

typedef struct rootset_t rootset_t;

#include "config.h"
#include "vm/types.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#else
# include "threads/none/threads.h"
#endif

#include "vm/jit/replace.h"
#include "vmcore/method.h"


/* Structures *****************************************************************/

#define ROOTSET_DUMMY_THREAD ((threadobject *) (ptrint) -1)
#define RS_REFS 512 /* TODO: you see why we need to rethink this!!! */

#define REFTYPE_THREADOBJECT 1
#define REFTYPE_CLASSLOADER  2
#define REFTYPE_GLOBALREF    3
#define REFTYPE_FINALIZER    4
#define REFTYPE_LOCALREF     5
#define REFTYPE_STACK        6
#define REFTYPE_CLASSREF     7

/* rootset is passed as array of pointers, which point to the location of
   the reference */

struct rootset_t {
	rootset_t          *next;           /* link to the next chain element */
	threadobject       *thread;         /* thread this rootset belongs to */
	sourcestate_t      *ss;             /* sourcestate of the thread */
	executionstate_t   *es;             /* executionstate of the thread */
	s4                  refcount;       /* number of references */
	java_objectheader **refs[RS_REFS];  /* list of references */
	bool                ref_marks[RS_REFS]; /* indicates if a reference marks */
#if !defined(NDEBUG)
	s4                  ref_type[RS_REFS];
#endif
};


/* Prototypes *****************************************************************/

/*
rootset_t *rootset_create(void);
void rootset_from_globals(rootset_t *rs);
void rootset_from_thread(threadobject *thread, rootset_t *rs);
*/
rootset_t *rootset_readout();
void rootset_writeback(rootset_t *rs);

#if !defined(NDEBUG)
void rootset_print(rootset_t *rs);
#endif


#endif /* _ROOTSET_H */

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
