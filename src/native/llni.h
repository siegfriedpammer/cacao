/* src/native/llni.h - low level native interfarce (LLNI) macros

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

   $Id: llni.h 8318 2007-08-16 10:05:34Z michi $

*/


#ifndef _LLNI_H
#define _LLNI_H

#include "config.h"


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
	LLNI_field_set_val(obj, field, reference)

#define LLNI_field_set_cls(obj, field, value) \
	LLNI_field_direct(obj, field) = (java_lang_Class *) (value)

#define LLNI_field_get_val(obj, field, variable) \
	(variable) = LLNI_field_direct(obj, field)

#define LLNI_field_get_ref(obj, field, variable) \
	LLNI_field_get_val(obj, field, variable)

#define LLNI_field_get_cls(obj, field, variable) \
	(variable) = (classinfo *) LLNI_field_direct(obj, field)

#define LLNI_class_get(obj, variable) \
	(variable) = LLNI_field_direct(obj, header.vftbl->class)

/* XXX the direct macros have to be used inside a critical section!!! */

#define LLNI_field_direct(obj, field) ((obj)->field) 

#define LLNI_vftbl_direct(obj) (((java_handle_t *) (obj))->vftbl)

#define LLNI_array_direct(arr, index) ((arr)->data[(index)])

#define LLNI_array_data(arr) ((arr)->data)

#define LLNI_array_size(arr) ((java_array_t *) (arr))->size

/* XXX document me */

#define LLNI_objectarray_element_set(arr, index, reference) \
	LLNI_array_direct(arr, index) = reference

#define LLNI_objectarray_element_get(arr, index, variable) \
	(variable) = (java_handle_t *) LLNI_array_direct(arr, index)



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
