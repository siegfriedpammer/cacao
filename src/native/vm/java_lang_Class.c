/* src/native/vm/java_lang_Class.c - java/lang/Class

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

   $Id: java_lang_Class.c 8330 2007-08-16 18:15:51Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

/* keep this order of the native includes */

#include "native/include/java_lang_String.h"

#if defined(ENABLE_JAVASE)
# if defined(WITH_CLASSPATH_SUN)
#  include "native/include/java_nio_ByteBuffer.h"       /* required by j.l.CL */
# endif
# include "native/include/java_lang_ClassLoader.h"
#endif

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Class.h"

#if defined(ENABLE_JAVASE)
# include "native/include/java_lang_reflect_Constructor.h"
# include "native/include/java_lang_reflect_Field.h"
# include "native/include/java_lang_reflect_Method.h"
#endif

#include "native/vm/java_lang_Class.h"
#include "native/vm/java_lang_String.h"

#if defined(ENABLE_JAVASE)
# include "native/vm/reflect.h"
#endif

#include "toolbox/logging.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/primitive.h"
#include "vm/resolve.h"
#include "vm/stringlocal.h"

#include "vmcore/class.h"
#include "vmcore/loader.h"

#if defined(WITH_CLASSPATH_GNU) && defined(ENABLE_ANNOTATIONS)
#include "vm/vm.h"
#include "vmcore/annotation.h"
#include "native/include/sun_reflect_ConstantPool.h"
#endif

/*
 * Class:     java/lang/Class
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
java_lang_String *_Jv_java_lang_Class_getName(java_lang_Class *klass)
{
	classinfo        *c;
	java_lang_String *s;
	java_chararray_t *ca;
	u4                i;

	c = (classinfo *) klass;

	/* create a java string */

	s = (java_lang_String *) javastring_new(c->name);

	if (s == NULL)
		return NULL;

	/* return string where '/' is replaced by '.' */

	LLNI_field_get_ref(s, value, ca);

	for (i = 0; i < LLNI_array_size(ca); i++) {
		if (LLNI_array_direct(ca, i) == '/')
			LLNI_array_direct(ca, i) = '.';
	}

	return s;
}


/*
 * Class:     java/lang/Class
 * Method:    forName
 * Signature: (Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;
 */
#if defined(ENABLE_JAVASE)
java_lang_Class *_Jv_java_lang_Class_forName(java_lang_String *name, s4 initialize, java_lang_ClassLoader *loader)
#elif defined(ENABLE_JAVAME_CLDC1_1)
java_lang_Class *_Jv_java_lang_Class_forName(java_lang_String *name)
#endif
{
#if defined(ENABLE_JAVASE)
	classloader *cl;
#endif
	utf         *ufile;
	utf         *uname;
	classinfo   *c;
	u2          *pos;
	s4           i;

#if defined(ENABLE_JAVASE)
	cl = (classloader *) loader;
#endif

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

#if defined(ENABLE_JAVASE)
	c = load_class_from_classloader(ufile, cl);
#elif defined(ENABLE_JAVAME_CLDC1_1)
	c = load_class_bootstrap(ufile);
#endif

	if (c == NULL)
	    return NULL;

	/* link, ... */

	if (!link_class(c))
		return NULL;
	
	/* ...and initialize it, if required */

#if defined(ENABLE_JAVASE)
	if (initialize)
#endif
		if (!initialize_class(c))
			return NULL;

	return (java_lang_Class *) c;
}


/*
 * Class:     java/lang/Class
 * Method:    isInstance
 * Signature: (Ljava/lang/Object;)Z
 */
s4 _Jv_java_lang_Class_isInstance(java_lang_Class *klass, java_lang_Object *o)
{
	classinfo     *c;
	java_handle_t *ob;

	c = (classinfo *) klass;
	ob = (java_handle_t *) o;

	if (!(c->state & CLASS_LINKED))
		if (!link_class(c))
			return 0;

	return builtin_instanceof(ob, c);
}


/*
 * Class:     java/lang/Class
 * Method:    isAssignableFrom
 * Signature: (Ljava/lang/Class;)Z
 */
