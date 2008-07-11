/* src/native/vm/gnuclasspath/java_lang_reflect_VMMethod.cpp

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

#include <stdint.h>

#if defined(ENABLE_ANNOTATIONS)
#include "vm/vm.h"
#endif

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_String.h"

#if defined(ENABLE_ANNOTATIONS)
# include "native/include/java_util_Map.h"
# include "native/include/sun_reflect_ConstantPool.h"
#endif

#include "native/include/java_lang_reflect_Method.h"

// FIXME
extern "C" {
#include "native/include/java_lang_reflect_VMMethod.h"
}

#include "native/vm/reflect.h"

#include "vm/access.h"
#include "vm/global.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/initialize.h"
#include "vm/resolve.h"
#include "vm/stringlocal.h"

#include "vmcore/method.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ (char*) "getModifiersInternal",    (char*) "()I",                                                       (void*) (uintptr_t) &Java_java_lang_reflect_VMMethod_getModifiersInternal    },
	{ (char*) "getReturnType",           (char*) "()Ljava/lang/Class;",                                       (void*) (uintptr_t) &Java_java_lang_reflect_VMMethod_getReturnType           },
	{ (char*) "getParameterTypes",       (char*) "()[Ljava/lang/Class;",                                      (void*) (uintptr_t) &Java_java_lang_reflect_VMMethod_getParameterTypes       },
	{ (char*) "getExceptionTypes",       (char*) "()[Ljava/lang/Class;",                                      (void*) (uintptr_t) &Java_java_lang_reflect_VMMethod_getExceptionTypes       },
	{ (char*) "invoke",                  (char*) "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;", (void*) (uintptr_t) &Java_java_lang_reflect_VMMethod_invoke                  },
	{ (char*) "getSignature",            (char*) "()Ljava/lang/String;",                                      (void*) (uintptr_t) &Java_java_lang_reflect_VMMethod_getSignature            },
#if defined(ENABLE_ANNOTATIONS)
	{ (char*) "getDefaultValue",         (char*) "()Ljava/lang/Object;",                                      (void*) (uintptr_t) &Java_java_lang_reflect_VMMethod_getDefaultValue         },
	{ (char*) "declaredAnnotations",     (char*) "()Ljava/util/Map;",                                         (void*) (uintptr_t) &Java_java_lang_reflect_VMMethod_declaredAnnotations     },
	{ (char*) "getParameterAnnotations", (char*) "()[[Ljava/lang/annotation/Annotation;",                     (void*) (uintptr_t) &Java_java_lang_reflect_VMMethod_getParameterAnnotations },
#endif
};


/* _Jv_java_lang_reflect_VMMethod_init *****************************************

   Register native functions.

*******************************************************************************/

// FIXME
extern "C" {
void _Jv_java_lang_reflect_VMMethod_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/reflect/VMMethod");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}
}


// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     java/lang/reflect/VMMethod
 * Method:    getModifiersInternal
 * Signature: ()I
 */
JNIEXPORT int32_t JNICALL Java_java_lang_reflect_VMMethod_getModifiersInternal(JNIEnv *env, java_lang_reflect_VMMethod *_this)
{
	classinfo  *c;
	methodinfo *m;
	int32_t     slot;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	m = &(c->methods[slot]);

	return m->flags;
}


/*
 * Class:     java/lang/reflect/VMMethod
 * Method:    getReturnType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_reflect_VMMethod_getReturnType(JNIEnv *env, java_lang_reflect_VMMethod *_this)
{
	classinfo  *c;
	methodinfo *m;
	classinfo  *result;
	int32_t     slot;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	m = &(c->methods[slot]);

	result = method_returntype_get(m);

	return LLNI_classinfo_wrap(result);
}


/*
 * Class:     java/lang/reflect/VMMethod
 * Method:    getParameterTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_reflect_VMMethod_getParameterTypes(JNIEnv *env, java_lang_reflect_VMMethod *_this)
{
	classinfo  *c;
	methodinfo *m;
	int32_t     slot;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	m = &(c->methods[slot]);

	return method_get_parametertypearray(m);
}


/*
 * Class:     java/lang/reflect/VMMethod
 * Method:    getExceptionTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_reflect_VMMethod_getExceptionTypes(JNIEnv *env, java_lang_reflect_VMMethod *_this)
{
	classinfo  *c;
	methodinfo *m;
	int32_t     slot;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	m = &(c->methods[slot]);

	return method_get_exceptionarray(m);
}


/*
 * Class:     java/lang/reflect/VMMethod
 * Method:    invoke
 * Signature: (Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_VMMethod_invoke(JNIEnv *env, java_lang_reflect_VMMethod *_this, java_lang_Object *o, java_handle_objectarray_t *args)
{
	classinfo                *c;
	int32_t                   slot;
	java_lang_reflect_Method *rm;
	int32_t                   override;
	methodinfo               *m;
	java_handle_t            *ro;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot,  slot);

	LLNI_field_get_ref(_this, m,     rm);
	LLNI_field_get_val(rm,   flag,  override);

	m = &(c->methods[slot]);

	ro = reflect_method_invoke(m, (java_handle_t *) o, args, override);

	return (java_lang_Object *) ro;
}


/*
 * Class:     java/lang/reflect/VMMethod
 * Method:    getSignature
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_reflect_VMMethod_getSignature(JNIEnv *env, java_lang_reflect_VMMethod* _this)
{
	classinfo     *c;
	methodinfo    *m;
	java_handle_t *o;
	int32_t        slot;

	LLNI_field_get_cls(_this, clazz, c);
	LLNI_field_get_val(_this, slot , slot);
	m = &(c->methods[slot]);

	if (m->signature == NULL)
		return NULL;

	o = javastring_new(m->signature);

	/* in error case o is NULL */

	return (java_lang_String *) o;
}

