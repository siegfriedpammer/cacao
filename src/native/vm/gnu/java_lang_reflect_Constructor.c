/* src/native/vm/gnu/java_lang_reflect_Constructor.c

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
# include "native/include/java_util_Map.h"
# include "native/include/sun_reflect_ConstantPool.h"
#endif

#include "native/vm/reflect.h"

#include "vm/stringlocal.h"

#include "vmcore/utf8.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "getModifiersInternal",    "()I",                                                       (void *) (ptrint) &Java_java_lang_reflect_Constructor_getModifiersInternal    },
	{ "getParameterTypes",       "()[Ljava/lang/Class;",                                      (void *) (ptrint) &Java_java_lang_reflect_Constructor_getParameterTypes       },
	{ "getExceptionTypes",       "()[Ljava/lang/Class;",                                      (void *) (ptrint) &Java_java_lang_reflect_Constructor_getExceptionTypes       },
	{ "constructNative",         "([Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;", (void *) (ptrint) &Java_java_lang_reflect_Constructor_constructNative    },
	{ "getSignature",            "()Ljava/lang/String;",                                      (void *) (ptrint) &Java_java_lang_reflect_Constructor_getSignature            },
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
 * Method:    getModifiersInternal
 * Signature: ()I
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_Constructor_getModifiersInternal(JNIEnv *env, java_lang_reflect_Constructor *this)
{
	classinfo  *c;
	methodinfo *m;
	int32_t     slot;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot,  slot);

	m = &(c->methods[slot]);

	return m->flags;
}


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getParameterTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_reflect_Constructor_getParameterTypes(JNIEnv *env, java_lang_reflect_Constructor *this)
{
	classinfo  *c;
	methodinfo *m;
	int32_t     slot;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot,  slot);

	m = &(c->methods[slot]);

	return method_get_parametertypearray(m);
}


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getExceptionTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_reflect_Constructor_getExceptionTypes(JNIEnv *env, java_lang_reflect_Constructor *this)
{
	classinfo  *c;
	methodinfo *m;
	int32_t     slot;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot,  slot);

	m = &(c->methods[slot]);

	return method_get_exceptionarray(m);
}


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    constructNative
 * Signature: ([Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_Constructor_constructNative(JNIEnv *env, java_lang_reflect_Constructor *this, java_handle_objectarray_t *args, java_lang_Class *declaringClass, int32_t xslot)
{
	classinfo     *c;
	int32_t        slot;
	int32_t        override;
	methodinfo    *m;
	java_handle_t *o;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot,  slot);
	LLNI_field_get_val(this, flag,  override);

	m = &(c->methods[slot]);

	o = reflect_constructor_newinstance(m, args, override);

	return (java_lang_Object *) o;
}


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getSignature
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_reflect_Constructor_getSignature(JNIEnv *env, java_lang_reflect_Constructor *this)
{
	classinfo     *c;
	methodinfo    *m;
	java_handle_t *o;
	int32_t        slot;

	LLNI_field_get_cls(this, clazz, c);
	LLNI_field_get_val(this, slot,  slot);

	m = &(c->methods[slot]);

	if (m->signature == NULL)
		return NULL;

	o = javastring_new(m->signature);

	/* In error case o is NULL. */

	return (java_lang_String *) o;
}


#if defined(ENABLE_ANNOTATIONS)
/*
 * Class:     java/lang/reflect/Constructor
 * Method:    declaredAnnotations
 * Signature: ()Ljava/util/Map;
 *
 * Parses the annotations (if they aren't parsed yet) and stores them into
 * the declaredAnnotations map and return this map.
 */
JNIEXPORT struct java_util_Map* JNICALL Java_java_lang_reflect_Constructor_declaredAnnotations(JNIEnv *env, java_lang_reflect_Constructor *this)
{
	java_util_Map           *declaredAnnotations = NULL; /* parsed annotations                                */
	java_handle_bytearray_t *annotations         = NULL; /* unparsed annotations                              */
	java_lang_Class         *declaringClass      = NULL; /* the constant pool of this class is used           */
	classinfo               *referer             = NULL; /* class, which calles the annotation parser         */
	                                                     /* (for the parameter 'referer' of vm_call_method()) */

	LLNI_field_get_ref(this, declaredAnnotations, declaredAnnotations);

	/* are the annotations parsed yet? */
	if (declaredAnnotations == NULL) {
		LLNI_field_get_ref(this, annotations, annotations);
		LLNI_field_get_ref(this, clazz, declaringClass);
		LLNI_class_get(this, referer);

		declaredAnnotations = reflect_get_declaredannotatios(annotations, declaringClass, referer);

		LLNI_field_set_ref(this, declaredAnnotations, declaredAnnotations);
	}

	return declaredAnnotations;
}


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getParameterAnnotations
 * Signature: ()[[Ljava/lang/annotation/Annotation;
 *
 * Parses the parameter annotations and returns them in an 2 dimensional array.
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_reflect_Constructor_getParameterAnnotations(JNIEnv *env, java_lang_reflect_Constructor *this)
{
	java_handle_bytearray_t *parameterAnnotations = NULL; /* unparsed parameter annotations                    */
	int32_t                  slot                 = -1;   /* slot of the method                                */
	java_lang_Class         *declaringClass       = NULL; /* the constant pool of this class is used           */
	classinfo               *referer              = NULL; /* class, which calles the annotation parser         */
	                                                      /* (for the parameter 'referer' of vm_call_method()) */

	LLNI_field_get_ref(this, parameterAnnotations, parameterAnnotations);
	LLNI_field_get_val(this, slot, slot);
	LLNI_field_get_ref(this, clazz, declaringClass);
	LLNI_class_get(this, referer);

	return reflect_get_parameterannotations((java_handle_t*)parameterAnnotations, slot, declaringClass, referer);
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
