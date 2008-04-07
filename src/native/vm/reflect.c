/* src/native/vm/reflect.c - helper functions for java/lang/reflect

   Copyright (C) 2007, 2008
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

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

/* keep this order of the native includes */

#include "native/include/java_lang_String.h"

#if defined(WITH_CLASSPATH_SUN)
# include "native/include/java_nio_ByteBuffer.h"        /* required by j.l.CL */
#endif
#include "native/include/java_lang_ClassLoader.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_reflect_Constructor.h"
#include "native/include/java_lang_reflect_Field.h"
#include "native/include/java_lang_reflect_Method.h"

#if defined(ENABLE_ANNOTATIONS) && defined(WITH_CLASSPATH_GNU)
# include "vm/vm.h"
# include "native/include/sun_reflect_ConstantPool.h"
#endif

#include "native/vm/reflect.h"

#include "vm/access.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/stringlocal.h"

#include "vmcore/method.h"


/* reflect_constructor_new *****************************************************

   Allocates a new java.lang.reflect.Constructor object and
   initializes the fields with the method passed.

*******************************************************************************/

java_lang_reflect_Constructor *reflect_constructor_new(methodinfo *m)
{
	classinfo                     *c;
	java_handle_t                 *o;
	java_lang_reflect_Constructor *rc;
	int32_t                        slot;

	/* get declaring class */

	c = m->class;

	/* allocate a new object */

	o = builtin_new(class_java_lang_reflect_Constructor);

	if (o == NULL)
		return NULL;

	/* initialize instance fields */

	rc = (java_lang_reflect_Constructor *) o;

	/* calculate the slot */

	slot = m - c->methods;

#if defined(WITH_CLASSPATH_GNU)

	LLNI_field_set_cls(rc, clazz               , c);
	LLNI_field_set_val(rc, slot                , slot);
	LLNI_field_set_ref(rc, annotations         , method_get_annotations(m));
	LLNI_field_set_ref(rc, parameterAnnotations, method_get_parameterannotations(m));

#elif defined(WITH_CLASSPATH_SUN)

	LLNI_field_set_cls(rc, clazz               , c);
	LLNI_field_set_ref(rc, parameterTypes      , method_get_parametertypearray(m));
	LLNI_field_set_ref(rc, exceptionTypes      , method_get_exceptionarray(m));
	LLNI_field_set_val(rc, modifiers           , m->flags & ACC_CLASS_REFLECT_MASK);
	LLNI_field_set_val(rc, slot                , slot);
	LLNI_field_set_ref(rc, signature           , m->signature ? (java_lang_String *) javastring_new(m->signature) : NULL);
	LLNI_field_set_ref(rc, annotations         , method_get_annotations(m));
	LLNI_field_set_ref(rc, parameterAnnotations, method_get_parameterannotations(m));

#else
# error unknown classpath configuration
#endif

	return rc;
}


/* reflect_field_new ***********************************************************

   Allocates a new java.lang.reflect.Field object and initializes the
   fields with the field passed.

*******************************************************************************/

java_lang_reflect_Field *reflect_field_new(fieldinfo *f)
{
	classinfo               *c;
	java_handle_t           *o;
	java_lang_reflect_Field *rf;
	int32_t                  slot;

	/* get declaring class */

	c = f->class;

	/* allocate a new object */

	o = builtin_new(class_java_lang_reflect_Field);

	if (o == NULL)
		return NULL;

	/* initialize instance fields */

	rf = (java_lang_reflect_Field *) o;

	/* calculate the slot */

	slot = f - c->fields;

#if defined(WITH_CLASSPATH_GNU)

	LLNI_field_set_cls(rf, clazz         , c);

	/* The name needs to be interned */
	/* XXX implement me better! */

	LLNI_field_set_ref(rf, name          , (java_lang_String *) javastring_intern(javastring_new(f->name)));
	LLNI_field_set_val(rf, slot          , slot);
	LLNI_field_set_ref(rf, annotations   , field_get_annotations(f));

#elif defined(WITH_CLASSPATH_SUN)

	LLNI_field_set_cls(rf, clazz         , c);

	/* The name needs to be interned */
	/* XXX implement me better! */

	LLNI_field_set_ref(rf, name          , (java_lang_String *) javastring_intern(javastring_new(f->name)));
	LLNI_field_set_cls(rf, type          , (java_lang_Class *) field_get_type(f));
	LLNI_field_set_val(rf, modifiers     , f->flags);
	LLNI_field_set_val(rf, slot          , slot);
	LLNI_field_set_ref(rf, signature     , f->signature ? (java_lang_String *) javastring_new(f->signature) : NULL);
	LLNI_field_set_ref(rf, annotations   , field_get_annotations(f));

#else
# error unknown classpath configuration
#endif

	return rf;
}


