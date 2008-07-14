/* src/native/vm/gnuclasspath/java_lang_reflect_VMConstructor.cpp

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

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_String.h"

#if defined(ENABLE_ANNOTATIONS)
# include "native/include/java_util_Map.h"
# include "native/include/sun_reflect_ConstantPool.h"
#endif

#include "native/include/java_lang_reflect_Constructor.h"
// FIXME
extern "C" {
#include "native/include/java_lang_reflect_VMConstructor.h"
}

#include "native/vm/reflect.h"

#include "vm/stringlocal.h"

#include "vmcore/utf8.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ (char*) "getModifiersInternal",    (char*) "()I",                                     (void*) (uintptr_t) &Java_java_lang_reflect_VMConstructor_getModifiersInternal    },
	{ (char*) "getParameterTypes",       (char*) "()[Ljava/lang/Class;",                    (void*) (uintptr_t) &Java_java_lang_reflect_VMConstructor_getParameterTypes       },
	{ (char*) "getExceptionTypes",       (char*) "()[Ljava/lang/Class;",                    (void*) (uintptr_t) &Java_java_lang_reflect_VMConstructor_getExceptionTypes       },
	{ (char*) "construct",               (char*) "([Ljava/lang/Object;)Ljava/lang/Object;", (void*) (uintptr_t) &Java_java_lang_reflect_VMConstructor_construct               },
	{ (char*) "getSignature",            (char*) "()Ljava/lang/String;",                    (void*) (uintptr_t) &Java_java_lang_reflect_VMConstructor_getSignature            },
#if defined(ENABLE_ANNOTATIONS)
	{ (char*) "declaredAnnotations",     (char*) "()Ljava/util/Map;",                       (void*) (uintptr_t) &Java_java_lang_reflect_VMConstructor_declaredAnnotations     },
	{ (char*) "getParameterAnnotations", (char*) "()[[Ljava/lang/annotation/Annotation;",   (void*) (uintptr_t) &Java_java_lang_reflect_VMConstructor_getParameterAnnotations },
#endif
};


/* _Jv_java_lang_reflect_VMConstructor_init ************************************

   Register native functions.

*******************************************************************************/

extern "C" {
void _Jv_java_lang_reflect_VMConstructor_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/reflect/VMConstructor");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}
}


// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     java/lang/reflect/VMConstructor
 * Method:    getModifiersInternal
 * Signature: ()I
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_VMConstructor_getModifiersInternal(JNIEnv *env, java_lang_reflect_VMConstructor *_this)
{
	classinfo  *c;
	methodinfo *m;
	int32_t     slot;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot,  slot);

	m = &(c->methods[slot]);

	return m->flags;
}


/*
 * Class:     java/lang/reflect/VMConstructor
 * Method:    getParameterTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_reflect_VMConstructor_getParameterTypes(JNIEnv *env, java_lang_reflect_VMConstructor *_this)
{
	classinfo  *c;
	methodinfo *m;
	int32_t     slot;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot,  slot);

	m = &(c->methods[slot]);

	return method_get_parametertypearray(m);
}


/*
 * Class:     java/lang/reflect/VMConstructor
 * Method:    getExceptionTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_reflect_VMConstructor_getExceptionTypes(JNIEnv *env, java_lang_reflect_VMConstructor *_this)
{
	classinfo  *c;
	methodinfo *m;
	int32_t     slot;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot,  slot);

	m = &(c->methods[slot]);

	return method_get_exceptionarray(m);
}


/*
 * Class:     java/lang/reflect/VMConstructor
 * Method:    construct
 * Signature: ([Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_VMConstructor_construct(JNIEnv *env, java_lang_reflect_VMConstructor *_this, java_handle_objectarray_t *args)
{
	classinfo                     *c;
	int32_t                        slot;
	java_lang_reflect_Constructor *rc;
	int32_t                        override;
	methodinfo                    *m;
	java_handle_t                 *o;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot,  slot);

	LLNI_field_get_ref(_this, cons,  rc);
	LLNI_field_get_val(rc,   flag,  override);

	m = &(c->methods[slot]);

	o = reflect_constructor_newinstance(m, args, override);

	return (java_lang_Object *) o;
}


/*
 * Class:     java/lang/reflect/VMConstructor
 * Method:    getSignature
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_reflect_VMConstructor_getSignature(JNIEnv *env, java_lang_reflect_VMConstructor *_this)
{
	classinfo     *c;
	methodinfo    *m;
	java_handle_t *o;
	int32_t        slot;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot,  slot);

	m = &(c->methods[slot]);

	if (m->signature == NULL)
		return NULL;

	o = javastring_new(m->signature);

	/* In error case o is NULL. */

	return (java_lang_String *) o;
}


#if defined(ENABLE_ANNOTATIONS)
/*
 * Class:     java/lang/reflect/VMConstructor
 * Method:    declaredAnnotations
 * Signature: ()Ljava/util/Map;
 *
 * Parses the annotations (if they aren't parsed yet) and stores them into
 * the declaredAnnotations map and return this map.
 */
JNIEXPORT struct java_util_Map* JNICALL Java_java_lang_reflect_VMConstructor_declaredAnnotations(JNIEnv *env, java_lang_reflect_VMConstructor *_this)
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


/*
 * Class:     java/lang/reflect/VMConstructor
 * Method:    getParameterAnnotations
 * Signature: ()[[Ljava/lang/annotation/Annotation;
 *
 * Parses the parameter annotations and returns them in an 2 dimensional array.
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_reflect_VMConstructor_getParameterAnnotations(JNIEnv *env, java_lang_reflect_VMConstructor *_this)
{
	java_handle_bytearray_t *parameterAnnotations = NULL; /* unparsed parameter annotations                    */
	int32_t                  slot                 = -1;   /* slot of the method                                */
	classinfo               *c;
	methodinfo              *m;
	classinfo               *referer              = NULL; /* class, which calles the annotation parser         */
	                                                      /* (for the parameter 'referer' of vm_call_method()) */

	LLNI_field_get_ref(_this, parameterAnnotations, parameterAnnotations);
	LLNI_field_get_val(_this, slot, slot);
	LLNI_field_get_cls(_this, clazz, c);
	m = &(c->methods[slot]);

	LLNI_class_get(_this, referer);

	return reflect_get_parameterannotations((java_handle_t*)parameterAnnotations, m, referer);
}
#endif

} // extern "C"


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
