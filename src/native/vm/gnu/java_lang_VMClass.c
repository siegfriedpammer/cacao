/* src/native/vm/gnu/java_lang_VMClass.c

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

#include "vm/types.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_ClassLoader.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Throwable.h"
#include "native/include/java_lang_reflect_Constructor.h"
#include "native/include/java_lang_reflect_Method.h"

#include "native/include/java_lang_VMClass.h"

#include "vm/exceptions.h"
#include "vm/initialize.h"
#include "vm/stringlocal.h"

#include "vmcore/class.h"

#if defined(WITH_CLASSPATH_GNU) && defined(ENABLE_ANNOTATIONS)
#include "native/include/sun_reflect_ConstantPool.h"

#include "vm/vm.h"

#include "vmcore/annotation.h"
#endif


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "isInstance",              "(Ljava/lang/Class;Ljava/lang/Object;)Z",                        (void *) (ptrint) &Java_java_lang_VMClass_isInstance              },
	{ "isAssignableFrom",        "(Ljava/lang/Class;Ljava/lang/Class;)Z",                         (void *) (ptrint) &Java_java_lang_VMClass_isAssignableFrom        },
	{ "isInterface",             "(Ljava/lang/Class;)Z",                                          (void *) (ptrint) &Java_java_lang_VMClass_isInterface             },
	{ "isPrimitive",             "(Ljava/lang/Class;)Z",                                          (void *) (ptrint) &Java_java_lang_VMClass_isPrimitive             },
	{ "getName",                 "(Ljava/lang/Class;)Ljava/lang/String;",                         (void *) (ptrint) &Java_java_lang_VMClass_getName                 },
	{ "getSuperclass",           "(Ljava/lang/Class;)Ljava/lang/Class;",                          (void *) (ptrint) &Java_java_lang_VMClass_getSuperclass           },
	{ "getInterfaces",           "(Ljava/lang/Class;)[Ljava/lang/Class;",                         (void *) (ptrint) &Java_java_lang_VMClass_getInterfaces           },
	{ "getComponentType",        "(Ljava/lang/Class;)Ljava/lang/Class;",                          (void *) (ptrint) &Java_java_lang_VMClass_getComponentType        },
	{ "getModifiers",            "(Ljava/lang/Class;Z)I",                                         (void *) (ptrint) &Java_java_lang_VMClass_getModifiers            },
	{ "getDeclaringClass",       "(Ljava/lang/Class;)Ljava/lang/Class;",                          (void *) (ptrint) &Java_java_lang_VMClass_getDeclaringClass       },
	{ "getDeclaredClasses",      "(Ljava/lang/Class;Z)[Ljava/lang/Class;",                        (void *) (ptrint) &Java_java_lang_VMClass_getDeclaredClasses      },
	{ "getDeclaredFields",       "(Ljava/lang/Class;Z)[Ljava/lang/reflect/Field;",                (void *) (ptrint) &Java_java_lang_VMClass_getDeclaredFields       },
	{ "getDeclaredMethods",      "(Ljava/lang/Class;Z)[Ljava/lang/reflect/Method;",               (void *) (ptrint) &Java_java_lang_VMClass_getDeclaredMethods      },
	{ "getDeclaredConstructors", "(Ljava/lang/Class;Z)[Ljava/lang/reflect/Constructor;",          (void *) (ptrint) &Java_java_lang_VMClass_getDeclaredConstructors },
	{ "getClassLoader",          "(Ljava/lang/Class;)Ljava/lang/ClassLoader;",                    (void *) (ptrint) &Java_java_lang_VMClass_getClassLoader          },
	{ "forName",                 "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;", (void *) (ptrint) &Java_java_lang_VMClass_forName                 },
	{ "isArray",                 "(Ljava/lang/Class;)Z",                                          (void *) (ptrint) &Java_java_lang_VMClass_isArray                 },
	{ "throwException",          "(Ljava/lang/Throwable;)V",                                      (void *) (ptrint) &Java_java_lang_VMClass_throwException          },
#if defined(WITH_CLASSPATH_GNU) && defined(ENABLE_ANNOTATIONS)
	{ "getDeclaredAnnotations",  "(Ljava/lang/Class;)[Ljava/lang/annotation/Annotation;",         (void *) (ptrint) &Java_java_lang_VMClass_getDeclaredAnnotations  },
#endif
	{ "getEnclosingClass",       "(Ljava/lang/Class;)Ljava/lang/Class;",                          (void *) (ptrint) &Java_java_lang_VMClass_getEnclosingClass       },
	{ "getEnclosingConstructor", "(Ljava/lang/Class;)Ljava/lang/reflect/Constructor;",            (void *) (ptrint) &Java_java_lang_VMClass_getEnclosingConstructor },
	{ "getEnclosingMethod",      "(Ljava/lang/Class;)Ljava/lang/reflect/Method;",                 (void *) (ptrint) &Java_java_lang_VMClass_getEnclosingMethod      },
	{ "getClassSignature",       "(Ljava/lang/Class;)Ljava/lang/String;",                         (void *) (ptrint) &Java_java_lang_VMClass_getClassSignature       },
	{ "isAnonymousClass",        "(Ljava/lang/Class;)Z",                                          (void *) (ptrint) &Java_java_lang_VMClass_isAnonymousClass        },
	{ "isLocalClass",            "(Ljava/lang/Class;)Z",                                          (void *) (ptrint) &Java_java_lang_VMClass_isLocalClass            },
	{ "isMemberClass",           "(Ljava/lang/Class;)Z",                                          (void *) (ptrint) &Java_java_lang_VMClass_isMemberClass           },
};


/* _Jv_java_lang_VMClass_init **************************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_VMClass_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/VMClass");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/VMClass
 * Method:    isInstance
 * Signature: (Ljava/lang/Class;Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isInstance(JNIEnv *env, jclass clazz, java_lang_Class *klass, java_lang_Object *o)
{
	classinfo     *c;
	java_handle_t *h;

	c = LLNI_classinfo_unwrap(klass);
	h = (java_handle_t *) o;

	return class_is_instance(c, h);
}


/*
 * Class:     java/lang/VMClass
 * Method:    isAssignableFrom
 * Signature: (Ljava/lang/Class;Ljava/lang/Class;)Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_VMClass_isAssignableFrom(JNIEnv *env, jclass clazz, java_lang_Class *klass, java_lang_Class *c)
{
	classinfo *to;
	classinfo *from;

	to   = LLNI_classinfo_unwrap(klass);
	from = LLNI_classinfo_unwrap(c);

	if (from == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}

	return class_is_assignable_from(to, from);
}


/*
 * Class:     java/lang/VMClass
 * Method:    isInterface
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isInterface(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo *c;

	c = LLNI_classinfo_unwrap(klass);

	return class_is_interface(c);
}


/*
 * Class:     java/lang/VMClass
 * Method:    isPrimitive
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_VMClass_isPrimitive(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo *c;

	c = LLNI_classinfo_unwrap(klass);

	return class_is_primitive(c);
}


/*
 * Class:     java/lang/VMClass
 * Method:    getName
 * Signature: (Ljava/lang/Class;)Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_VMClass_getName(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo* c;

	c = LLNI_classinfo_unwrap(klass);

	return (java_lang_String*) class_get_classname(c);
}


/*
 * Class:     java/lang/VMClass
 * Method:    getSuperclass
 * Signature: (Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClass_getSuperclass(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo *c;
	classinfo *super;

	c = LLNI_classinfo_unwrap(klass);

	super = class_get_superclass(c);

	return LLNI_classinfo_wrap(super);
}


/*
 * Class:     java/lang/VMClass
 * Method:    getInterfaces
 * Signature: (Ljava/lang/Class;)[Ljava/lang/Class;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_VMClass_getInterfaces(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo                 *c;
	java_handle_objectarray_t *oa;

	c = LLNI_classinfo_unwrap(klass);

	oa = class_get_interfaces(c);

	return oa;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getComponentType
 * Signature: (Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClass_getComponentType(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo *c;
	classinfo *component;
	
	c = LLNI_classinfo_unwrap(klass);
	
	component = class_get_componenttype(c);

	return LLNI_classinfo_wrap(component);
}


/*
 * Class:     java/lang/VMClass
 * Method:    getModifiers
 * Signature: (Ljava/lang/Class;Z)I
 */
