/* src/vm/array.h - array functions

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

   $Id: access.c 8318 2007-08-16 10:05:34Z michi $

*/


#ifndef _VM_ARRAY_H
#define _VM_ARRAY_H

#include "config.h"

#include <stdint.h>

#include "vm/global.h"
#include "vm/primitive.h"


/* array types ****************************************************************/

/* CAUTION: Don't change the numerical values! These constants (with
   the exception of ARRAYTYPE_OBJECT) are used as indices in the
   primitive type table. */

#define ARRAYTYPE_INT         PRIMITIVETYPE_INT
#define ARRAYTYPE_LONG        PRIMITIVETYPE_LONG
#define ARRAYTYPE_FLOAT       PRIMITIVETYPE_FLOAT
#define ARRAYTYPE_DOUBLE      PRIMITIVETYPE_DOUBLE
#define ARRAYTYPE_BYTE        PRIMITIVETYPE_BYTE
#define ARRAYTYPE_CHAR        PRIMITIVETYPE_CHAR
#define ARRAYTYPE_SHORT       PRIMITIVETYPE_SHORT
#define ARRAYTYPE_BOOLEAN     PRIMITIVETYPE_BOOLEAN
#define ARRAYTYPE_OBJECT      PRIMITIVETYPE_VOID     /* don't use as index! */


/* function prototypes ********************************************************/

java_handle_t *array_element_get(java_handle_t *a, int32_t index);
void           array_element_set(java_handle_t *a, int32_t index, java_handle_t *o);

imm_union      array_element_primitive_get(java_handle_t *a, int32_t index);
void           array_element_primitive_set(java_handle_t *a, int32_t index, imm_union value);

uint8_t        array_booleanarray_element_get(java_handle_booleanarray_t *a, int32_t index);
int8_t         array_bytearray_element_get(java_handle_bytearray_t *a, int32_t index);
uint16_t       array_chararray_element_get(java_handle_chararray_t *a, int32_t index);
int16_t        array_shortarray_element_get(java_handle_shortarray_t *a, int32_t index);
int32_t        array_intarray_element_get(java_handle_intarray_t *a, int32_t index);
int64_t        array_longarray_element_get(java_handle_longarray_t *a, int32_t index);
float          array_floatarray_element_get(java_handle_floatarray_t *a, int32_t index);
double         array_doublearray_element_get(java_handle_doublearray_t *a, int32_t index);
java_handle_t *array_objectarray_element_get(java_handle_objectarray_t *a, int32_t index);

void           array_booleanarray_element_set(java_handle_booleanarray_t *a, int32_t index, uint8_t value);
void           array_bytearray_element_set(java_handle_bytearray_t *a, int32_t index, int8_t value);
void           array_chararray_element_set(java_handle_chararray_t *a, int32_t index, uint16_t value);
void           array_shortarray_element_set(java_handle_shortarray_t *a, int32_t index, int16_t value);
void           array_intarray_element_set(java_handle_intarray_t *a, int32_t index, int32_t value);
void           array_longarray_element_set(java_handle_longarray_t *a, int32_t index, int64_t value);
void           array_floatarray_element_set(java_handle_floatarray_t *a, int32_t index, float value);
void           array_doublearray_element_set(java_handle_doublearray_t *a, int32_t index, double value);
void           array_objectarray_element_set(java_handle_objectarray_t *a, int32_t index, java_handle_t *value);

int32_t        array_length_get(java_handle_t *a);

#endif /* _VM_ARRAY_H */


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
