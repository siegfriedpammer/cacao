/* src/native/vm/gnu/java_lang_reflect_Field.c

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

   $Id: java_lang_reflect_Field.c 8305 2007-08-15 13:49:26Z panzi $

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Boolean.h"
#include "native/include/java_lang_Byte.h"
#include "native/include/java_lang_Character.h"
#include "native/include/java_lang_Short.h"
#include "native/include/java_lang_Integer.h"
#include "native/include/java_lang_Long.h"
#include "native/include/java_lang_Float.h"
#include "native/include/java_lang_Double.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_String.h"

#include "native/include/java_lang_reflect_Field.h"

#if defined(ENABLE_ANNOTATIONS)
#include "native/include/sun_reflect_ConstantPool.h"
#include "native/vm/reflect.h"
#endif

#include "vm/access.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/primitive.h"
#include "vm/resolve.h"
#include "vm/stringlocal.h"

#include "vm/jit/stacktrace.h"

#include "vmcore/loader.h"
#include "vmcore/utf8.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "getModifiersInternal", "()I",                                     (void *) (intptr_t) &Java_java_lang_reflect_Field_getModifiersInternal },
	{ "getType",              "()Ljava/lang/Class;",                     (void *) (intptr_t) &Java_java_lang_reflect_Field_getType              },
	{ "get",                  "(Ljava/lang/Object;)Ljava/lang/Object;",  (void *) (intptr_t) &Java_java_lang_reflect_Field_get                  },
	{ "getBoolean",           "(Ljava/lang/Object;)Z",                   (void *) (intptr_t) &Java_java_lang_reflect_Field_getBoolean           },
	{ "getByte",              "(Ljava/lang/Object;)B",                   (void *) (intptr_t) &Java_java_lang_reflect_Field_getByte              },
	{ "getChar",              "(Ljava/lang/Object;)C",                   (void *) (intptr_t) &Java_java_lang_reflect_Field_getChar              },
	{ "getShort",             "(Ljava/lang/Object;)S",                   (void *) (intptr_t) &Java_java_lang_reflect_Field_getShort             },
	{ "getInt",               "(Ljava/lang/Object;)I",                   (void *) (intptr_t) &Java_java_lang_reflect_Field_getInt               },
	{ "getLong",              "(Ljava/lang/Object;)J",                   (void *) (intptr_t) &Java_java_lang_reflect_Field_getLong              },
	{ "getFloat",             "(Ljava/lang/Object;)F",                   (void *) (intptr_t) &Java_java_lang_reflect_Field_getFloat             },
	{ "getDouble",            "(Ljava/lang/Object;)D",                   (void *) (intptr_t) &Java_java_lang_reflect_Field_getDouble            },
	{ "set",                  "(Ljava/lang/Object;Ljava/lang/Object;)V", (void *) (intptr_t) &Java_java_lang_reflect_Field_set                  },
	{ "setBoolean",           "(Ljava/lang/Object;Z)V",                  (void *) (intptr_t) &Java_java_lang_reflect_Field_setBoolean           },
	{ "setByte",              "(Ljava/lang/Object;B)V",                  (void *) (intptr_t) &Java_java_lang_reflect_Field_setByte              },
	{ "setChar",              "(Ljava/lang/Object;C)V",                  (void *) (intptr_t) &Java_java_lang_reflect_Field_setChar              },
	{ "setShort",             "(Ljava/lang/Object;S)V",                  (void *) (intptr_t) &Java_java_lang_reflect_Field_setShort             },
	{ "setInt",               "(Ljava/lang/Object;I)V",                  (void *) (intptr_t) &Java_java_lang_reflect_Field_setInt               },
	{ "setLong",              "(Ljava/lang/Object;J)V",                  (void *) (intptr_t) &Java_java_lang_reflect_Field_setLong              },
	{ "setFloat",             "(Ljava/lang/Object;F)V",                  (void *) (intptr_t) &Java_java_lang_reflect_Field_setFloat             },
	{ "setDouble",            "(Ljava/lang/Object;D)V",                  (void *) (intptr_t) &Java_java_lang_reflect_Field_setDouble            },
	{ "getSignature",         "()Ljava/lang/String;",                    (void *) (intptr_t) &Java_java_lang_reflect_Field_getSignature         },
#if defined(ENABLE_ANNOTATIONS)
	{ "declaredAnnotations",  "()Ljava/util/Map;",                       (void *) (intptr_t) &Java_java_lang_reflect_Field_declaredAnnotations  },
#endif
};


/* _Jv_java_lang_reflect_Field_init ********************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_reflect_Field_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/reflect/Field");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/* cacao_get_field_address *****************************************************

   Return the address of a field of an object.

   IN:
      this.........the field (a java.lang.reflect.Field object)
	  o............the object of which to get the field

   RETURN VALUE:
      a pointer to the field, or
	  NULL if an exception has been thrown

*******************************************************************************/

