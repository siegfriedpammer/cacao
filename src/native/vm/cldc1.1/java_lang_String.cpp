/* src/native/vm/cldc1.1/java_lang_String.cpp

   Copyright (C) 2006, 2007, 2008
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Object.h"

// FIXME
extern "C" {
#include "native/include/java_lang_String.h"
}

#include "vm/string.hpp"


/* native methods implemented by this file ************************************/
 
static JNINativeMethod methods[] = {
	{ (char*) "hashCode",    (char*) "()I",                    (void*) (uintptr_t) &Java_java_lang_String_hashCode        },
	{ (char*) "indexOf",     (char*) "(I)I",                   (void*) (uintptr_t) &Java_java_lang_String_indexOf__I      },
	{ (char*) "indexOf",     (char*) "(II)I",                  (void*) (uintptr_t) &Java_java_lang_String_indexOf__II     },
	{ (char*) "lastIndexOf", (char*) "(I)I",                   (void*) (uintptr_t) &Java_java_lang_String_lastIndexOf__I  },
	{ (char*) "lastIndexOf", (char*) "(II)I",                  (void*) (uintptr_t) &Java_java_lang_String_lastIndexOf__II },
#if 0
	{ (char*) "equals",      (char*) "(Ljava/lang/Object;)Z;", (void*) (uintptr_t) &Java_java_lang_String_equals          },
#endif
	{ (char*) "intern",      (char*) "()Ljava/lang/String;",   (void*) (uintptr_t) &Java_java_lang_String_intern          },
};


/* _Jv_java_lang_String_init ***************************************************
 
   Register native functions.
 
*******************************************************************************/
 
// FIXME
extern "C" {
void _Jv_java_lang_String_init(void)
{
	utf *u;
 
	u = utf_new_char("java/lang/String");
 
	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}
}


// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     java/lang/String
 * Method:    hashCode
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_String_hashCode(JNIEnv *env, java_lang_String *_this)
{
	java_handle_chararray_t *value;
	int32_t              	offset;
	int32_t              	count;
	s4              		hash;
	s4              		i;

	/* get values from Java object */
	
	LLNI_field_get_val(_this, offset, offset);
	LLNI_field_get_val(_this, count, count);
	LLNI_field_get_ref(_this, value, value);

	hash = 0;

	for (i = 0; i < count; i++) {
		hash = (31 * hash) + LLNI_array_direct(value, offset + i);
	}

	return hash;
}


/*
 * Class:     java/lang/String
 * Method:    indexOf
 * Signature: (I)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_String_indexOf__I(JNIEnv *env, java_lang_String *_this, s4 ch)
{
	java_handle_chararray_t *value;
	int32_t              	offset;
	int32_t              	count;
	s4              		i;

	/* get values from Java object */

	LLNI_field_get_val(_this, offset, offset);
	LLNI_field_get_val(_this, count, count);
	LLNI_field_get_ref(_this, value, value);

	for (i = 0; i < count; i++) {
		if (LLNI_array_direct(value, offset + i) == ch) {
			return i;
		}
	}

	return -1;
}


/*
 * Class:     java/lang/String
 * Method:    indexOf
 * Signature: (II)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_String_indexOf__II(JNIEnv *env, java_lang_String *_this, s4 ch, s4 fromIndex)
{
	java_handle_chararray_t *value;
	int32_t              	offset;
	int32_t              	count;
	s4              		i;

	/* get values from Java object */

	LLNI_field_get_val(_this, offset, offset);
	LLNI_field_get_val(_this, count, count);
	LLNI_field_get_ref(_this, value, value);

	if (fromIndex < 0) {
		fromIndex = 0;
	}
	else if (fromIndex >= count) {
		/* Note: fromIndex might be near -1>>>1. */
		return -1;
	}

	for (i = fromIndex ; i < count ; i++) {
		if (LLNI_array_direct(value, offset + i) == ch) {
			return i;
		}
	}

	return -1;
}


/*
 * Class:     java/lang/String
 * Method:    lastIndexOf
 * Signature: (I)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_String_lastIndexOf__I(JNIEnv *env, java_lang_String *_this, s4 ch)
{
	int32_t	count;
	
	LLNI_field_get_val(_this, count, count);
	
	return Java_java_lang_String_lastIndexOf__II(env, _this, ch, count - 1);
}


/*
 * Class:     java/lang/String
 * Method:    lastIndexOf
 * Signature: (II)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_String_lastIndexOf__II(JNIEnv *env, java_lang_String *_this, s4 ch, s4 fromIndex)
{
	java_handle_chararray_t *value;
	int32_t              	offset;
	int32_t              	count;
	s4              		start;
	s4              		i;

	/* get values from Java object */

	LLNI_field_get_val(_this, offset, offset);
	LLNI_field_get_val(_this, count, count);
	LLNI_field_get_ref(_this, value, value);

	start = ((fromIndex >= count) ? count - 1 : fromIndex);

	for (i = start; i >= 0; i--) {
		if (LLNI_array_direct(value, offset + i) == ch) {
			return i;
		}
	}

	return -1;
}


#if 0
/*
 * Class:     java/lang/String
 * Method:    equals
 * Signature: (Ljava/lang/Object;)Z;
 */
JNIEXPORT s4 JNICALL Java_java_lang_String_equals(JNIEnv *env, java_lang_String* _this, java_lang_Object *o)
{
	java_lang_String* 		s;
	java_handle_chararray_t *value;
	int32_t              	offset;
	int32_t              	count;
	java_handle_chararray_t *dvalue;
	int32_t              	doffset;
	int32_t              	dcount;
	classinfo     			*c;
	
	LLNI_field_get_val(_this, offset, offset);
	LLNI_field_get_val(_this, count, count);
	LLNI_field_get_ref(_this, value, value);
	LLNI_class_get(o, c);

	/* TODO: is this the correct implementation for short-circuiting on object identity? */
	if ((java_lang_Object*)_this == o)
		return 1;
	
	if (c != class_java_lang_String) 
		return 0;

	s = (java_lang_String *) o;
	LLNI_field_get_val(_this, offset, doffset);
	LLNI_field_get_val(_this, count, dcount);
	LLNI_field_get_ref(_this, value, dvalue);

	if (count != dcount)
		return 0;

	return ( 0 == memcmp((void*)(LLNI_array_direct(value, offset)),
						 (void*)(LLNI_array_direct(dvalue, doffset),
						 count) );

}
#endif


/*
 * Class:     java/lang/String
 * Method:    intern
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_String_intern(JNIEnv *env, java_lang_String *_this)
{
	if (_this == NULL)
		return NULL;

	return (java_lang_String *) javastring_intern((java_handle_t *) _this);
}

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
 */
