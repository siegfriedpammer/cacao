/* src/mm/gc.hpp - gc independant interface for heap managment

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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


#ifndef _GC_HPP
#define _GC_HPP

#include "config.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "vm/global.h"
#include "vm/method.h"


/* reference types ************************************************************/

enum {
	GC_REFTYPE_THREADOBJECT,
	GC_REFTYPE_CLASSLOADER,
	GC_REFTYPE_JNI_GLOBALREF,
	GC_REFTYPE_FINALIZER,
	GC_REFTYPE_LOCALREF,
	GC_REFTYPE_STACK,
	GC_REFTYPE_CLASSREF,
	GC_REFTYPE_LOCKRECORD
};


/* function prototypes ********************************************************/

void    gc_init(size_t heapmaxsize, size_t heapstartsize);

void*   heap_alloc_uncollectable(size_t size);
void*   heap_alloc(size_t size, int references, methodinfo *finalizer, bool collect);
void    heap_free(void *p);

#if defined(ENABLE_GC_CACAO)
void    heap_init_objectheader(java_object_t *o, uint32_t size);
int32_t heap_get_hashcode(java_object_t *o);

void    gc_reference_register(java_object_t **ref, int32_t reftype);
void    gc_reference_unregister(java_object_t **ref);

void    gc_weakreference_register(java_object_t **ref, int32_t reftype);
void    gc_weakreference_unregister(java_object_t **ref);
#endif

void    gc_call(void);
int64_t gc_get_heap_size(void);
int64_t gc_get_free_bytes(void);
int64_t gc_get_total_bytes(void);
int64_t gc_get_max_heap_size(void);
void    gc_invoke_finalizers(void);
void    gc_finalize_all(void);
void*   gc_out_of_memory(size_t bytes_requested);


/* inlined functions **********************************************************/

static inline int32_t heap_hashcode(java_object_t *obj)
{
#if defined(ENABLE_GC_CACAO)
	return heap_get_hashcode(obj);
#else
	return (int32_t)(intptr_t) obj;
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* _GC_HPP */


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
