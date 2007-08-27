/* src/vm/primitive.c - primitive types

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

#include <assert.h>
#include <stdint.h>

#include "native/jni.h"
#include "native/llni.h"

#include "native/include/java_lang_Boolean.h"
#include "native/include/java_lang_Byte.h"
#include "native/include/java_lang_Short.h"
#include "native/include/java_lang_Character.h"
#include "native/include/java_lang_Integer.h"
#include "native/include/java_lang_Long.h"
#include "native/include/java_lang_Float.h"
#include "native/include/java_lang_Double.h"

#include "vm/builtin.h"
#include "vm/global.h"
#include "vm/primitive.h"
#include "vm/vm.h"

#include "vmcore/class.h"
#include "vmcore/utf8.h"


/* primitive_class_get_by_name *************************************************

   Returns the primitive class of the given class name.

*******************************************************************************/

classinfo *primitive_class_get_by_name(utf *name)
{
	int i;

	/* search table of primitive classes */

	for (i = 0; i < PRIMITIVETYPE_COUNT; i++)
		if (primitivetype_table[i].name == name)
			return primitivetype_table[i].class_primitive;

	/* keep compiler happy */

	return NULL;
}


/* primitive_class_get_by_type *************************************************

   Returns the primitive class of the given type.

*******************************************************************************/

classinfo *primitive_class_get_by_type(int type)
{
	return primitivetype_table[type].class_primitive;
}


/* primitive_class_get_by_char *************************************************

   Returns the primitive class of the given type.

*******************************************************************************/

classinfo *primitive_class_get_by_char(char ch)
{
	int index;

	switch (ch) {
	case 'I':
		index = PRIMITIVETYPE_INT;
		break;
	case 'J':
		index = PRIMITIVETYPE_LONG;
		break;
	case 'F':
		index = PRIMITIVETYPE_FLOAT;
		break;
	case 'D':
		index = PRIMITIVETYPE_DOUBLE;
		break;
	case 'B':
		index = PRIMITIVETYPE_BYTE;
		break;
	case 'C':
		index = PRIMITIVETYPE_CHAR;
		break;
	case 'S':
		index = PRIMITIVETYPE_SHORT;
		break;
	case 'Z':
		index = PRIMITIVETYPE_BOOLEAN;
		break;
	case 'V':
		index = PRIMITIVETYPE_VOID;
		break;
	default:
		return NULL;
	}

	return primitivetype_table[index].class_primitive;
}


/* primitive_arrayclass_get_by_name ********************************************

   Returns the primitive array-class of the given primitive class
   name.

*******************************************************************************/

classinfo *primitive_arrayclass_get_by_name(utf *name)
{
	int i;

	/* search table of primitive classes */

	for (i = 0; i < PRIMITIVETYPE_COUNT; i++)
		if (primitivetype_table[i].name == name)
			return primitivetype_table[i].arrayclass;

	/* keep compiler happy */

	return NULL;
}


/* primitive_arrayclass_get_by_type ********************************************

   Returns the primitive array-class of the given type.

*******************************************************************************/

classinfo *primitive_arrayclass_get_by_type(int type)
{
	return primitivetype_table[type].arrayclass;
}


/* primitive_type_get_by_wrapperclass ******************************************

   Returns the primitive type of the given wrapper-class.

*******************************************************************************/

int primitive_type_get_by_wrapperclass(classinfo *c)
{
	int i;

	/* Search primitive table. */

	for (i = 0; i < PRIMITIVETYPE_COUNT; i++)
		if (primitivetype_table[i].class_wrap == c)
			return i;

	/* Invalid primitive wrapper-class. */

	return -1;
}


/* primitive_box ***************************************************************

   Box a primitive of the given type.  If the type is an object,
   simply return it.

*******************************************************************************/

java_handle_t *primitive_box(int type, imm_union value)
{
	java_handle_t *o;

	switch (type) {
	case PRIMITIVETYPE_BOOLEAN:
		o = primitive_box_boolean(value.i);
		break;
	case PRIMITIVETYPE_BYTE:
		o = primitive_box_byte(value.i);
		break;
	case PRIMITIVETYPE_CHAR:
		o = primitive_box_char(value.i);
		break;
	case PRIMITIVETYPE_SHORT:
		o = primitive_box_short(value.i);
		break;
	case PRIMITIVETYPE_INT:
		o = primitive_box_int(value.i);
		break;
	case PRIMITIVETYPE_LONG:
		o = primitive_box_long(value.l);
		break;
	case PRIMITIVETYPE_FLOAT:
		o = primitive_box_float(value.f);
		break;
	case PRIMITIVETYPE_DOUBLE:
		o = primitive_box_double(value.d);
		break;
	case PRIMITIVETYPE_VOID:
		o = value.a;
		break;
	default:
		vm_abort("primitive_box: invalid primitive type %d", type);
	}

	return o;
}


