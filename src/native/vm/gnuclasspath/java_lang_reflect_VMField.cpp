/* src/native/vm/gnuclasspath/java_lang_reflect_VMField.cpp

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

// FIXME
extern "C" {
#include "native/include/java_lang_reflect_VMField.h"
}

#if defined(ENABLE_ANNOTATIONS)
#include "native/include/java_util_Map.h"
#include "native/include/sun_reflect_ConstantPool.h"
#include "native/vm/reflect.h"
#endif

#include "vm/access.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/primitive.hpp"
#include "vm/resolve.h"
#include "vm/stringlocal.h"

#include "vmcore/loader.h"
#include "vmcore/utf8.h"


/* native methods implemented by this file ************************************/

static const JNINativeMethod methods[] = {
	{ (char*) "getModifiersInternal", (char*) "()I",                                     (void*) (uintptr_t) &Java_java_lang_reflect_VMField_getModifiersInternal },
	{ (char*) "getType",              (char*) "()Ljava/lang/Class;",                     (void*) (uintptr_t) &Java_java_lang_reflect_VMField_getType              },
	{ (char*) "get",                  (char*) "(Ljava/lang/Object;)Ljava/lang/Object;",  (void*) (uintptr_t) &Java_java_lang_reflect_VMField_get                  },
	{ (char*) "getBoolean",           (char*) "(Ljava/lang/Object;)Z",                   (void*) (uintptr_t) &Java_java_lang_reflect_VMField_getBoolean           },
	{ (char*) "getByte",              (char*) "(Ljava/lang/Object;)B",                   (void*) (uintptr_t) &Java_java_lang_reflect_VMField_getByte              },
	{ (char*) "getChar",              (char*) "(Ljava/lang/Object;)C",                   (void*) (uintptr_t) &Java_java_lang_reflect_VMField_getChar              },
	{ (char*) "getShort",             (char*) "(Ljava/lang/Object;)S",                   (void*) (uintptr_t) &Java_java_lang_reflect_VMField_getShort             },
	{ (char*) "getInt",               (char*) "(Ljava/lang/Object;)I",                   (void*) (uintptr_t) &Java_java_lang_reflect_VMField_getInt               },
	{ (char*) "getLong",              (char*) "(Ljava/lang/Object;)J",                   (void*) (uintptr_t) &Java_java_lang_reflect_VMField_getLong              },
	{ (char*) "getFloat",             (char*) "(Ljava/lang/Object;)F",                   (void*) (uintptr_t) &Java_java_lang_reflect_VMField_getFloat             },
	{ (char*) "getDouble",            (char*) "(Ljava/lang/Object;)D",                   (void*) (uintptr_t) &Java_java_lang_reflect_VMField_getDouble            },
	{ (char*) "set",                  (char*) "(Ljava/lang/Object;Ljava/lang/Object;)V", (void*) (uintptr_t) &Java_java_lang_reflect_VMField_set                  },
	{ (char*) "setBoolean",           (char*) "(Ljava/lang/Object;Z)V",                  (void*) (uintptr_t) &Java_java_lang_reflect_VMField_setBoolean           },
	{ (char*) "setByte",              (char*) "(Ljava/lang/Object;B)V",                  (void*) (uintptr_t) &Java_java_lang_reflect_VMField_setByte              },
	{ (char*) "setChar",              (char*) "(Ljava/lang/Object;C)V",                  (void*) (uintptr_t) &Java_java_lang_reflect_VMField_setChar              },
	{ (char*) "setShort",             (char*) "(Ljava/lang/Object;S)V",                  (void*) (uintptr_t) &Java_java_lang_reflect_VMField_setShort             },
	{ (char*) "setInt",               (char*) "(Ljava/lang/Object;I)V",                  (void*) (uintptr_t) &Java_java_lang_reflect_VMField_setInt               },
	{ (char*) "setLong",              (char*) "(Ljava/lang/Object;J)V",                  (void*) (uintptr_t) &Java_java_lang_reflect_VMField_setLong              },
	{ (char*) "setFloat",             (char*) "(Ljava/lang/Object;F)V",                  (void*) (uintptr_t) &Java_java_lang_reflect_VMField_setFloat             },
	{ (char*) "setDouble",            (char*) "(Ljava/lang/Object;D)V",                  (void*) (uintptr_t) &Java_java_lang_reflect_VMField_setDouble            },
	{ (char*) "getSignature",         (char*) "()Ljava/lang/String;",                    (void*) (uintptr_t) &Java_java_lang_reflect_VMField_getSignature         },
#if defined(ENABLE_ANNOTATIONS)
	{ (char*) "declaredAnnotations",  (char*) "()Ljava/util/Map;",                       (void*) (uintptr_t) &Java_java_lang_reflect_VMField_declaredAnnotations  },
#endif
};


