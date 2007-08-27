/* src/vm/array.c - array functions

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


#include "config.h"

#include <stdint.h>

#include "native/llni.h"

#include "vm/array.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/primitive.h"
#include "vm/vm.h"


/* array_element_get ***********************************************************

   Returns a boxed element of the given Java array.

*******************************************************************************/

java_handle_t *array_element_get(java_handle_t *a, int32_t index)
{
	vftbl_t       *v;
	int            type;
	imm_union      value;
	java_handle_t *o;

	v = LLNI_vftbl_direct(a);

	type = v->arraydesc->arraytype;

	value = array_element_primitive_get(a, index);

	o = primitive_box(type, value);

	return o;
}


/* array_element_set ***********************************************************

   Sets a boxed element in the given Java array.

*******************************************************************************/

void array_element_set(java_handle_t *a, int32_t index, java_handle_t *o)
{
	imm_union value;

	value = primitive_unbox(o);

	array_element_primitive_set(a, index, value);
}


/* array_element_primitive_get *************************************************

   Returns a primitive element of the given Java array.

*******************************************************************************/

imm_union array_element_primitive_get(java_handle_t *a, int32_t index)
{
	vftbl_t  *v;
	int       type;
	imm_union value;

	v = LLNI_vftbl_direct(a);

	type = v->arraydesc->arraytype;

	switch (type) {
	case ARRAYTYPE_BOOLEAN:
		value.i = array_booleanarray_element_get((java_handle_booleanarray_t *) a, index);
		break;
	case ARRAYTYPE_BYTE:
		value.i = array_bytearray_element_get((java_handle_bytearray_t *) a,
											  index);
		break;
	case ARRAYTYPE_CHAR:
		value.i = array_chararray_element_get((java_handle_chararray_t *) a,
											  index);
		break;
	case ARRAYTYPE_SHORT:
		value.i = array_shortarray_element_get((java_handle_shortarray_t *) a,
											   index);
		break;
	case ARRAYTYPE_INT:
		value.i = array_intarray_element_get((java_handle_intarray_t *) a,
											 index);
		break;
	case ARRAYTYPE_LONG:
		value.l = array_longarray_element_get((java_handle_longarray_t *) a,
											  index);
		break;
	case ARRAYTYPE_FLOAT:
		value.f = array_floatarray_element_get((java_handle_floatarray_t *) a,
											   index);
		break;
	case ARRAYTYPE_DOUBLE:
		value.d = array_doublearray_element_get((java_handle_doublearray_t *) a,
												index);
		break;
	case ARRAYTYPE_OBJECT:
		value.a = array_objectarray_element_get((java_handle_objectarray_t *) a,
												index);
		break;
	default:
		vm_abort("array_element_primitive_get: invalid array element type %d",
				 type);
	}

	return value;
}


/* array_element_primitive_set *************************************************

   Sets a primitive element in the given Java array.

*******************************************************************************/

void array_element_primitive_set(java_handle_t *a, int32_t index, imm_union value)
{
	vftbl_t *v;
	int      type;

	v = LLNI_vftbl_direct(a);

	type = v->arraydesc->arraytype;

	switch (type) {
	case ARRAYTYPE_BOOLEAN:
		array_booleanarray_element_set((java_handle_booleanarray_t *) a,
									   index, value.i);
		break;
	case ARRAYTYPE_BYTE:
		array_bytearray_element_set((java_handle_bytearray_t *) a,
									index, value.i);
		break;
	case ARRAYTYPE_CHAR:
		array_chararray_element_set((java_handle_chararray_t *) a,
									index, value.i);
		break;
	case ARRAYTYPE_SHORT:
		array_shortarray_element_set((java_handle_shortarray_t *) a,
									 index, value.i);
		break;
	case ARRAYTYPE_INT:
		array_intarray_element_set((java_handle_intarray_t *) a,
								   index, value.i);
		break;
	case ARRAYTYPE_LONG:
		array_longarray_element_set((java_handle_longarray_t *) a,
									index, value.l);
		break;
	case ARRAYTYPE_FLOAT:
		array_floatarray_element_set((java_handle_floatarray_t *) a,
									 index, value.f);
		break;
	case ARRAYTYPE_DOUBLE:
		array_doublearray_element_set((java_handle_doublearray_t *) a,
									  index, value.d);
		break;
	case ARRAYTYPE_OBJECT:
		array_objectarray_element_set((java_handle_objectarray_t *) a,
									  index, value.a);
		break;
	default:
		vm_abort("array_element_primitive_set: invalid array element type %d",
				 type);
	}
}


