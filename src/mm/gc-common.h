/* src/mm/gc-common.h - gc independant interface for heap managment

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

   $Id: gc-common.h 8374 2007-08-21 10:20:33Z michi $

*/


#ifndef _GC_COMMON_H
#define _GC_COMMON_H

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"

#include "vmcore/method.h"


/* reference types ************************************************************/

#define GC_REFTYPE_THREADOBJECT  1
#define GC_REFTYPE_CLASSLOADER   2
#define GC_REFTYPE_JNI_GLOBALREF 3
#define GC_REFTYPE_FINALIZER     4
#define GC_REFTYPE_LOCALREF      5
#define GC_REFTYPE_STACK         6
#define GC_REFTYPE_CLASSREF      7


/* function prototypes ********************************************************/

void  gc_init(u4 heapmaxsize, u4 heapstartsize);

void *heap_alloc_uncollectable(u4 size);
void *heap_alloc(u4 size, u4 references, methodinfo *finalizer, bool collect);
void  heap_free(void *p);

#if defined(ENABLE_GC_CACAO)
void  heap_init_objectheader(java_object_t *o, u4 size);
s4    heap_get_hashcode(java_object_t *o);

void  gc_reference_register(java_object_t **ref, int32_t reftype);
void  gc_reference_unregister(java_object_t **ref);
#endif

void  gc_call(void);
s8    gc_get_heap_size(void);
s8    gc_get_free_bytes(void);
s8    gc_get_total_bytes(void);
s8    gc_get_max_heap_size(void);
void  gc_invoke_finalizers(void);
void  gc_finalize_all(void);
void *gc_out_of_memory(size_t bytes_requested);

#endif /* _GC_COMMON_H */


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
