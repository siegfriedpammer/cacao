/* src/mm/boehm.h - interface for boehm gc header

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

   $Id: boehm.h 3216 2005-09-19 13:07:54Z twisti $

*/


#ifndef _BOEHM_H
#define _BOEHM_H

#include "vm/types.h"

#include "vm/method.h"


struct otherstackcall;

typedef void *(*calltwoargs)(void *, u4);

struct otherstackcall {
	calltwoargs  p2;
	void        *p;
	u4           l;
};


/* function prototypes ********************************************************/

void *heap_alloc_uncollectable(u4 bytelength);
void  runboehmfinalizer(void *o, void *p);
void *heap_allocate (u4 bytelength, bool references, methodinfo *finalizer);
void *heap_reallocate(void *p, u4 bytelength);
void  heap_free(void *p);

void  gc_init(u4 heapmaxsize, u4 heapstartsize);
void  gc_call(void);
s8    gc_get_heap_size(void);
s8    gc_get_free_bytes(void);
s8    gc_get_max_heap_size(void);
void  gc_invoke_finalizers(void);
void  gc_finalize_all(void);
void *gc_out_of_memory(size_t bytes_requested);

#endif /* _BOEHM_H */


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