/* array_xxxarray_element_get **************************************************

   Returns a primitive element of the given Java array.

*******************************************************************************/

#define ARRAY_TYPEARRAY_ELEMENT_GET(name, type)                                \
type array_##name##array_element_get(java_handle_##name##array_t *a, int32_t index) \
{                                                                              \
	type    value;                                                             \
	int32_t size;                                                              \
                                                                               \
	if (a == NULL) {                                                           \
		exceptions_throw_nullpointerexception();                               \
		return (type) 0;                                                       \
	}                                                                          \
                                                                               \
	size = LLNI_array_size(a);                                                 \
                                                                               \
	if ((index < 0) || (index > size)) {                                       \
		exceptions_throw_arrayindexoutofboundsexception();                     \
		return (type) 0;                                                       \
	}                                                                          \
                                                                               \
	value = LLNI_array_direct(a, index);                                       \
                                                                               \
	return value;                                                              \
}

ARRAY_TYPEARRAY_ELEMENT_GET(boolean, uint8_t)
ARRAY_TYPEARRAY_ELEMENT_GET(byte,    int8_t)
ARRAY_TYPEARRAY_ELEMENT_GET(char,    uint16_t)
ARRAY_TYPEARRAY_ELEMENT_GET(short,   int16_t)
ARRAY_TYPEARRAY_ELEMENT_GET(int,     int32_t)
ARRAY_TYPEARRAY_ELEMENT_GET(long,    int64_t)
ARRAY_TYPEARRAY_ELEMENT_GET(float,   float)
ARRAY_TYPEARRAY_ELEMENT_GET(double,  double)
ARRAY_TYPEARRAY_ELEMENT_GET(object,  java_handle_t*)


/* array_xxxarray_element_set **************************************************

   Sets a primitive element in the given Java array.

*******************************************************************************/

#define ARRAY_TYPEARRAY_ELEMENT_SET(name, type)                                \
void array_##name##array_element_set(java_handle_##name##array_t *a, int32_t index, type value) \
{                                                                              \
	int32_t size;                                                              \
                                                                               \
	if (a == NULL) {                                                           \
		exceptions_throw_nullpointerexception();                               \
		return;                                                                \
	}                                                                          \
                                                                               \
	size = LLNI_array_size(a);                                                 \
                                                                               \
	if ((index < 0) || (index > size)) {                                       \
		exceptions_throw_arrayindexoutofboundsexception();                     \
		return;                                                                \
	}                                                                          \
                                                                               \
	LLNI_array_direct(a, index) = value;                                       \
}

ARRAY_TYPEARRAY_ELEMENT_SET(boolean, uint8_t)
ARRAY_TYPEARRAY_ELEMENT_SET(byte,    int8_t)
ARRAY_TYPEARRAY_ELEMENT_SET(char,    uint16_t)
ARRAY_TYPEARRAY_ELEMENT_SET(short,   int16_t)
ARRAY_TYPEARRAY_ELEMENT_SET(int,     int32_t)
ARRAY_TYPEARRAY_ELEMENT_SET(long,    int64_t)
ARRAY_TYPEARRAY_ELEMENT_SET(float,   float)
ARRAY_TYPEARRAY_ELEMENT_SET(double,  double)
ARRAY_TYPEARRAY_ELEMENT_SET(object,  java_handle_t*)


/* array_length_get ***********************************************************

   Returns a the length of the given Java array.

*******************************************************************************/

int32_t array_length_get(java_handle_t *a)
{
	vftbl_t *v;
	int32_t  size;

	if (a == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}

	v = LLNI_vftbl_direct(a);

	if (!class_is_array(v->class)) {
/* 		exceptions_throw_illegalargumentexception("Argument is not an array"); */
		exceptions_throw_illegalargumentexception();
		return 0;
	}

	size = LLNI_array_size(a);

	return size;
}


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