JNIEXPORT int32_t JNICALL Java_java_lang_VMClass_getModifiers(JNIEnv *env, jclass clazz, java_lang_Class *klass, int32_t ignoreInnerClassesAttrib)
{
	classinfo *c;
	int32_t    flags;

	c = LLNI_classinfo_unwrap(klass);

	flags = class_get_modifiers(c, ignoreInnerClassesAttrib);

	return flags;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaringClass
 * Signature: (Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClass_getDeclaringClass(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo *c;
	classinfo *dc;

	c = LLNI_classinfo_unwrap(klass);

	dc = class_get_declaringclass(c);

	return LLNI_classinfo_wrap(dc);
}


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredClasses
 * Signature: (Ljava/lang/Class;Z)[Ljava/lang/Class;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_VMClass_getDeclaredClasses(JNIEnv *env, jclass clazz, java_lang_Class *klass, int32_t publicOnly)
{
	classinfo                 *c;
	java_handle_objectarray_t *oa;

	c = LLNI_classinfo_unwrap(klass);

	oa = class_get_declaredclasses(c, publicOnly);

	return oa;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredFields
 * Signature: (Ljava/lang/Class;Z)[Ljava/lang/reflect/Field;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_VMClass_getDeclaredFields(JNIEnv *env, jclass clazz, java_lang_Class *klass, int32_t publicOnly)
{
	classinfo                 *c;
	java_handle_objectarray_t *oa;

	c = LLNI_classinfo_unwrap(klass);

	oa = class_get_declaredfields(c, publicOnly);

	return oa;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredMethods
 * Signature: (Ljava/lang/Class;Z)[Ljava/lang/reflect/Method;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_VMClass_getDeclaredMethods(JNIEnv *env, jclass clazz, java_lang_Class *klass, s4 publicOnly)
{
	classinfo                 *c;
	java_handle_objectarray_t *oa;

	c = LLNI_classinfo_unwrap(klass);

	oa = class_get_declaredmethods(c, publicOnly);

	return oa;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredConstructors
 * Signature: (Ljava/lang/Class;Z)[Ljava/lang/reflect/Constructor;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_VMClass_getDeclaredConstructors(JNIEnv *env, jclass clazz, java_lang_Class *klass, s4 publicOnly)
{
	classinfo                 *c;
	java_handle_objectarray_t *oa;

	c = LLNI_classinfo_unwrap(klass);

	oa = class_get_declaredconstructors(c, publicOnly);

	return oa;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getClassLoader
 * Signature: (Ljava/lang/Class;)Ljava/lang/ClassLoader;
 */
JNIEXPORT java_lang_ClassLoader* JNICALL Java_java_lang_VMClass_getClassLoader(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo     *c;
	classloader_t *cl;

	c  = LLNI_classinfo_unwrap(klass);
	cl = class_get_classloader(c);

	return (java_lang_ClassLoader *) cl;
}


/*
 * Class:     java/lang/VMClass
 * Method:    forName
 * Signature: (Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClass_forName(JNIEnv *env, jclass clazz, java_lang_String *name, s4 initialize, java_lang_ClassLoader *loader)
{
	classloader_t *cl;
	utf           *ufile;
	utf           *uname;
	classinfo     *c;
	u2            *pos;
	s4             i;

	cl = loader_hashtable_classloader_add((java_handle_t *) loader);

	/* illegal argument */

	if (name == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* create utf string in which '.' is replaced by '/' */

	ufile = javastring_toutf((java_handle_t *) name, true);
	uname = javastring_toutf((java_handle_t *) name, false);

	/* name must not contain '/' (mauve test) */

	for (i = 0, pos = LLNI_field_direct(name, value)->data + LLNI_field_direct(name, offset); i < LLNI_field_direct(name, count); i++, pos++) {
		if (*pos == '/') {
			exceptions_throw_classnotfoundexception(uname);
			return NULL;
		}
	}

	/* try to load, ... */

	c = load_class_from_classloader(ufile, cl);

	if (c == NULL)
	    return NULL;

	/* link, ... */

	if (!link_class(c))
		return NULL;
	
	/* ...and initialize it, if required */

	if (initialize)
		if (!initialize_class(c))
			return NULL;

	return LLNI_classinfo_wrap(c);
}


/*
 * Class:     java/lang/VMClass
 * Method:    isArray
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_VMClass_isArray(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo *c;

	c = LLNI_classinfo_unwrap(klass);

	return class_is_array(c);
}


/*
 * Class:     java/lang/VMClass
 * Method:    throwException
 * Signature: (Ljava/lang/Throwable;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_throwException(JNIEnv *env, jclass clazz, java_lang_Throwable *t)
{
	java_handle_t *o;

	o = (java_handle_t *) t;

	exceptions_set_exception(o);
}


#if defined(WITH_CLASSPATH_GNU) && defined(ENABLE_ANNOTATIONS)
/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredAnnotations
 * Signature: (Ljava/lang/Class;)[Ljava/lang/annotation/Annotation;
 */
JNIEXPORT java_handle_objectarray_t* JNICALL Java_java_lang_VMClass_getDeclaredAnnotations(JNIEnv *env, jclass clazz, java_lang_Class* klass)
{
	classinfo                *c               = NULL; /* classinfo for the java.lang.Class object 'klass'       */
	static methodinfo        *m_parseAnnotationsIntoArray   = NULL; /* parser method (cached, therefore static) */
	utf                      *utf_parseAnnotationsIntoArray = NULL; /* parser method name     */
	utf                      *utf_desc        = NULL;               /* parser method descriptor (signature)     */
	java_handle_bytearray_t  *annotations     = NULL;               /* unparsed annotations   */
	sun_reflect_ConstantPool *constantPool    = NULL;               /* constant pool of klass */
	java_lang_Object         *constantPoolOop = (java_lang_Object*)klass; /* constantPoolOop field of */
	                                                                      /* sun.reflect.ConstantPool */

	if (klass == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}
	
	c = LLNI_classinfo_unwrap(klass);

	/* get annotations: */
	annotations = class_get_annotations(c);

	constantPool = 
		(sun_reflect_ConstantPool*)native_new_and_init(
			class_sun_reflect_ConstantPool);
	
	if (constantPool == NULL) {
		/* out of memory */
		return NULL;
	}

	LLNI_field_set_ref(constantPool, constantPoolOop, constantPoolOop);

	/* only resolve the parser method the first time */
	if (m_parseAnnotationsIntoArray == NULL) {
		utf_parseAnnotationsIntoArray = utf_new_char("parseAnnotationsIntoArray");
		utf_desc = utf_new_char(
			"([BLsun/reflect/ConstantPool;Ljava/lang/Class;)"
			"[Ljava/lang/annotation/Annotation;");

		if (utf_parseAnnotationsIntoArray == NULL || utf_desc == NULL) {
			/* out of memory */
			return NULL;
		}

		m_parseAnnotationsIntoArray = class_resolveclassmethod(
			class_sun_reflect_annotation_AnnotationParser,
			utf_parseAnnotationsIntoArray,
			utf_desc,
			class_java_lang_Class,
			true);

		if (m_parseAnnotationsIntoArray == NULL) {
			/* method not found */
			return NULL;
		}
	}

	return (java_handle_objectarray_t*)vm_call_method(
		m_parseAnnotationsIntoArray, NULL,
		annotations, constantPool, klass);
}
#endif


/*
 * Class:     java/lang/VMClass
 * Method:    getEnclosingClass
 * Signature: (Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClass_getEnclosingClass(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo *c;
	classinfo *result;

	c = LLNI_classinfo_unwrap(klass);

	result = class_get_enclosingclass(c);

	return LLNI_classinfo_wrap(result);
}


/*
 * Class:     java/lang/VMClass
 * Method:    getEnclosingConstructor
 * Signature: (Ljava/lang/Class;)Ljava/lang/reflect/Constructor;
 */
JNIEXPORT java_lang_reflect_Constructor* JNICALL Java_java_lang_VMClass_getEnclosingConstructor(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo*     c;
	java_handle_t* h;

	c = LLNI_classinfo_unwrap(klass);
	h = class_get_enclosingconstructor(c);

	return (java_lang_reflect_Constructor*) h;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getEnclosingMethod
 * Signature: (Ljava/lang/Class;)Ljava/lang/reflect/Method;
 */
JNIEXPORT java_lang_reflect_Method* JNICALL Java_java_lang_VMClass_getEnclosingMethod(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo*     c;
	java_handle_t* h;

	c = LLNI_classinfo_unwrap(klass);
	h = class_get_enclosingmethod(c);

	return (java_lang_reflect_Method*) h;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getClassSignature
 * Signature: (Ljava/lang/Class;)Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_VMClass_getClassSignature(JNIEnv *env, jclass clazz, java_lang_Class* klass)
{
	classinfo     *c;
	utf           *u;
	java_handle_t *s;

	c = LLNI_classinfo_unwrap(klass);

	u = class_get_signature(c);

	if (u == NULL)
		return NULL;

	s = javastring_new(u);

	/* in error case s is NULL */

	return (java_lang_String *) s;
}


/*
 * Class:     java/lang/VMClass
 * Method:    isAnonymousClass
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_VMClass_isAnonymousClass(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	return class_is_anonymousclass(LLNI_classinfo_unwrap(klass));
}


/*
 * Class:     java/lang/VMClass
 * Method:    isLocalClass
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_VMClass_isLocalClass(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	return class_is_localclass(LLNI_classinfo_unwrap(klass));
}


/*
 * Class:     java/lang/VMClass
 * Method:    isMemberClass
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_VMClass_isMemberClass(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	return class_is_memberclass(LLNI_classinfo_unwrap(klass));
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