s4 _Jv_java_lang_Class_isAssignableFrom(java_lang_Class *klass, java_lang_Class *c)
{
	classinfo *kc;
	classinfo *cc;

	kc = (classinfo *) klass;
	cc = (classinfo *) c;

	if (cc == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}

	if (!(kc->state & CLASS_LINKED))
		if (!link_class(kc))
			return 0;

	if (!(cc->state & CLASS_LINKED))
		if (!link_class(cc))
			return 0;

	return class_isanysubclass(cc, kc);
}


/*
 * Class:     java/lang/Class
 * Method:    isInterface
 * Signature: ()Z
 */
JNIEXPORT int32_t JNICALL _Jv_java_lang_Class_isInterface(JNIEnv *env, java_lang_Class *this)
{
	classinfo *c;

	c = (classinfo *) this;

	return class_is_interface(c);
}


#if defined(ENABLE_JAVASE)

/*
 * Class:     java/lang/Class
 * Method:    isPrimitive
 * Signature: ()Z
 */
s4 _Jv_java_lang_Class_isPrimitive(java_lang_Class *klass)
{
	classinfo *c;

	c = (classinfo *) klass;

	return class_is_primitive(c);
}


/*
 * Class:     java/lang/Class
 * Method:    getSuperclass
 * Signature: ()Ljava/lang/Class;
 */
java_lang_Class *_Jv_java_lang_Class_getSuperclass(java_lang_Class *klass)
{
	classinfo *c;
	classinfo *super;

	c = (classinfo *) klass;

	super = class_get_superclass(c);

	return (java_lang_Class *) super;
}


/*
 * Class:     java/lang/Class
 * Method:    getInterfaces
 * Signature: ()[Ljava/lang/Class;
 */
java_handle_objectarray_t *_Jv_java_lang_Class_getInterfaces(java_lang_Class *klass)
{
	classinfo                 *c;
	java_handle_objectarray_t *oa;

	c = (classinfo *) klass;

	oa = class_get_interfaces(c);

	return oa;
}


/*
 * Class:     java/lang/Class
 * Method:    getModifiers
 * Signature: (Z)I
 */
s4 _Jv_java_lang_Class_getModifiers(java_lang_Class *klass, s4 ignoreInnerClassesAttrib)
{
	classinfo             *c;
	classref_or_classinfo  inner;
	classref_or_classinfo  outer;
	utf                   *innername;
	s4                     i;

	c = (classinfo *) klass;

	if (!ignoreInnerClassesAttrib && (c->innerclasscount != 0)) {
		/* search for passed class as inner class */

		for (i = 0; i < c->innerclasscount; i++) {
			inner = c->innerclass[i].inner_class;
			outer = c->innerclass[i].outer_class;

			/* Check if inner is a classref or a real class and get
               the name of the structure */

			innername = IS_CLASSREF(inner) ? inner.ref->name : inner.cls->name;

			/* innerclass is this class */

			if (innername == c->name) {
				/* has the class actually an outer class? */

				if (outer.any)
					/* return flags got from the outer class file */
					return c->innerclass[i].flags & ACC_CLASS_REFLECT_MASK;
				else
					return c->flags & ACC_CLASS_REFLECT_MASK;
			}
		}
	}

	/* passed class is no inner class or it was not requested */

	return c->flags & ACC_CLASS_REFLECT_MASK;
}


/*
 * Class:     java/lang/Class
 * Method:    getDeclaringClass
 * Signature: ()Ljava/lang/Class;
 */
java_lang_Class *_Jv_java_lang_Class_getDeclaringClass(java_lang_Class *klass)
{
	classinfo *c;

	c = (classinfo *) klass;

	return (java_lang_Class *) class_get_declaringclass(c);
}


/*
 * Class:     java/lang/Class
 * Method:    getDeclaredClasses
 * Signature: (Z)[Ljava/lang/Class;
 */