static void *cacao_get_field_address(java_lang_reflect_Field *this,
									 java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;
	int32_t    flag;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* check field access */
	/* check if we should bypass security checks (AccessibleObject) */

	LLNI_field_get_val(this, flag, flag);
	if (flag == false) {
		/* this function is always called like this:

			   java.lang.reflect.Field.xxx (Native Method)
		   [0] <caller>
		*/
		if (!access_check_field(f, 0))
			return NULL;
	}

	/* get the address of the field */

	if (f->flags & ACC_STATIC) {
		/* initialize class if required */

		if (!(c->state & CLASS_INITIALIZED))
			if (!initialize_class(c))
				return NULL;

		/* return value pointer */

		return f->value;

	} else {
		/* obj is required for not-static fields */

		if (o == NULL) {
			exceptions_throw_nullpointerexception();
			return NULL;
		}
	
		if (builtin_instanceof((java_handle_t *) o, c))
			return (void *) (((intptr_t) o) + f->offset);
	}

	/* exception path */

	exceptions_throw_illegalargumentexception();

	return NULL;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getModifiersInternal
 * Signature: ()I
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_Field_getModifiersInternal(JNIEnv *env, java_lang_reflect_Field *this)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &(c->fields[slot]);

	return f->flags;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_reflect_Field_getType(JNIEnv *env, java_lang_reflect_Field *this)
{
	classinfo *c;
	typedesc  *desc;
	classinfo *ret;
	int32_t    slot;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	desc = c->fields[slot].parseddesc;

	if (desc == NULL)
		return NULL;

	if (!resolve_class_from_typedesc(desc, true, false, &ret))
		return NULL;
	
	return (java_lang_Class *) ret;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    get
 * Signature: (Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_Field_get(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *object)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;
	imm_union  value;
	java_handle_t *o;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get address of the source field value */

	if ((addr = cacao_get_field_address(this, object)) == NULL)
		return NULL;

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BOOLEAN:
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_CHAR:
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		value.i = *((int32_t *) addr);
		break;

	case PRIMITIVETYPE_LONG:
		value.l = *((int64_t *) addr);
		break;

	case PRIMITIVETYPE_FLOAT:
		value.f = *((float *) addr);
		break;

	case PRIMITIVETYPE_DOUBLE:
		value.d = *((double *) addr);
		break;

	case TYPE_ADR:
#warning this whole thing needs to be inside a critical section!
		return (java_lang_Object *) *((java_handle_t **) addr);
	}

	/* Now box the primitive types. */

	o = primitive_box(f->parseddesc->decltype, value);

	return (java_lang_Object *) o;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getBoolean
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_Field_getBoolean(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BOOLEAN:
		return (int32_t) *((int32_t *) addr);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getByte
 * Signature: (Ljava/lang/Object;)B
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_Field_getByte(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
		return (int32_t) *((int32_t *) addr);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getChar
 * Signature: (Ljava/lang/Object;)C
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_Field_getChar(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_CHAR:
		return (int32_t) *((int32_t *) addr);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getShort
 * Signature: (Ljava/lang/Object;)S
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_Field_getShort(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_SHORT:
		return (int32_t) *((int32_t *) addr);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getInt
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_Field_getInt(JNIEnv *env , java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_CHAR:
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		return (int32_t) *((int32_t *) addr);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getLong
 * Signature: (Ljava/lang/Object;)J
 */
JNIEXPORT int64_t JNICALL Java_java_lang_reflect_Field_getLong(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_CHAR:
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		return (int64_t) *((int32_t *) addr);
	case PRIMITIVETYPE_LONG:
		return (int64_t) *((int64_t *) addr);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getFloat
 * Signature: (Ljava/lang/Object;)F
 */
JNIEXPORT float JNICALL Java_java_lang_reflect_Field_getFloat(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_CHAR:
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		return (float) *((int32_t *) addr);
	case PRIMITIVETYPE_LONG:
		return (float) *((int64_t *) addr);
	case PRIMITIVETYPE_FLOAT:
		return (float) *((float *) addr);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getDouble
 * Signature: (Ljava/lang/Object;)D
 */
JNIEXPORT double JNICALL Java_java_lang_reflect_Field_getDouble(JNIEnv *env , java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_CHAR:
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		return (double) *((int32_t *) addr);
	case PRIMITIVETYPE_LONG:
		return (double) *((int64_t *) addr);
	case PRIMITIVETYPE_FLOAT:
		return (double) *((float *) addr);
	case PRIMITIVETYPE_DOUBLE:
		return (double) *((double *) addr);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    set
 * Signature: (Ljava/lang/Object;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_set(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, java_lang_Object *value)
{
	classinfo *sc;
	classinfo *dc;
	fieldinfo *sf;
	fieldinfo *df;
	void      *faddr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, dc);
	LLNI_field_get_val(this, slot , slot);
	df = &dc->fields[slot];

	/* get the address of the destination field */

	if ((faddr = cacao_get_field_address(this, o)) == NULL)
		return;

	/* get the source classinfo from the object */

	if (value == NULL)
		sc = NULL;
	else
		LLNI_class_get(value, sc);

	/* The fieldid is used to set the new value, for primitive
	   types the value has to be retrieved from the wrapping
	   object */

	switch (df->parseddesc->decltype) {
	case PRIMITIVETYPE_BOOLEAN: {
		int32_t val;

		/* determine the field to read the value */

		if ((sc == NULL) || !(sf = class_findfield(sc, utf_value, utf_Z)))
			break;

		switch (sf->parseddesc->decltype) {
		case PRIMITIVETYPE_BOOLEAN:
			LLNI_field_get_val((java_lang_Boolean *) value, value, val);
			break;
		default:
			exceptions_throw_illegalargumentexception();
			return;
		}

		*((int32_t *) faddr) = val;
		return;
	}

	case PRIMITIVETYPE_BYTE: {
		int32_t val;

		if ((sc == NULL) || !(sf = class_findfield(sc, utf_value, utf_B)))
			break;

		switch (sf->parseddesc->decltype) {
		case PRIMITIVETYPE_BYTE:
			LLNI_field_get_val((java_lang_Byte *) value, value, val);
			break;
		default:	
			exceptions_throw_illegalargumentexception();
			return;
		}

		*((int32_t *) faddr) = val;
		return;
	}

	case PRIMITIVETYPE_CHAR: {
		int32_t val;

		if ((sc == NULL) || !(sf = class_findfield(sc, utf_value, utf_C)))
			break;
				   
		switch (sf->parseddesc->decltype) {
		case PRIMITIVETYPE_CHAR:
			LLNI_field_get_val((java_lang_Character *) value, value, val);
			break;
		default:
			exceptions_throw_illegalargumentexception();
			return;
		}

		*((int32_t *) faddr) = val;
		return;
	}

	case PRIMITIVETYPE_SHORT: {
		int32_t val;

		/* get field only by name, it can be one of B, S */

		if ((sc == NULL) || !(sf = class_findfield_by_name(sc, utf_value)))
			break;
				   
		switch (sf->parseddesc->decltype) {
		case PRIMITIVETYPE_BYTE:
			LLNI_field_get_val((java_lang_Byte *) value, value, val);
			break;
		case PRIMITIVETYPE_SHORT:
			LLNI_field_get_val((java_lang_Short *) value, value, val);
			break;
		default:
			exceptions_throw_illegalargumentexception();
			return;
		}

		*((int32_t *) faddr) = val;
		return;
	}

	case PRIMITIVETYPE_INT: {
		int32_t val;

		/* get field only by name, it can be one of B, S, C, I */

		if ((sc == NULL) || !(sf = class_findfield_by_name(sc, utf_value)))
			break;

		switch (sf->parseddesc->decltype) {
		case PRIMITIVETYPE_BYTE:
			LLNI_field_get_val((java_lang_Byte *) value, value, val);
			break;
		case PRIMITIVETYPE_CHAR:
			LLNI_field_get_val((java_lang_Character *) value, value, val);
			break;
		case PRIMITIVETYPE_SHORT:
			LLNI_field_get_val((java_lang_Short *) value, value, val);
			break;
		case PRIMITIVETYPE_INT:
			LLNI_field_get_val((java_lang_Integer *) value, value, val);
			break;
		default:
			exceptions_throw_illegalargumentexception();
			return;
		}

		*((int32_t *) faddr) = val;
		return;
	}

	case PRIMITIVETYPE_LONG: {
		int64_t val;

		/* get field only by name, it can be one of B, S, C, I, J */

		if ((sc == NULL) || !(sf = class_findfield_by_name(sc, utf_value)))
			break;

		switch (sf->parseddesc->decltype) {
		case PRIMITIVETYPE_BYTE:
			LLNI_field_get_val((java_lang_Byte *) value, value, val);
			break;
		case PRIMITIVETYPE_CHAR:
			LLNI_field_get_val((java_lang_Character *) value, value, val);
			break;
		case PRIMITIVETYPE_SHORT:
			LLNI_field_get_val((java_lang_Short *) value, value, val);
			break;
		case PRIMITIVETYPE_INT:
			LLNI_field_get_val((java_lang_Integer *) value, value, val);
			break;
		case PRIMITIVETYPE_LONG:
			LLNI_field_get_val((java_lang_Long *) value, value, val);
			break;
		default:
			exceptions_throw_illegalargumentexception();
			return;
		}

		*((int64_t *) faddr) = val;
		return;
	}

	case PRIMITIVETYPE_FLOAT: {
		float val;

		/* get field only by name, it can be one of B, S, C, I, J, F */

		if ((sc == NULL) || !(sf = class_findfield_by_name(sc, utf_value)))
			break;

		switch (sf->parseddesc->decltype) {
		case PRIMITIVETYPE_BYTE:
			LLNI_field_get_val((java_lang_Byte *) value, value, val);
			break;
		case PRIMITIVETYPE_CHAR:
			LLNI_field_get_val((java_lang_Character *) value, value, val);
			break;
		case PRIMITIVETYPE_SHORT:
			LLNI_field_get_val((java_lang_Short *) value, value, val);
			break;
		case PRIMITIVETYPE_INT:
			LLNI_field_get_val((java_lang_Integer *) value, value, val);
			break;
		case PRIMITIVETYPE_LONG:
			LLNI_field_get_val((java_lang_Long *) value, value, val);
			break;
		case PRIMITIVETYPE_FLOAT:
			LLNI_field_get_val((java_lang_Float *) value, value, val);
			break;
		default:
			exceptions_throw_illegalargumentexception();
			return;
		}

		*((float *) faddr) = val;
		return;
	}

	case PRIMITIVETYPE_DOUBLE: {
		double val;

		/* get field only by name, it can be one of B, S, C, I, J, F, D */

		if ((sc == NULL) || !(sf = class_findfield_by_name(sc, utf_value)))
			break;

		switch (sf->parseddesc->decltype) {
		case PRIMITIVETYPE_BYTE:
			LLNI_field_get_val((java_lang_Byte *) value, value, val);
			break;
		case PRIMITIVETYPE_CHAR:
			LLNI_field_get_val((java_lang_Character *) value, value, val);
			break;
		case PRIMITIVETYPE_SHORT:
			LLNI_field_get_val((java_lang_Short *) value, value, val);
			break;
		case PRIMITIVETYPE_INT:
			LLNI_field_get_val((java_lang_Integer *) value, value, val);
			break;
		case PRIMITIVETYPE_LONG:
			LLNI_field_get_val((java_lang_Long *) value, value, val);
			break;
		case PRIMITIVETYPE_FLOAT:
			LLNI_field_get_val((java_lang_Float *) value, value, val);
			break;
		case PRIMITIVETYPE_DOUBLE:
			LLNI_field_get_val((java_lang_Double *) value, value, val);
			break;
		default:
			exceptions_throw_illegalargumentexception();
			return;
		}

		*((double *) faddr) = val;
		return;
	}

	case TYPE_ADR:
		/* check if value is an instance of the destination class */

		/* XXX TODO */
		/*  			if (!builtin_instanceof((java_handle_t *) value, df->class)) */
		/*  				break; */

		*((java_lang_Object **) faddr) = value;
		return;
	}

	/* raise exception */

	exceptions_throw_illegalargumentexception();
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setBoolean
 * Signature: (Ljava/lang/Object;Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, int32_t value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BOOLEAN:
		*((int32_t *) addr) = value;
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setByte
 * Signature: (Ljava/lang/Object;B)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, int32_t value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		*((int32_t *) addr) = value;
		break;
	case PRIMITIVETYPE_LONG:
		*((int64_t *) addr) = value;
		break;
	case PRIMITIVETYPE_FLOAT:
		*((float *) addr) = value;
		break;
	case PRIMITIVETYPE_DOUBLE:
		*((double *) addr) = value;
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setChar
 * Signature: (Ljava/lang/Object;C)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, int32_t value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_CHAR:
	case PRIMITIVETYPE_INT:
		*((int32_t *) addr) = value;
		break;
	case PRIMITIVETYPE_LONG:
		*((int64_t *) addr) = value;
		break;
	case PRIMITIVETYPE_FLOAT:
		*((float *) addr) = value;
		break;
	case PRIMITIVETYPE_DOUBLE:
		*((double *) addr) = value;
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setShort
 * Signature: (Ljava/lang/Object;S)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, int32_t value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		*((int32_t *) addr) = value;
		break;
	case PRIMITIVETYPE_LONG:
		*((int64_t *) addr) = value;
		break;
	case PRIMITIVETYPE_FLOAT:
		*((float *) addr) = value;
		break;
	case PRIMITIVETYPE_DOUBLE:
		*((double *) addr) = value;
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setInt
 * Signature: (Ljava/lang/Object;I)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, int32_t value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_INT:
		*((int32_t *) addr) = value;
		break;
	case PRIMITIVETYPE_LONG:
		*((int64_t *) addr) = value;
		break;
	case PRIMITIVETYPE_FLOAT:
		*((float *) addr) = value;
		break;
	case PRIMITIVETYPE_DOUBLE:
		*((double *) addr) = value;
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setLong
 * Signature: (Ljava/lang/Object;J)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, int64_t value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_LONG:
		*((int64_t *) addr) = value;
		break;
	case PRIMITIVETYPE_FLOAT:
		*((float *) addr) = value;
		break;
	case PRIMITIVETYPE_DOUBLE:
		*((double *) addr) = value;
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setFloat
 * Signature: (Ljava/lang/Object;F)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, float value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_FLOAT:
		*((float *) addr) = value;
		break;
	case PRIMITIVETYPE_DOUBLE:
		*((double *) addr) = value;
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setDouble
 * Signature: (Ljava/lang/Object;D)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, double value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(this, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_DOUBLE:
		*((double *) addr) = value;
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getSignature
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_reflect_Field_getSignature(JNIEnv *env, java_lang_reflect_Field* this)
{
	classinfo     *c;
	fieldinfo     *f;
	java_handle_t *o;
	int32_t        slot;

	/* get the class and the field */

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot , slot);
	f = &c->fields[slot];

	if (f->signature == NULL)
		return NULL;

	o = javastring_new(f->signature);

	/* in error case o is NULL */

	return (java_lang_String *) o;
}


#if defined(ENABLE_ANNOTATIONS)
/*
 * Class:     java/lang/reflect/Field
 * Method:    declaredAnnotations
 * Signature: ()Ljava/util/Map;
 */
JNIEXPORT struct java_util_Map* JNICALL Java_java_lang_reflect_Field_declaredAnnotations(JNIEnv *env, struct java_lang_reflect_Field* this)
{
	java_handle_t        *o                   = (java_handle_t*)this;
	struct java_util_Map *declaredAnnotations = NULL;
	java_bytearray       *annotations         = NULL;
	java_lang_Class      *declaringClass      = NULL;

	if (this == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	LLNI_field_get_ref(this, declaredAnnotations, declaredAnnotations);

	if (declaredAnnotations == NULL) {
		LLNI_field_get_val(this, annotations, annotations);
		LLNI_field_get_ref(this, clazz, declaringClass);

		declaredAnnotations = reflect_get_declaredannotatios(annotations, declaringClass, o->vftbl->class);

		LLNI_field_set_ref(this, declaredAnnotations, declaredAnnotations);
	}

	return declaredAnnotations;
}
#endif


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
