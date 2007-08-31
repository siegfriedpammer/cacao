/* src/native/llni.h - low level native interfarce (LLNI)

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
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


#ifndef _LLNI_H
#define _LLNI_H

#include "config.h"

#include "native/localref.h"


/* LLNI macros *****************************************************************

   The following macros should be used whenever a Java Object is
   accessed in native code without the use of an JNI function.

   LLNI_field_set_val, LLNI_field_get_val:
     Deal with primitive values like integer and float values. Do
     not use these macros to access pointers or references!

   LLNI_field_set_ref, LLNI_field_get_ref:
     Deal with references to other objects.

   LLNI_field_set_cls, LLNI_field_get_cls:
     Deal with references to Java Classes which are internally
     represented by classinfo or java_lang_Class.

*******************************************************************************/

#define LLNI_field_set_val(obj, field, value) \
	LLNI_field_direct(obj, field) = (value)

#define LLNI_field_set_ref(obj, field, reference) \
	LLNI_field_direct(obj, field) = LLNI_DIRECT(reference)

#define LLNI_field_set_cls(obj, field, value) \
	LLNI_field_direct(obj, field) = (java_lang_Class *) (value)

#define LLNI_field_get_val(obj, field, variable) \
	(variable) = LLNI_field_direct(obj, field)

#define LLNI_field_get_ref(obj, field, variable) \
	(variable) = LLNI_WRAP(LLNI_field_direct(obj, field))

#define LLNI_field_get_cls(obj, field, variable) \
	(variable) = (classinfo *) LLNI_field_direct(obj, field)

#define LLNI_class_get(obj, variable) \
	(variable) = LLNI_field_direct((java_handle_t *) obj, vftbl->class)


/* LLNI classinfo wrapping / unwrapping macros *********************************

   The following macros are used to wrap or unwrap a classinfo from
   or into a handle (typically java_lang_Class). No critical sections
   are needed here, because classinfos are not placed on the GC heap.

   XXX This might change once we implement Class Unloading!

*******************************************************************************/

#define LLNI_classinfo_wrap(classinfo) \
	((java_lang_Class *) LLNI_WRAP(classinfo))

#define LLNI_classinfo_unwrap(clazz) \
	((classinfo *) LLNI_UNWRAP((java_handle_t *) (clazz)))


/* XXX the direct macros have to be used inside a critical section!!! */

#define LLNI_field_direct(obj, field) (LLNI_DIRECT(obj)->field)
#define LLNI_vftbl_direct(obj)        (LLNI_DIRECT((java_handle_t *) (obj))->vftbl)
#define LLNI_array_direct(arr, index) (LLNI_DIRECT(arr)->data[(index)])
#define LLNI_array_data(arr)          (LLNI_DIRECT(arr)->data)
#define LLNI_array_size(arr)          (LLNI_DIRECT((java_handle_objectarray_t *) (arr))->header.size)


/* XXX document me */

#define LLNI_objectarray_element_set(arr, index, reference) \
	LLNI_array_direct(arr, index) = LLNI_DIRECT(reference)

#define LLNI_objectarray_element_get(arr, index, variable) \
	(variable) = LLNI_WRAP(LLNI_array_direct(arr, index))


/* LLNI wrapping / unwrapping macros *******************************************

   ATTENTION: Only use these macros inside a LLNI critical section!
   Once the ciritical section ends, all pointers onto the GC heap
   retrieved through these macros are void!

*******************************************************************************/

#if defined(ENABLE_HANDLES)
# define LLNI_WRAP(obj)   ((obj) == NULL ? NULL : localref_add(obj))
# define LLNI_UNWRAP(hdl) ((hdl) == NULL ? NULL : (hdl)->heap_object)
# define LLNI_DIRECT(obj) ((obj)->heap_object)
#else
# define LLNI_WRAP(obj)   ((java_handle_t *) obj)
# define LLNI_UNWRAP(hdl) ((java_object_t *) hdl)
# define LLNI_DIRECT(obj) (obj)
#endif


/* LLNI critical sections ******************************************************

   These macros handle the LLNI critical sections. While a critical
   section is active, the absolute position of objects on the GC heap
   is not allowed to change (no moving Garbage Collection).

   ATTENTION: Use a critical section whenever you have a direct
   pointer onto the GC heap!

*******************************************************************************/

#if defined(ENABLE_THREADS) && defined(ENABLE_GC_CACAO)
void llni_critical_start(void);
void llni_critical_end(void);
# define LLNI_CRITICAL_START llni_critical_start()
# define LLNI_CRITICAL_END   llni_critical_end()
#else
# define LLNI_CRITICAL_START
# define LLNI_CRITICAL_END
#endif


#endif /* _LLNI_H */


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