/* reflect_method_new **********************************************************

   Allocates a new java.lang.reflect.Method object and initializes the
   fields with the method passed.

*******************************************************************************/

java_lang_reflect_Method *reflect_method_new(methodinfo *m)
{
	classinfo                *c;
	java_handle_t            *o;
	java_lang_reflect_Method *rm;
	int32_t                   slot;

	/* get declaring class */

	c = m->class;

	/* allocate a new object */

	o = builtin_new(class_java_lang_reflect_Method);

	if (o == NULL)
		return NULL;

	/* initialize instance fields */

	rm = (java_lang_reflect_Method *) o;

	/* calculate the slot */

	slot = m - c->methods;

#if defined(WITH_CLASSPATH_GNU)

	LLNI_field_set_cls(rm, clazz               , m->class);

	/* The name needs to be interned */
	/* XXX implement me better! */

	LLNI_field_set_ref(rm, name                , (java_lang_String *) javastring_intern(javastring_new(m->name)));
	LLNI_field_set_val(rm, slot                , slot);
	LLNI_field_set_ref(rm, annotations         , method_get_annotations(m));
	LLNI_field_set_ref(rm, parameterAnnotations, method_get_parameterannotations(m));
	LLNI_field_set_ref(rm, annotationDefault   , method_get_annotationdefault(m));

#elif defined(WITH_CLASSPATH_SUN)

	LLNI_field_set_cls(rm, clazz               , m->class);

	/* The name needs to be interned */
	/* XXX implement me better! */

	LLNI_field_set_ref(rm, name                , (java_lang_String *) javastring_intern(javastring_new(m->name)));
	LLNI_field_set_ref(rm, parameterTypes      , method_get_parametertypearray(m));
	LLNI_field_set_cls(rm, returnType          , (java_lang_Class *) method_returntype_get(m));
	LLNI_field_set_ref(rm, exceptionTypes      , method_get_exceptionarray(m));
	LLNI_field_set_val(rm, modifiers           , m->flags & ACC_CLASS_REFLECT_MASK);
	LLNI_field_set_val(rm, slot                , slot);
	LLNI_field_set_ref(rm, signature           , m->signature ? (java_lang_String *) javastring_new(m->signature) : NULL);
	LLNI_field_set_ref(rm, annotations         , method_get_annotations(m));
	LLNI_field_set_ref(rm, parameterAnnotations, method_get_parameterannotations(m));
	LLNI_field_set_ref(rm, annotationDefault   , method_get_annotationdefault(m));

#else
# error unknown classpath configuration
#endif

	return rm;
}


/* reflect_invoke **************************************************************

   Invoke a method on the given object with the given arguments.

   For instance methods OBJ must be != NULL and the method is looked up
   in the vftbl of the object.

   For static methods, OBJ is ignored.

*******************************************************************************/