java_handle_objectarray_t *_Jv_java_lang_Class_getDeclaredClasses(java_lang_Class *klass, s4 publicOnly)
{
	classinfo                 *c;
	java_handle_objectarray_t *oa;

	c = (classinfo *) klass;

	oa = class_get_declaredclasses(c, publicOnly);

	return oa;
}


/*
 * Class:     java/lang/Class
 * Method:    getDeclaredFields
 * Signature: (Z)[Ljava/lang/reflect/Field;
 */
java_handle_objectarray_t *_Jv_java_lang_Class_getDeclaredFields(java_lang_Class *klass, s4 publicOnly)
{
	classinfo                 *c;
	java_handle_objectarray_t *oa;          /* result: array of field-objects */
	fieldinfo                 *f;
	java_lang_reflect_Field   *rf;
	s4 public_fields;                    /* number of elements in field-array */
	s4 pos;
	s4 i;

	c = (classinfo *) klass;

	/* determine number of fields */

	for (i = 0, public_fields = 0; i < c->fieldscount; i++)
		if ((c->fields[i].flags & ACC_PUBLIC) || (publicOnly == 0))
			public_fields++;

	/* create array of fields */

	oa = builtin_anewarray(public_fields, class_java_lang_reflect_Field);

	if (oa == NULL)
		return NULL;

	/* get the fields and store in the array */

	for (i = 0, pos = 0; i < c->fieldscount; i++) {
		f = &(c->fields[i]);

		if ((f->flags & ACC_PUBLIC) || (publicOnly == 0)) {
			/* create Field object */

			rf = reflect_field_new(f);

			/* store object into array */

			LLNI_objectarray_element_set(oa, pos, rf);
			pos++;
		}
	}

	return oa;
}


/*
 * Class:     java/lang/Class
 * Method:    getDeclaredMethods
 * Signature: (Z)[Ljava/lang/reflect/Method;
 */
java_handle_objectarray_t *_Jv_java_lang_Class_getDeclaredMethods(java_lang_Class *klass, s4 publicOnly)
{
	classinfo                 *c;
	java_lang_reflect_Method  *rm;
	java_handle_objectarray_t *oa;         /* result: array of Method-objects */
	methodinfo                *m;     /* the current method to be represented */
	s4 public_methods;               /* number of public methods of the class */
	s4 pos;
	s4 i;

	c = (classinfo *) klass;

	public_methods = 0;

	/* JOWENN: array classes do not declare methods according to mauve
	   test.  It should be considered, if we should return to my old
	   clone method overriding instead of declaring it as a member
	   function. */

	if (class_is_array(c))
		return builtin_anewarray(0, class_java_lang_reflect_Method);

	/* determine number of methods */

	for (i = 0; i < c->methodscount; i++) {
		m = &c->methods[i];

		if (((m->flags & ACC_PUBLIC) || (publicOnly == false)) &&
			((m->name != utf_init) && (m->name != utf_clinit)) &&
			!(m->flags & ACC_MIRANDA))
			public_methods++;
	}

	oa = builtin_anewarray(public_methods, class_java_lang_reflect_Method);

	if (oa == NULL)
		return NULL;

	for (i = 0, pos = 0; i < c->methodscount; i++) {
		m = &c->methods[i];

		if (((m->flags & ACC_PUBLIC) || (publicOnly == false)) && 
			((m->name != utf_init) && (m->name != utf_clinit)) &&
			!(m->flags & ACC_MIRANDA)) {
			/* create Method object */

			rm = reflect_method_new(m);

			/* store object into array */

			LLNI_objectarray_element_set(oa, pos, rm);
			pos++;
		}
	}

	return oa;
}


/*
 * Class:     java/lang/Class
 * Method:    getDeclaredConstructors
 * Signature: (Z)[Ljava/lang/reflect/Constructor;
 */