#if defined(ENABLE_ANNOTATIONS)
/*
 * Class:     java/lang/reflect/VMMethod
 * Method:    getDefaultValue
 * Signature: ()Ljava/lang/Object;
 *
 * Parses the annotation default value and returnes it (boxed, if it's a primitive).
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_VMMethod_getDefaultValue(JNIEnv *env, struct java_lang_reflect_VMMethod* _this)
{
	java_handle_bytearray_t  *annotationDefault          = NULL; /* unparsed annotation default value                 */
	static methodinfo        *m_parseAnnotationDefault   = NULL; /* parser method (will be chached, therefore static) */
	utf                      *utf_parseAnnotationDefault = NULL; /* parser method name                                */
	utf                      *utf_desc        = NULL;            /* parser method descriptor (signature)              */
	sun_reflect_ConstantPool *constantPool    = NULL;            /* constant pool object to use                       */
	java_lang_Class          *constantPoolOop = NULL;            /* methods declaring class                           */
	classinfo                *referer         = NULL;            /* class, which calles the annotation parser         */
	                                                             /* (for the parameter 'referer' of vm_call_method()) */
	java_lang_reflect_Method* rm;
	java_handle_t*            h;

	if (_this == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	constantPool = 
		(sun_reflect_ConstantPool*)native_new_and_init(
			class_sun_reflect_ConstantPool);
	
	if (constantPool == NULL) {
		/* out of memory */
		return NULL;
	}

	LLNI_field_get_ref(_this, clazz, constantPoolOop);
	LLNI_field_set_ref(constantPool, constantPoolOop, (java_lang_Object*)constantPoolOop);

	/* only resolve the parser method the first time */
	if (m_parseAnnotationDefault == NULL) {
		utf_parseAnnotationDefault = utf_new_char("parseAnnotationDefault");
		utf_desc = utf_new_char(
			"(Ljava/lang/reflect/Method;[BLsun/reflect/ConstantPool;)"
			"Ljava/lang/Object;");

		if (utf_parseAnnotationDefault == NULL || utf_desc == NULL) {
			/* out of memory */
			return NULL;
		}

		LLNI_class_get(_this, referer);

		m_parseAnnotationDefault = class_resolveclassmethod(
			class_sun_reflect_annotation_AnnotationParser,
			utf_parseAnnotationDefault,
			utf_desc,
			referer,
			true);

		if (m_parseAnnotationDefault == NULL) {
			/* method not found */
			return NULL;
		}
	}

	LLNI_field_get_ref(_this, m,                 rm);
	LLNI_field_get_ref(_this, annotationDefault, annotationDefault);

	h = vm_call_method(m_parseAnnotationDefault, NULL, rm, annotationDefault, constantPool);

	return (java_lang_Object*) h;
}


/*
 * Class:     java/lang/reflect/VMMethod
 * Method:    declaredAnnotations
 * Signature: ()Ljava/util/Map;
 *
 * Parses the annotations (if they aren't parsed yet) and stores them into
 * the declaredAnnotations map and return this map.
 */
JNIEXPORT struct java_util_Map* JNICALL Java_java_lang_reflect_VMMethod_declaredAnnotations(JNIEnv *env, java_lang_reflect_VMMethod *_this)
{
	java_util_Map           *declaredAnnotations = NULL; /* parsed annotations                                */
	java_handle_bytearray_t *annotations         = NULL; /* unparsed annotations                              */
	java_lang_Class         *declaringClass      = NULL; /* the constant pool of _this class is used           */
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
 * Class:     java/lang/reflect/VMMethod
 * Method:    getParameterAnnotations
 * Signature: ()[[Ljava/lang/annotation/Annotation;
 *
 * Parses the parameter annotations and returns them in an 2 dimensional array.
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_reflect_VMMethod_getParameterAnnotations(JNIEnv *env, java_lang_reflect_VMMethod *_this)
{
	java_handle_bytearray_t *parameterAnnotations = NULL; /* unparsed parameter annotations                    */
	int32_t                  slot                 = -1;   /* slot of the method                                */
	classinfo               *c;
	methodinfo*              m;
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
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
