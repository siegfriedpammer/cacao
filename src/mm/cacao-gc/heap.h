/* mm/cacao-gc/heap.h - GC header for heap management

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

   Contact: cacao@cacaojvm.org

   Authors: Michael Starzinger

   $Id$

*/


#ifndef _HEAP_H
#define _HEAP_H

#include "vm/types.h"

#include "gc.h"
#include "region.h"



/* TODO: maybe move this to global.h */
#define GC_FLAG_FINALIZER     0x40
#define GC_FLAG_UNCOLLECTABLE HDRFLAG_UNCOLLECTABLE
#define GC_FLAG_MARKED        (HDRFLAG_MARK1 | HDRFLAG_MARK2)

#define GC_IS_MARKED(obj)    GC_TEST_FLAGS(obj, GC_FLAG_MARKED)
#define GC_SET_MARKED(obj)   GC_SET_FLAGS(obj, GC_FLAG_MARKED)
#define GC_CLEAR_MARKED(obj) GC_CLEAR_FLAGS(obj, GC_FLAG_MARKED)


#define GC_SIZE_DUMMY 0xff


#if SIZEOF_VOID_P == 8
# define GC_GET_SIZE(obj)       ((u4) (((obj)->hdrflags >> 56) & 0xff))
# define GC_SET_SIZE(obj, size) ((obj)->hdrflags |= ((u8) ((size) & 0xff)) << 56)
#else
# define GC_GET_SIZE(obj)       ((u4) (((obj)->hdrflags >> 24) & 0xff))
# define GC_SET_SIZE(obj, size) ((obj)->hdrflags |= ((u4) ((size) & 0xff)) << 24)
#endif


/*extern void *heap_base;
extern void *heap_ptr;*/
extern s4 heap_current_size;
extern s4 heap_maximal_size;
extern s4 heap_free_size;
extern s4 heap_used_size;
extern regioninfo_t *heap_region_sys;
extern regioninfo_t *heap_region_main;


s4 get_object_size(java_objectheader *o);


#if !defined(NDEBUG)
void heap_print_object(java_objectheader *o);
void heap_dump_region(regioninfo_t *region, bool marked_only);
#endif


/* walking macros */
#define IS_ARRAY(o) ((o)->vftbl->arraydesc != NULL)
#define FOREACH_ARRAY_REF(o,ref,refptr,code) \
	{ \
		java_objectarray *a = (java_objectarray *) o; \
		arraydescriptor  *desc = o->vftbl->arraydesc; \
		int i; \
		\
		GC_ASSERT(desc); \
		\
		if (desc->arraytype == ARRAYTYPE_OBJECT) { \
			for (i = 0; i < a->header.size; i++) { \
				\
				refptr = &( a->data[i] ); \
				ref = (java_objectheader *) (a->data[i]); \
				\
				code; \
			} \
		} \
	}

#define FOREACH_OBJECT_REF(o,ref,refptr,code) \
	{ \
		classinfo *c = o->vftbl->class; \
		fieldinfo *f; \
		int i; \
		\
		GC_ASSERT(c); \
		\
		for (; c; c = c->super.cls) { \
			for (i = 0; i < c->fieldscount; i++) { \
				f = &(c->fields[i]); \
				\
				if (!IS_ADR_TYPE(f->type) || (f->flags & ACC_STATIC)) \
					continue; \
				\
				refptr = (java_objectheader **) ((s1 *) o + f->offset); \
				ref = *( refptr ); \
				\
				code; \
			} \
		} \
	}



#endif /* _HEAP_H */

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