java_handle_objectarray_t *_Jv_java_lang_Class_getDeclaredConstructors(java_lang_Class *klass, s4 publicOnly)
{
	classinfo                     *c;
	methodinfo                    *m; /* the current method to be represented */
	java_handle_objectarray_t     *oa;     /* result: array of Method-objects */
	java_lang_reflect_Constructor *rc;
	s4 public_methods;               /* number of public methods of the class */
	s4 pos;
	s4 i;

	c = (classinfo *) klass;

	/* determine number of constructors */

	for (i = 0, public_methods = 0; i < c->methodscount; i++) {
		m = &c->methods[i];

		if (((m->flags & ACC_PUBLIC) || (publicOnly == 0)) &&
			(m->name == utf_init))
			public_methods++;
	}

	oa = builtin_anewarray(public_methods, class_java_lang_reflect_Constructor);

	if (oa == NULL)
		return NULL;

	for (i = 0, pos = 0; i < c->methodscount; i++) {
		m = &c->methods[i];

		if (((m->flags & ACC_PUBLIC) || (publicOnly == 0)) &&
			(m->name == utf_init)) {
			/* create Constructor object */

			rc = reflect_constructor_new(m);

			/* store object into array */

			LLNI_objectarray_element_set(oa, pos, rc);
			pos++;
		}
	}

	return oa;
}


/*
 * Class:     java/lang/Class
 * Method:    getClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
java_lang_ClassLoader *_Jv_java_lang_Class_getClassLoader(java_lang_Class *klass)
{
	classinfo *c;

	c = (classinfo *) klass;

	return (java_lang_ClassLoader *) c->classloader;
}

#endif /* defined(ENABLE_JAVASE) */


/*
 * Class:     java/lang/Class
 * Method:    isArray
 * Signature: ()Z
 */
JNIEXPORT int32_t JNICALL _Jv_java_lang_Class_isArray(JNIEnv *env, java_lang_Class *this)
{
	classinfo *c;

	c = (classinfo *) this;

	return class_is_array(c);
}


#if defined(ENABLE_JAVASE)

/*
 * Class:     java/lang/Class
 * Method:    throwException
 * Signature: (Ljava/lang/Throwable;)V
 */
void _Jv_java_lang_Class_throwException(java_lang_Throwable *t)
{
	java_handle_t *o;

	o = (java_handle_t *) t;

	exceptions_set_exception(o);
}


#if defined(WITH_CLASSPATH_GNU) && defined(ENABLE_ANNOTATIONS)
/*
 * Class:     java/lang/Class
 * Method:    getDeclaredAnnotations
 * Signature: (Ljava/lang/Class;)[Ljava/lang/annotation/Annotation;
 */
