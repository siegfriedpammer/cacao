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

#include <assert.h>
#include <stdint.h>

#if defined(ENABLE_GC_CACAO)
# include "threads/thread.hpp"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "vm/global.h"
#include "vm/method.h"

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class GC {
public:
	// Critical section functions.
	static void critical_enter(void);
	static void critical_leave(void);
};

extern "C" {
#endif


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

/**
 * Enters a LLNI critical section which prevents the GC from moving
 * objects around on the collected heap.
 *
 * There are no race conditions possible while entering such a critical
 * section, because each thread only modifies its own thread local flag
 * and the GC reads the flags while the world is stopped.
 */
inline void GC::critical_enter()
{
#if defined(ENABLE_GC_CACAO)
	threadobject *t;

	t = THREADOBJECT;
	assert(!t->gc_critical);
	t->gc_critical = true;
#endif
}

/**
 * Leaves a LLNI critical section and allows the GC to move objects
 * around on the collected heap again.
 */
inline void GC::critical_leave()
{
#if defined(ENABLE_GC_CACAO)
	threadobject *t;

	t = THREADOBJECT;
	assert(t->gc_critical);
	t->gc_critical = false;
#endif
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