/* primitive_unbox *************************************************************

   Unbox a primitive of the given type.  If the type is an object,
   simply return it.

*******************************************************************************/

imm_union primitive_unbox(java_handle_t *o)
{
	classinfo *c;
	int        type;
	imm_union  value;

	c = o->vftbl->class;

	type = primitive_type_get_by_wrapperclass(c);

	switch (type) {
	case PRIMITIVETYPE_BOOLEAN:
		value.i = primitive_unbox_boolean(o);
		break;
	case PRIMITIVETYPE_BYTE:
		value.i = primitive_unbox_byte(o);
		break;
	case PRIMITIVETYPE_CHAR:
		value.i = primitive_unbox_char(o);
		break;
	case PRIMITIVETYPE_SHORT:
		value.i = primitive_unbox_short(o);
		break;
	case PRIMITIVETYPE_INT:
		value.i = primitive_unbox_int(o);
		break;
	case PRIMITIVETYPE_LONG:
		value.l = primitive_unbox_long(o);
		break;
	case PRIMITIVETYPE_FLOAT:
		value.f = primitive_unbox_float(o);
		break;
	case PRIMITIVETYPE_DOUBLE:
		value.d = primitive_unbox_double(o);
		break;
	case -1:
		/* If type is -1 the object is not a primitive box but a
		   normal object. */
		value.a = o;
		break;
	default:
		vm_abort("primitive_unbox: invalid primitive type %d", type);
	}

	return value;
}


/* primitive_box_xxx ***********************************************************

   Box a primitive type.

*******************************************************************************/

#define PRIMITIVE_BOX_TYPE(name, object, type)      \
java_handle_t *primitive_box_##name(type value)     \
{                                                   \
	java_handle_t      *o;                          \
	java_lang_##object *jo;                         \
                                                    \
	o = builtin_new(class_java_lang_##object);      \
                                                    \
	if (o == NULL)                                  \
		return NULL;                                \
                                                    \
	jo = (java_lang_##object *) o;                  \
                                                    \
	LLNI_field_set_val(jo, value, value);           \
                                                    \
	return o;                                       \
}

PRIMITIVE_BOX_TYPE(boolean, Boolean,   int32_t)
PRIMITIVE_BOX_TYPE(byte,    Byte,      int32_t)
PRIMITIVE_BOX_TYPE(char,    Character, int32_t)
PRIMITIVE_BOX_TYPE(short,   Short,     int32_t)
PRIMITIVE_BOX_TYPE(int,     Integer,   int32_t)
PRIMITIVE_BOX_TYPE(long,    Long,      int64_t)
PRIMITIVE_BOX_TYPE(float,   Float,     float)
PRIMITIVE_BOX_TYPE(double,  Double,    double)


/* primitive_unbox_xxx *********************************************************

   Unbox a primitive type.

*******************************************************************************/

#define PRIMITIVE_UNBOX_TYPE(name, object, type)  \
type primitive_unbox_##name(java_handle_t *o)     \
{                                                 \
	java_lang_##object *jo;                       \
	type                value;                    \
                                                  \
	jo = (java_lang_##object *) o;                \
                                                  \
	LLNI_field_get_val(jo, value, value);         \
                                                  \
	return value;                                 \
}

PRIMITIVE_UNBOX_TYPE(boolean, Boolean,   int32_t)
PRIMITIVE_UNBOX_TYPE(byte,    Byte,      int32_t)
PRIMITIVE_UNBOX_TYPE(char,    Character, int32_t)
PRIMITIVE_UNBOX_TYPE(short,   Short,     int32_t)
PRIMITIVE_UNBOX_TYPE(int,     Integer,   int32_t)
PRIMITIVE_UNBOX_TYPE(long,    Long,      int64_t)
PRIMITIVE_UNBOX_TYPE(float,   Float,     float)
PRIMITIVE_UNBOX_TYPE(double,  Double,    double)


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
