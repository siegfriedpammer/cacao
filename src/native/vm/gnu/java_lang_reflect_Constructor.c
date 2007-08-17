/* src/native/vm/gnu/java_lang_reflect_Constructor.c

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

   $Id: java_lang_reflect_Constructor.c 8321 2007-08-16 11:37:25Z michi $

*/


#include "config.h"

#include <assert.h>
#include <stdlib.h>

#if defined(ENABLE_ANNOTATIONS)
#include "vm/vm.h"
#include "vm/exceptions.h"
#endif

#include "vm/types.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_reflect_Constructor.h"

#if defined(ENABLE_ANNOTATIONS)
#include "native/include/sun_reflect_ConstantPool.h"
#include "native/vm/reflect.h"
#endif

#include "native/vm/java_lang_reflect_Constructor.h"

#include "vmcore/utf8.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "getModifiersInternal",    "()I",                                                       (void *) (ptrint) &_Jv_java_lang_reflect_Constructor_getModifiers        },
	{ "getParameterTypes",       "()[Ljava/lang/Class;",                                      (void *) (ptrint) &_Jv_java_lang_reflect_Constructor_getParameterTypes   },
	{ "getExceptionTypes",       "()[Ljava/lang/Class;",                                      (void *) (ptrint) &_Jv_java_lang_reflect_Constructor_getExceptionTypes   },
	{ "constructNative",         "([Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;", (void *) (ptrint) &Java_java_lang_reflect_Constructor_constructNative    },
	{ "getSignature",            "()Ljava/lang/String;",                                      (void *) (ptrint) &_Jv_java_lang_reflect_Constructor_getSignature        },
#if defined(ENABLE_ANNOTATIONS)
	{ "declaredAnnotations",     "()Ljava/util/Map;",                                         (void *) (ptrint) &Java_java_lang_reflect_Constructor_declaredAnnotations     },
	{ "getParameterAnnotations", "()[[Ljava/lang/annotation/Annotation;",                     (void *) (ptrint) &Java_java_lang_reflect_Constructor_getParameterAnnotations },
#endif
};


/* _Jv_java_lang_reflect_Constructor_init **************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_reflect_Constructor_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/reflect/Constructor");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    constructNative
 * Signature: ([Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_Constructor_constructNative(JNIEnv *env, java_lang_reflect_Constructor *this, java_handle_objectarray_t *args, java_lang_Class *declaringClass, s4 slot)
{
	/* just to be sure */

	assert(LLNI_field_direct(this, clazz) == declaringClass);
	assert(LLNI_field_direct(this, slot)  == slot);

	return _Jv_java_lang_reflect_Constructor_newInstance(env, this, args);
}


#if defined(ENABLE_ANNOTATIONS)
/*
 * Class:     java/lang/reflect/Constructor
 * Method:    declaredAnnotations
 * Signature: ()Ljava/util/Map;
 */
JNIEXPORT struct java_util_Map* JNICALL Java_java_lang_reflect_Constructor_declaredAnnotations(JNIEnv *env, struct java_lang_reflect_Constructor* this)
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


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getParameterAnnotations
 * Signature: ()[[Ljava/lang/annotation/Annotation;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_reflect_Constructor_getParameterAnnotations(JNIEnv *env, struct java_lang_reflect_Constructor* this)
{
	java_handle_t   *o                    = (java_handle_t*)this;
	java_bytearray  *parameterAnnotations = NULL;
	int32_t          slot                 = -1;
	java_lang_Class *declaringClass       = NULL;

	if (this == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	LLNI_field_get_ref(this, parameterAnnotations, parameterAnnotations);
	LLNI_field_get_val(this, slot, slot);
	LLNI_field_get_ref(this, clazz, declaringClass);

	return reflect_get_parameterannotations((java_handle_t*)parameterAnnotations, slot, declaringClass, o->vftbl->class);
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
