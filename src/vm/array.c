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

   $Id: access.c 8318 2007-08-16 10:05:34Z michi $

*/


#include "config.h"

#include <stdint.h>

#include "native/llni.h"

#include "vm/array.h"
#include "vm/global.h"
#include "vm/vm.h"


/* array_element_primitive_get *************************************************

   Returns a primitive element of the given Java array.

*******************************************************************************/

imm_union array_element_primitive_get(java_handle_t *a, int32_t index)
{
	vftbl_t  *v;
	int       elementtype;
	imm_union value;

	v = LLNI_vftbl_direct(a);

	elementtype = v->arraydesc->elementtype;

	switch (elementtype) {
	case ARRAYTYPE_BOOLEAN:
		value.i = array_booleanarray_element_get(a, index);
		break;
	case ARRAYTYPE_BYTE:
		value.i = array_bytearray_element_get(a, index);
		break;
	case ARRAYTYPE_CHAR:
		value.i = array_chararray_element_get(a, index);
		break;
	case ARRAYTYPE_SHORT:
		value.i = array_shortarray_element_get(a, index);
		break;
	case ARRAYTYPE_INT:
		value.i = array_intarray_element_get(a, index);
		break;
	case ARRAYTYPE_LONG:
		value.l = array_longarray_element_get(a, index);
		break;
	case ARRAYTYPE_FLOAT:
		value.f = array_floatarray_element_get(a, index);
		break;
	case ARRAYTYPE_DOUBLE:
		value.d = array_doublearray_element_get(a, index);
		break;
	case ARRAYTYPE_OBJECT:
		value.a = array_objectarray_element_get(a, index);
		break;

	default:
		vm_abort("array_element_primitive_get: invalid array element type %d",
				 elementtype);
	}

	return value;
}


/* array_xxxarray_element_get **************************************************

   Returns a primitive element of the given Java array.

*******************************************************************************/

#define ARRAY_TYPEARRAY_ELEMENT_GET(name, type)                       \
type array_##name##array_element_get(java_handle_t *a, int32_t index) \
{                                                                     \
	java_handle_##name##array_t *ja;                                  \
	type                         value;                               \
                                                                      \
	ja = (java_handle_##name##array_t *) a;                           \
                                                                      \
	value = LLNI_array_direct(ja, index);                             \
                                                                      \
	return value;                                                     \
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