static java_handle_t *reflect_invoke(methodinfo *m, java_handle_t *o, java_handle_objectarray_t *params)
{
	methodinfo    *resm;
	java_handle_t *ro;
	int            argcount;
	int            paramcount;

	/* Sanity check. */

	assert(m != NULL);

	argcount = m->parseddesc->paramcount;
	paramcount = argcount;

	/* If method is non-static, remove the `this' pointer. */

	if (!(m->flags & ACC_STATIC))
		paramcount--;

	/* For instance methods the object has to be an instance of the
	   class the method belongs to. For static methods the obj
	   parameter is ignored. */

	if (!(m->flags & ACC_STATIC) && o && (!builtin_instanceof(o, m->class))) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	/* check if we got the right number of arguments */

	if (((params == NULL) && (paramcount != 0)) ||
		(params && (LLNI_array_size(params) != paramcount))) 
	{
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	/* for instance methods we need an object */

	if (!(m->flags & ACC_STATIC) && (o == NULL)) {
		/* XXX not sure if that is the correct exception */
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* for static methods, zero object to make subsequent code simpler */
	if (m->flags & ACC_STATIC)
		o = NULL;

	if (o != NULL) {
		/* for instance methods we must do a vftbl lookup */
		resm = method_vftbl_lookup(LLNI_vftbl_direct(o), m);
	}
	else {
		/* for static methods, just for convenience */
		resm = m;
	}

	ro = vm_call_method_objectarray(resm, o, params);

	return ro;
}


/* reflect_constructor_newinstance ********************************************

   Creates an Java object instance of the given constructor.

   ARGUMENTS:
      m .......... methodinfo of the constructor
      args ....... constructor arguments
      override ... override security checks

   RETURN:
      constructed Java object

*******************************************************************************/

java_handle_t *reflect_constructor_newinstance(methodinfo *m, java_handle_objectarray_t *args, bool override)
{
	java_handle_t *o;

	/* Should we bypass security the checks (AccessibleObject)? */

	if (override == false) {
		/* This method is always called like this:
		       [0] java.lang.reflect.Constructor.constructNative (Native Method)
		       [1] java.lang.reflect.Constructor.newInstance
		       [2] <caller>
		*/

		if (!access_check_method(m, 2))
			return NULL;
	}

	/* Create a Java object. */

	o = builtin_new(m->class);

	if (o == NULL)
		return NULL;
        
	/* Call initializer. */

	(void) reflect_invoke(m, o, args);

	return o;
}


/* reflect_method_invoke *******************************************************

   Invokes the given method.

   ARGUMENTS:
      m .......... methodinfo
      args ....... method arguments
      override ... override security checks

   RETURN:
      return value of the method

*******************************************************************************/

java_handle_t *reflect_method_invoke(methodinfo *m, java_handle_t *o, java_handle_objectarray_t *args, bool override)
{
	java_handle_t *ro;

	/* Should we bypass security the checks (AccessibleObject)? */

	if (override == false) {
#if defined(WITH_CLASSPATH_GNU)
		/* This method is always called like this:
		       [0] java.lang.reflect.Method.invokeNative (Native Method)
		       [1] java.lang.reflect.Method.invoke (Method.java:329)
		       [2] <caller>
		*/

		if (!access_check_method(m, 2))
			return NULL;
#elif defined(WITH_CLASSPATH_SUN)
		/* We only pass 1 here as stacktrace_get_caller_class, which
		   is called from access_check_method, skips
		   java.lang.reflect.Method.invoke(). */

		if (!access_check_method(m, 1))
			return NULL;
#endif
	}

	/* Check if method class is initialized. */

	if (!(m->class->state & CLASS_INITIALIZED))
		if (!initialize_class(m->class))
			return NULL;

	/* Call the Java method. */

	ro = reflect_invoke(m, o, args);

	return ro;
}


#if defined(WITH_CLASSPATH_GNU) && defined(ENABLE_ANNOTATIONS)
/* reflect_get_declaredannotatios *********************************************

   Calls the annotation parser with the unparsed annotations and returnes
   the parsed annotations as a map.
   
   IN:
       annotations........the unparsed annotations
       declaringClass.....the class in which the annotated element is declared
       referer............the calling class (for the 'referer' parameter of
                          vm_call_method())

   RETURN VALUE:
       The parsed annotations as a
	   java.util.Map<Class<? extends Annotation>, Annotation>.

*******************************************************************************/

struct java_util_Map* reflect_get_declaredannotatios(
	java_handle_bytearray_t *annotations,
	java_lang_Class         *declaringClass,
	classinfo               *referer)
{
	static methodinfo        *m_parseAnnotations   = NULL;
	                            /* parser method (chached, therefore static)  */
	utf                      *utf_parseAnnotations = NULL;
	                            /* parser method name                         */
	utf                      *utf_desc             = NULL;
	                            /* parser method descriptor (signature)       */
	sun_reflect_ConstantPool *constantPool         = NULL;
	                            /* constant pool of the declaring class       */
	java_lang_Object         *constantPoolOop      = (java_lang_Object*)declaringClass;
	                            /* constantPoolOop field of the constant pool */
	                            /* object (sun.reflect.ConstantPool)          */

	constantPool = 
		(sun_reflect_ConstantPool*)native_new_and_init(
			class_sun_reflect_ConstantPool);
		
	if(constantPool == NULL) {
		/* out of memory */
		return NULL;
	}
		
	LLNI_field_set_ref(constantPool, constantPoolOop, constantPoolOop);
		
	/* only resolve the parser method the first time */
	if (m_parseAnnotations == NULL) {
		utf_parseAnnotations = utf_new_char("parseAnnotations");
		utf_desc = utf_new_char(
			"([BLsun/reflect/ConstantPool;Ljava/lang/Class;)"
			"Ljava/util/Map;");

		if (utf_parseAnnotations == NULL || utf_desc == NULL) {
			/* out of memory */
			return NULL;
		}
		
		m_parseAnnotations = class_resolveclassmethod(
			class_sun_reflect_annotation_AnnotationParser,
			utf_parseAnnotations,
			utf_desc,
			referer,
			true);
	
		if (m_parseAnnotations == NULL) {
			/* method not found */
			return NULL;
		}
	}
	
	return (struct java_util_Map*)vm_call_method(
			m_parseAnnotations, NULL, annotations,
			constantPool, declaringClass);
}


/* reflect_get_parameterannotations *******************************************

   Calls the annotation parser with the unparsed parameter annotations of
   a method and returnes the parsed parameter annotations in a 2 dimensional
   array.
   
   IN:
       parameterAnnotations....the unparsed parameter annotations
	   slot....................the slot of the method
       declaringClass..........the class in which the annotated element is
	                           declared
       referer.................the calling class (for the 'referer' parameter
                               of vm_call_method())

   RETURN VALUE:
       The parsed parameter annotations in a 2 dimensional array.

*******************************************************************************/

java_handle_objectarray_t* reflect_get_parameterannotations(
	java_handle_t     *parameterAnnotations,
	int32_t            slot,
	java_lang_Class   *declaringClass,
	classinfo         *referer)
{
	/* This method in java would be basically the following.
	 * We don't do it in java because we don't want to make a
	 * public method with wich you can get a ConstantPool, because
	 * with that you could read any kind of constants (even private
	 * ones).
	 *
	 * ConstantPool constPool = new ConstantPool();
	 * constPool.constantPoolOop = method.getDeclaringClass();
	 * return sun.reflect.AnnotationParser.parseParameterAnnotations(
	 * 	  parameterAnnotations,
	 * 	  constPool,
	 * 	  method.getDeclaringClass(),
	 * 	  method.getParameterTypes().length);
	 */
	static methodinfo        *m_parseParameterAnnotations   = NULL;
	                     /* parser method (cached, therefore static)          */
	utf                      *utf_parseParameterAnnotations = NULL;
	                     /* parser method name                                */
	utf                      *utf_desc        = NULL;
	                     /* parser method descriptor (signature)              */
	sun_reflect_ConstantPool *constantPool    = NULL;
	                     /* constant pool of the declaring class              */
	java_lang_Object         *constantPoolOop = (java_lang_Object*)declaringClass;
	                     /* constantPoolOop field of the constant pool object */
	classinfo                *c               = NULL;
	                     /* classinfo of the decaring class                   */
	methodinfo               *m               = NULL;
	                     /* method info of the annotated method               */
	int32_t                   numParameters   = -1;
	                     /* parameter count of the annotated method           */

	/* get parameter count */

	c = LLNI_classinfo_unwrap(declaringClass);
	m = &(c->methods[slot]);

	numParameters = method_get_parametercount(m);

	if (numParameters < 0) {
		/* error parsing descriptor */
		return NULL;
	}

	/* get ConstantPool */

	constantPool = 
		(sun_reflect_ConstantPool*)native_new_and_init(
			class_sun_reflect_ConstantPool);
	
	if(constantPool == NULL) {
		/* out of memory */
		return NULL;
	}

	LLNI_field_set_ref(constantPool, constantPoolOop, constantPoolOop);

	/* only resolve the parser method the first time */
	if (m_parseParameterAnnotations == NULL) {
		utf_parseParameterAnnotations = utf_new_char("parseParameterAnnotations");
		utf_desc = utf_new_char(
			"([BLsun/reflect/ConstantPool;Ljava/lang/Class;I)"
			"[[Ljava/lang/annotation/Annotation;");

		if (utf_parseParameterAnnotations == NULL || utf_desc == NULL) {
			/* out of memory */
			return NULL;
		}

		/* get parser method */

		m_parseParameterAnnotations = class_resolveclassmethod(
			class_sun_reflect_annotation_AnnotationParser,
			utf_parseParameterAnnotations,
			utf_desc,
			referer,
			true);

		if (m_parseParameterAnnotations == NULL)
		{
			/* method not found */
			return NULL;
		}
	}

	return (java_handle_objectarray_t*)vm_call_method(
		m_parseParameterAnnotations, NULL, parameterAnnotations,
		constantPool, declaringClass, numParameters);
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
 * vim:noexpandtab:sw=4:ts=4:
 */