/* _Jv_java_lang_reflect_VMField_init ******************************************

   Register native functions.

*******************************************************************************/

// FIXME
extern "C" {
void _Jv_java_lang_reflect_VMField_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/reflect/VMField");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}
}


/* _field_access_check *********************************************************

   Checks if the field can be accessed.

   RETURN VALUE:
      true......field can be accessed, or
      false.....otherwise (maybe an Exception was thrown).

*******************************************************************************/

static bool _field_access_check(java_lang_reflect_VMField *_this,
								fieldinfo *f, java_handle_t *o)
{
	java_lang_reflect_Field *rf;
	int32_t                  flag;

	/* check if we should bypass security checks (AccessibleObject) */

	LLNI_field_get_ref(_this, f, rf);
	LLNI_field_get_val(rf, flag, flag);

	if (flag == false) {
		/* This function is always called like this:
		       [0] java.lang.reflect.VMField.xxx (Native Method)
		       [1] java.lang.reflect.Field.xxx
		       [2] <caller>
		*/

		if (!access_check_field(f, 2))
			return false;
	}

	/* some general checks */

	if (f->flags & ACC_STATIC) {
		/* initialize class if required */

		if (!(f->clazz->state & CLASS_INITIALIZED))
			if (!initialize_class(f->clazz))
				return false;

		/* everything is ok */

		return true;
	}
	else {
		/* obj is required for not-static fields */

		if (o == NULL) {
			exceptions_throw_nullpointerexception();
			return false;
		}
	
		if (builtin_instanceof(o, f->clazz))
			return true;
	}

	/* exception path */

	exceptions_throw_illegalargumentexception();
	return false;
}


/* _field_get_type *************************************************************

   Returns the content of the given field.

*******************************************************************************/

#define _FIELD_GET_TYPE(name, type, uniontype) \
static inline type _field_get_##name(fieldinfo *f, java_lang_Object *o) \
{ \
	type ret; \
	if (f->flags & ACC_STATIC) { \
		ret = f->value->uniontype; \
	} else { \
		LLNI_CRITICAL_START; \
		ret = *(type *) (((intptr_t) LLNI_DIRECT(o)) + f->offset); \
		LLNI_CRITICAL_END; \
	} \
	return ret; \
}

static inline java_handle_t *_field_get_handle(fieldinfo *f, java_lang_Object *o)
{
	java_object_t *obj;
	java_handle_t *hdl;

	LLNI_CRITICAL_START;

	if (f->flags & ACC_STATIC) {
		obj = (java_object_t*) f->value->a;
	} else {
		obj = *(java_object_t **) (((intptr_t) LLNI_DIRECT(o)) + f->offset);
	}

	hdl = LLNI_WRAP(obj);

	LLNI_CRITICAL_END;

	return hdl;
}

_FIELD_GET_TYPE(int,    int32_t, i)
_FIELD_GET_TYPE(long,   int64_t, l)
_FIELD_GET_TYPE(float,  float,   f)
_FIELD_GET_TYPE(double, double,  d)


/* _field_set_type *************************************************************

   Sets the content of the given field to the given value.

*******************************************************************************/

#define _FIELD_SET_TYPE(name, type, uniontype) \
static inline void _field_set_##name(fieldinfo *f, java_lang_Object *o, type value) \
{ \
	if (f->flags & ACC_STATIC) { \
		f->value->uniontype = value; \
	} else { \
		LLNI_CRITICAL_START; \
		*(type *) (((intptr_t) LLNI_DIRECT(o)) + f->offset) = value; \
		LLNI_CRITICAL_END; \
	} \
}

static inline void _field_set_handle(fieldinfo *f, java_lang_Object *o, java_handle_t *value)
{
	LLNI_CRITICAL_START;

	if (f->flags & ACC_STATIC) {
		f->value->a = LLNI_DIRECT(value);
	} else {
		*(java_object_t **) (((intptr_t) LLNI_DIRECT(o)) + f->offset) = LLNI_DIRECT(value);
	}

	LLNI_CRITICAL_END;
}

_FIELD_SET_TYPE(int,    int32_t, i)
_FIELD_SET_TYPE(long,   int64_t, l)
_FIELD_SET_TYPE(float,  float,   f)
_FIELD_SET_TYPE(double, double,  d)