java_handle_objectarray_t *_Jv_java_lang_Class_getDeclaredAnnotations(java_lang_Class* klass)
{
	classinfo                *c               = (classinfo*)klass;
	static methodinfo        *m_parseAnnotationsIntoArray   = NULL;
	utf                      *utf_parseAnnotationsIntoArray = NULL;
	utf                      *utf_desc        = NULL;
	java_handle_bytearray_t  *annotations     = NULL;
	sun_reflect_ConstantPool *constantPool    = NULL;
	uint32_t                  size            = 0;
	java_lang_Object         *constantPoolOop = (java_lang_Object*)klass;

	if (c == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}
	
	/* Return null for arrays and primitives: */
	if (class_is_primitive(c) || class_is_array(c)) {
		return NULL;
	}

	if (c->annotations != NULL) {
		size        = c->annotations->size;
		annotations = builtin_newarray_byte(size);

		if(annotations != NULL) {
			MCOPY(annotations->data, c->annotations->data, uint8_t, size);
		}
	}

	constantPool = 
		(sun_reflect_ConstantPool*)native_new_and_init(
			class_sun_reflect_ConstantPool);
	
	if(constantPool == NULL) {
		/* out of memory */
		return NULL;
	}

	LLNI_field_set_ref(constantPool, constantPoolOop, constantPoolOop);

	/* only resolve the method the first time */
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
 * Class:     java/lang/Class
 * Method:    getEnclosingClass
 * Signature: (Ljava/lang/Class;)Ljava/lang/Class;
 */
java_lang_Class *_Jv_java_lang_Class_getEnclosingClass(java_lang_Class *klass)
{
	classinfo             *c;
	classref_or_classinfo  cr;
	classinfo             *ec;

	c = (classinfo *) klass;

	/* get enclosing class */

	cr = c->enclosingclass;

	if (cr.any == NULL)
		return NULL;

	/* resolve the class if necessary */

	if (IS_CLASSREF(cr)) {
		ec = resolve_classref_eager(cr.ref);

		if (ec == NULL)
			return NULL;
	}
	else
		ec = cr.cls;

	return (java_lang_Class *) ec;
}


/* _Jv_java_lang_Class_getEnclosingMethod_intern *******************************

   Helper function for _Jv_java_lang_Class_getEnclosingConstructor and
   _Jv_java_lang_Class_getEnclosingMethod.

*******************************************************************************/

static methodinfo *_Jv_java_lang_Class_getEnclosingMethod_intern(classinfo *c)
{
	classref_or_classinfo     cr;
	constant_nameandtype     *cn;
	classinfo                *ec;
	methodinfo               *m;

	/* get enclosing class and method */

	cr = c->enclosingclass;
	cn = c->enclosingmethod;

	/* check for enclosing class and method */

	if (cr.any == NULL)
		return NULL;

	if (cn == NULL)
		return NULL;

	/* resolve the class if necessary */

	if (IS_CLASSREF(cr)) {
		ec = resolve_classref_eager(cr.ref);

		if (ec == NULL)
			return NULL;
	}
	else
		ec = cr.cls;

	/* find method in enclosing class */

	m = class_findmethod(ec, cn->name, cn->descriptor);

	if (m == NULL) {
		exceptions_throw_internalerror("Enclosing method doesn't exist");
		return NULL;
	}

	return m;
}


/*
 * Class:     java/lang/Class
 * Method:    getEnclosingConstructor
 * Signature: (Ljava/lang/Class;)Ljava/lang/reflect/Constructor;
 */
java_lang_reflect_Constructor *_Jv_java_lang_Class_getEnclosingConstructor(java_lang_Class *klass)
{
	classinfo                     *c;
	methodinfo                    *m;
	java_lang_reflect_Constructor *rc;

	c = (classinfo *) klass;

	/* get enclosing method */

	m = _Jv_java_lang_Class_getEnclosingMethod_intern(c);

	if (m == NULL)
		return NULL;

	/* check for <init> */

	if (m->name != utf_init)
		return NULL;

	/* create Constructor object */

	rc = reflect_constructor_new(m);

	return rc;
}


/*
 * Class:     java/lang/Class
 * Method:    getEnclosingMethod
 * Signature: (Ljava/lang/Class;)Ljava/lang/reflect/Method;
 */
java_lang_reflect_Method *_Jv_java_lang_Class_getEnclosingMethod(java_lang_Class *klass)
{
	classinfo                *c;
	methodinfo               *m;
	java_lang_reflect_Method *rm;

	c = (classinfo *) klass;

	/* get enclosing method */

	m = _Jv_java_lang_Class_getEnclosingMethod_intern(c);

	if (m == NULL)
		return NULL;

	/* check for <init> */

	if (m->name == utf_init)
		return NULL;

	/* create java.lang.reflect.Method object */

	rm = reflect_method_new(m);

	return rm;
}


/*
 * Class:     java/lang/Class
 * Method:    getClassSignature
 * Signature: (Ljava/lang/Class;)Ljava/lang/String;
 */
java_lang_String *_Jv_java_lang_Class_getClassSignature(java_lang_Class* klass)
{
	classinfo     *c;
	java_handle_t *o;

	c = (classinfo *) klass;

	if (c->signature == NULL)
		return NULL;

	o = javastring_new(c->signature);

	/* in error case o is NULL */

	return (java_lang_String *) o;
}


#if 0
/*
 * Class:     java/lang/Class
 * Method:    isAnonymousClass
 * Signature: (Ljava/lang/Class;)Z
 */
s4 _Jv_java_lang_Class_isAnonymousClass(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    isLocalClass
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isLocalClass(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    isMemberClass
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isMemberClass(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);
#endif

#endif /* ENABLE_JAVASE */


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