extern "C" {

/*
 * Class:     java/lang/reflect/VMField
 * Method:    getModifiersInternal
 * Signature: ()I
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_VMField_getModifiersInternal(JNIEnv *env, java_lang_reflect_VMField *_this)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &(c->fields[slot]);

	return f->flags;
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    getType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_reflect_VMField_getType(JNIEnv *env, java_lang_reflect_VMField *_this)
{
	classinfo *c;
	typedesc  *desc;
	classinfo *ret;
	int32_t    slot;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	desc = c->fields[slot].parseddesc;

	if (desc == NULL)
		return NULL;

	if (!resolve_class_from_typedesc(desc, true, false, &ret))
		return NULL;
	
	return LLNI_classinfo_wrap(ret);
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    get
 * Signature: (Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_VMField_get(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;
	imm_union  value;
	java_handle_t *object;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return NULL;

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BOOLEAN:
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_CHAR:
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		value.i = _field_get_int(f, o);
		break;

	case PRIMITIVETYPE_LONG:
		value.l = _field_get_long(f, o);
		break;

	case PRIMITIVETYPE_FLOAT:
		value.f = _field_get_float(f, o);
		break;

	case PRIMITIVETYPE_DOUBLE:
		value.d = _field_get_double(f, o);
		break;

	case TYPE_ADR:
		return (java_lang_Object *) _field_get_handle(f, o);
	}

	/* Now box the primitive types. */

	object = Primitive::box(f->parseddesc->decltype, value);

	return (java_lang_Object *) object;
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    getBoolean
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_VMField_getBoolean(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BOOLEAN:
		return (int32_t) _field_get_int(f, o);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    getByte
 * Signature: (Ljava/lang/Object;)B
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_VMField_getByte(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
		return (int32_t) _field_get_int(f, o);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    getChar
 * Signature: (Ljava/lang/Object;)C
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_VMField_getChar(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_CHAR:
		return (int32_t) _field_get_int(f, o);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    getShort
 * Signature: (Ljava/lang/Object;)S
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_VMField_getShort(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_SHORT:
		return (int32_t) _field_get_int(f, o);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    getInt
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_VMField_getInt(JNIEnv *env , java_lang_reflect_VMField *_this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_CHAR:
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		return (int32_t) _field_get_int(f, o);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    getLong
 * Signature: (Ljava/lang/Object;)J
 */
JNIEXPORT int64_t JNICALL Java_java_lang_reflect_VMField_getLong(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_CHAR:
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		return (int64_t) _field_get_int(f, o);
	case PRIMITIVETYPE_LONG:
		return (int64_t) _field_get_long(f, o);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    getFloat
 * Signature: (Ljava/lang/Object;)F
 */
JNIEXPORT float JNICALL Java_java_lang_reflect_VMField_getFloat(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_CHAR:
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		return (float) _field_get_int(f, o);
	case PRIMITIVETYPE_LONG:
		return (float) _field_get_long(f, o);
	case PRIMITIVETYPE_FLOAT:
		return (float) _field_get_float(f, o);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    getDouble
 * Signature: (Ljava/lang/Object;)D
 */
JNIEXPORT double JNICALL Java_java_lang_reflect_VMField_getDouble(JNIEnv *env , java_lang_reflect_VMField *_this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return 0;

	/* check the field type and return the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_CHAR:
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		return (double) _field_get_int(f, o);
	case PRIMITIVETYPE_LONG:
		return (double) _field_get_long(f, o);
	case PRIMITIVETYPE_FLOAT:
		return (double) _field_get_float(f, o);
	case PRIMITIVETYPE_DOUBLE:
		return (double) _field_get_double(f, o);
	default:
		exceptions_throw_illegalargumentexception();
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    set
 * Signature: (Ljava/lang/Object;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_VMField_set(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o, java_lang_Object *value)
{
	classinfo *sc;
	classinfo *dc;
	fieldinfo *sf;
	fieldinfo *df;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, dc);
	LLNI_field_get_val(_this, slot , slot);
	df = &dc->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, df, (java_handle_t *) o))
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

		_field_set_int(df, o, val);
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

		_field_set_int(df, o, val);
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

		_field_set_int(df, o, val);
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

		_field_set_int(df, o, val);
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

		_field_set_int(df, o, val);
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

		_field_set_long(df, o, val);
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

		_field_set_float(df, o, val);
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

		_field_set_double(df, o, val);
		return;
	}

	case TYPE_ADR:
		/* check if value is an instance of the destination class */

		/* XXX TODO */
		/*  			if (!builtin_instanceof((java_handle_t *) value, df->class)) */
		/*  				break; */

		_field_set_handle(df, o, (java_handle_t *) value);
		return;
	}

	/* raise exception */

	exceptions_throw_illegalargumentexception();
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    setBoolean
 * Signature: (Ljava/lang/Object;Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_VMField_setBoolean(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o, int32_t value)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BOOLEAN:
		_field_set_int(f, o, value);
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    setByte
 * Signature: (Ljava/lang/Object;B)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_VMField_setByte(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o, int32_t value)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_BYTE:
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		_field_set_int(f, o, value);
		break;
	case PRIMITIVETYPE_LONG:
		_field_set_long(f, o, value);
		break;
	case PRIMITIVETYPE_FLOAT:
		_field_set_float(f, o, value);
		break;
	case PRIMITIVETYPE_DOUBLE:
		_field_set_double(f, o, value);
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    setChar
 * Signature: (Ljava/lang/Object;C)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_VMField_setChar(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o, int32_t value)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_CHAR:
	case PRIMITIVETYPE_INT:
		_field_set_int(f, o, value);
		break;
	case PRIMITIVETYPE_LONG:
		_field_set_long(f, o, value);
		break;
	case PRIMITIVETYPE_FLOAT:
		_field_set_float(f, o, value);
		break;
	case PRIMITIVETYPE_DOUBLE:
		_field_set_double(f, o, value);
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    setShort
 * Signature: (Ljava/lang/Object;S)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_VMField_setShort(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o, int32_t value)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_SHORT:
	case PRIMITIVETYPE_INT:
		_field_set_int(f, o, value);
		break;
	case PRIMITIVETYPE_LONG:
		_field_set_long(f, o, value);
		break;
	case PRIMITIVETYPE_FLOAT:
		_field_set_float(f, o, value);
		break;
	case PRIMITIVETYPE_DOUBLE:
		_field_set_double(f, o, value);
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    setInt
 * Signature: (Ljava/lang/Object;I)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_VMField_setInt(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o, int32_t value)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_INT:
		_field_set_int(f, o, value);
		break;
	case PRIMITIVETYPE_LONG:
		_field_set_long(f, o, value);
		break;
	case PRIMITIVETYPE_FLOAT:
		_field_set_float(f, o, value);
		break;
	case PRIMITIVETYPE_DOUBLE:
		_field_set_double(f, o, value);
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    setLong
 * Signature: (Ljava/lang/Object;J)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_VMField_setLong(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o, int64_t value)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_LONG:
		_field_set_long(f, o, value);
		break;
	case PRIMITIVETYPE_FLOAT:
		_field_set_float(f, o, value);
		break;
	case PRIMITIVETYPE_DOUBLE:
		_field_set_double(f, o, value);
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    setFloat
 * Signature: (Ljava/lang/Object;F)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_VMField_setFloat(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o, float value)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_FLOAT:
		_field_set_float(f, o, value);
		break;
	case PRIMITIVETYPE_DOUBLE:
		_field_set_double(f, o, value);
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    setDouble
 * Signature: (Ljava/lang/Object;D)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_VMField_setDouble(JNIEnv *env, java_lang_reflect_VMField *_this, java_lang_Object *o, double value)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	/* check if the field can be accessed */

	if (!_field_access_check(_this, f, (java_handle_t *) o))
		return;

	/* check the field type and set the value */

	switch (f->parseddesc->decltype) {
	case PRIMITIVETYPE_DOUBLE:
		_field_set_double(f, o, value);
		break;
	default:
		exceptions_throw_illegalargumentexception();
	}

	return;
}


/*
 * Class:     java/lang/reflect/VMField
 * Method:    getSignature
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_reflect_VMField_getSignature(JNIEnv *env, java_lang_reflect_VMField* _this)
{
	classinfo     *c;
	fieldinfo     *f;
	java_handle_t *o;
	int32_t        slot;

	/* get the class and the field */

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	f = &c->fields[slot];

	if (f->signature == NULL)
		return NULL;

	o = javastring_new(f->signature);

	/* in error case o is NULL */

	return (java_lang_String *) o;
}


#if defined(ENABLE_ANNOTATIONS)
/*
 * Class:     java/lang/reflect/VMField
 * Method:    declaredAnnotations
 * Signature: ()Ljava/util/Map;
 */
JNIEXPORT struct java_util_Map* JNICALL Java_java_lang_reflect_VMField_declaredAnnotations(JNIEnv *env, java_lang_reflect_VMField *_this)
{
	java_util_Map           *declaredAnnotations = NULL; /* parsed annotations                                */
	java_handle_bytearray_t *annotations         = NULL; /* unparsed annotations                              */
	java_lang_Class         *declaringClass      = NULL; /* the constant pool of this class is used           */
	classinfo               *referer             = NULL; /* class, which calles the annotation parser         */
	                                                     /* (for the parameter 'referer' of vm_call_method()) */

	LLNI_field_get_ref(_this, declaredAnnotations, declaredAnnotations);

	/* are the annotations parsed yet? */
	if (declaredAnnotations == NULL) {
		LLNI_field_get_ref(_this, annotations, annotations);
		LLNI_field_get_ref(_this, clazz, declaringClass);
		LLNI_class_get(_this, referer);

		declaredAnnotations = reflect_get_declaredannotations(annotations, (classinfo*) declaringClass, referer);

		LLNI_field_set_ref(_this, declaredAnnotations, declaredAnnotations);
	}

	return declaredAnnotations;
}
#endif

} // extern "C"


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
