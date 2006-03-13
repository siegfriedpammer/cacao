/* src/native/vm/VMClass.c - java/lang/VMClass

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
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

   Contact: cacao@cacaojvm.org

   Authors: Roman Obermaiser

   Changes: Joseph Wenninger
            Christian Thalinger
			Edwin Steiner

   $Id: VMClass.c 4589 2006-03-13 08:02:58Z edwin $

*/


#include "config.h"

#include <assert.h>
#include <string.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_ClassLoader.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_VMClass.h"
#include "native/include/java_lang_reflect_Constructor.h"
#include "native/include/java_lang_reflect_Field.h"
#include "native/include/java_lang_reflect_Method.h"
#include "native/include/java_security_ProtectionDomain.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/loader.h"
#include "vm/resolve.h"
#include "vm/stringlocal.h"


/*
 * Class:     java/lang/VMClass
 * Method:    forName
 * Signature: (Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClass_forName(JNIEnv *env, jclass clazz, java_lang_String *name, s4 initialize, java_lang_ClassLoader *loader)
{
	classinfo *c;
	utf       *u;
	u2        *pos;
	s4         i;

	/* illegal argument */

	if (!name)
		return NULL;

	/* name must not contain '/' (mauve test) */

	for (i = 0, pos = name->value->data + name->offset; i < name->count; i++, pos++) {
		if (*pos == '/') {
			*exceptionptr =
				new_exception_javastring(string_java_lang_ClassNotFoundException, name);
			return NULL;
		}
	}

	/* create utf string in which '.' is replaced by '/' */

	u = javastring_toutf(name, true);

	/* try to load, ... */

	if (!(c = load_class_from_classloader(u, (java_objectheader *) loader))) {
		classinfo *xclass;

		xclass = (*exceptionptr)->vftbl->class;

		/* if the exception is a NoClassDefFoundError, we replace it with a
		   ClassNotFoundException, otherwise return the exception */

		if (xclass == class_java_lang_NoClassDefFoundError) {
			/* clear exceptionptr, because builtin_new checks for 
			   ExceptionInInitializerError */
			*exceptionptr = NULL;

			*exceptionptr =
				new_exception_javastring(string_java_lang_ClassNotFoundException, name);
		}

	    return NULL;
	}

	/* link, ... */

	if (!link_class(c))
		return NULL;
	
	/* ...and initialize it, if required */

	if (initialize)
		if (!initialize_class(c))
			return NULL;

	return (java_lang_Class *) c;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT java_lang_ClassLoader* JNICALL Java_java_lang_VMClass_getClassLoader(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo *c;

	c = (classinfo *) klass;

	return (java_lang_ClassLoader *) c->classloader;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getComponentType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClass_getComponentType(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo       *c;
	classinfo       *comp;
	arraydescriptor *desc;
	
	c = (classinfo *) klass;
	
	/* XXX maybe we could find a way to do this without linking. */
	/* This way should be safe and easy, however.                */

	if (!(c->state & CLASS_LINKED))
		if (!link_class(c))
			return NULL;

	desc = c->vftbl->arraydesc;
	
	if (desc == NULL)
		return NULL;
	
	if (desc->arraytype == ARRAYTYPE_OBJECT)
		comp = desc->componentvftbl->class;
	else
		comp = primitivetype_table[desc->arraytype].class_primitive;
		
	return (java_lang_Class *) comp;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredConstructors
 * Signature: (Z)[Ljava/lang/reflect/Constructor;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredConstructors(JNIEnv *env, jclass clazz, java_lang_Class *klass, s4 publicOnly)
{
	classinfo                     *c;
	methodinfo                    *m; /* the current method to be represented */
	java_objectarray              *oa;     /* result: array of Method-objects */
	java_objectheader             *o;
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

	if (!oa) 
		return NULL;

	for (i = 0, pos = 0; i < c->methodscount; i++) {
		m = &c->methods[i];

		if (((m->flags & ACC_PUBLIC) || (publicOnly == 0)) &&
			(m->name == utf_init)) {

			if (!(o = native_new_and_init(class_java_lang_reflect_Constructor)))
				return NULL;

			/* initialize instance fields */

			rc = (java_lang_reflect_Constructor *) o;

			rc->clazz = (java_lang_Class *) c;
			rc->slot  = i;

			/* store object into array */

			oa->data[pos++] = o;
		}
	}

	return oa;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredClasses
 * Signature: (Z)[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredClasses(JNIEnv *env, jclass clazz, java_lang_Class *klass, s4 publicOnly)
{
	classinfo             *c;
	classref_or_classinfo  outer;
	utf                   *outername;
	s4                     declaredclasscount;  /* number of declared classes */
	s4                     pos;                     /* current declared class */
	java_objectarray      *oa;                   /* array of declared classes */
	s4                     i;

	c = (classinfo *) klass;
	declaredclasscount = 0;

	if (!Java_java_lang_VMClass_isPrimitive(env, clazz, klass) &&
		(c->name->text[0] != '[')) {
		/* determine number of declared classes */

		for (i = 0; i < c->innerclasscount; i++) {
			outer = c->innerclass[i].outer_class;

			/* check if outer_class is a classref or a real class and
               get the class name from the structure */

			outername = IS_CLASSREF(outer) ? outer.ref->name : outer.cls->name;

			/* outer class is this class */

			if ((outername == c->name) &&
				((publicOnly == 0) || (c->innerclass[i].flags & ACC_PUBLIC)))
				declaredclasscount++;
		}
	}

	/* allocate Class[] and check for OOM */

	oa = builtin_anewarray(declaredclasscount, class_java_lang_Class);

	if (!oa)
		return NULL;

	for (i = 0, pos = 0; i < c->innerclasscount; i++) {
		outer = c->innerclass[i].outer_class;

		/* check if outer_class is a classref or a real class and
		   get the class name from the structure */

		outername = IS_CLASSREF(outer) ? outer.ref->name : outer.cls->name;

		/* outer class is this class */

		if ((outername == c->name) &&
			((publicOnly == 0) || (c->innerclass[i].flags & ACC_PUBLIC))) {
			classinfo *inner;

			if (!resolve_classref_or_classinfo(NULL,
											   c->innerclass[i].inner_class,
											   resolveEager, false, false,
											   &inner))
				return NULL;

			if (!(inner->state & CLASS_LINKED))
				if (!link_class(inner))
					return NULL;

			oa->data[pos++] = (java_objectheader *) inner;
		}
	}

	return oa;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaringClass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClass_getDeclaringClass(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo             *c;
	classref_or_classinfo  inner;
	utf                   *innername;
	classinfo             *outer;
	s4                     i;

	c = (classinfo *) klass;

	if (!Java_java_lang_VMClass_isPrimitive(env, clazz, klass) &&
		(c->name->text[0] != '[')) {

		if (c->innerclasscount == 0)  /* no innerclasses exist */
			return NULL;
    
		for (i = 0; i < c->innerclasscount; i++) {
			inner = c->innerclass[i].inner_class;

			/* check if inner_class is a classref or a real class and
               get the class name from the structure */

			innername = IS_CLASSREF(inner) ? inner.ref->name : inner.cls->name;

			/* innerclass is this class */

			if (innername == c->name) {
				/* maybe the outer class is not loaded yet */

				if (!resolve_classref_or_classinfo(NULL,
												   c->innerclass[i].outer_class,
												   resolveEager, false, false,
												   &outer))
					return NULL;

				if (!(outer->state & CLASS_LINKED))
					if (!link_class(outer))
						return NULL;

				return (java_lang_Class *) outer;
			}
		}
	}

	/* return NULL for arrayclasses and primitive classes */

	return NULL;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredFields
 * Signature: (Z)[Ljava/lang/reflect/Field;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredFields(JNIEnv *env, jclass clazz, java_lang_Class *klass, s4 publicOnly)
{
	classinfo               *c;
	java_objectarray        *oa;            /* result: array of field-objects */
	fieldinfo               *f;
	java_objectheader       *o;
	java_lang_reflect_Field *rf;
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

	if (!oa)
		return NULL;

	/* get the fields and store in the array */

	for (i = 0, pos = 0; i < c->fieldscount; i++) {
		f = &(c->fields[i]);

		if ((f->flags & ACC_PUBLIC) || (publicOnly == 0)) {
			/* create Field object */

			if (!(o = native_new_and_init(class_java_lang_reflect_Field)))
				return NULL;

			/* initialize instance fields */

			rf = (java_lang_reflect_Field *) o;

			rf->declaringClass = (java_lang_Class *) c;
			rf->name           = javastring_new(f->name);
			rf->slot           = i;

			/* store object into array */

			oa->data[pos++] = o;
		}
	}

	return oa;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getInterfaces
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getInterfaces(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo        *c;
	classinfo        *ic;
	java_objectarray *oa;
	u4                i;

	c = (classinfo *) klass;

	if (!(c->state & CLASS_LINKED))
		if (!link_class(c))
			return NULL;

	oa = builtin_anewarray(c->interfacescount, class_java_lang_Class);

	if (!oa)
		return NULL;

	for (i = 0; i < c->interfacescount; i++) {
		ic = c->interfaces[i].cls;

		oa->data[i] = (java_objectheader *) ic;
	}

	return oa;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredMethods
 * Signature: (Z)[Ljava/lang/reflect/Method;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredMethods(JNIEnv *env, jclass clazz, java_lang_Class *klass, s4 publicOnly)
{
	classinfo                *c;
	java_objectheader        *o;
	java_lang_reflect_Method *rm;
	java_objectarray         *oa;          /* result: array of Method-objects */
	methodinfo               *m;      /* the current method to be represented */
	s4 public_methods;               /* number of public methods of the class */
	s4 pos;
	s4 i;

	c = (classinfo *) klass;    
	public_methods = 0;

	/* JOWENN: array classes do not declare methods according to mauve
	   test.  It should be considered, if we should return to my old
	   clone method overriding instead of declaring it as a member
	   function. */

	if (Java_java_lang_VMClass_isArray(env, clazz, klass))
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

	if (!oa) 
		return NULL;

	for (i = 0, pos = 0; i < c->methodscount; i++) {
		m = &c->methods[i];

		if (((m->flags & ACC_PUBLIC) || (publicOnly == false)) && 
			((m->name != utf_init) && (m->name != utf_clinit)) &&
			!(m->flags & ACC_MIRANDA)) {

			if (!(o = native_new_and_init(class_java_lang_reflect_Method)))
				return NULL;

			/* initialize instance fields */

			rm = (java_lang_reflect_Method *) o;

			rm->declaringClass = (java_lang_Class *) m->class;
			rm->name           = javastring_new(m->name);
			rm->slot           = i;

			/* store object into array */

			oa->data[pos++] = o;
		}
	}

	return oa;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getModifiers
 * Signature: (Z)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_getModifiers(JNIEnv *env, jclass clazz, java_lang_Class *klass, s4 ignoreInnerClassesAttrib)
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
					return c->innerclass[i].flags;
				else
					return c->flags;
			}
		}
	}

	/* passed class is no inner class or it was not requested */

	return c->flags;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_VMClass_getName(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo        *c;
	java_lang_String *s;
	u4                i;

	c = (classinfo *) klass;
	s = (java_lang_String *) javastring_new(c->name);

	if (!s)
		return NULL;

	/* return string where '/' is replaced by '.' */

	for (i = 0; i < s->value->header.size; i++) {
		if (s->value->data[i] == '/')
			s->value->data[i] = '.';
	}

	return s;
}


/*
 * Class:     java/lang/Class
 * Method:    getSuperclass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClass_getSuperclass(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo *c;
	classinfo *sc;

	c = (classinfo *) klass;

	/* for java.lang.Object, primitive and Void classes we return NULL */
	if (!c->super.any)
		return NULL;

	/* for interfaces we also return NULL */
	if (c->flags & ACC_INTERFACE)
		return NULL;

	/* we may have to resolve the super class reference */
	if (!resolve_classref_or_classinfo(NULL, c->super, resolveEager, 
									   true, /* check access */
									   false,  /* don't link */
									   &sc))
	{
		return NULL;
	}

	/* store the resolution */
	c->super.cls = sc;

	return (java_lang_Class *) sc;
}


/*
 * Class:     java/lang/Class
 * Method:    isArray
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isArray(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo *c = (classinfo *) klass;

	if (!(c->state & CLASS_LINKED))
		if (!link_class(c))
			return 0;

	return (c->vftbl->arraydesc != NULL);
}


/*
 * Class:     java/lang/Class
 * Method:    isAssignableFrom
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isAssignableFrom(JNIEnv *env, jclass clazz, java_lang_Class *klass, java_lang_Class *c)
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

	/* XXX this may be wrong for array classes */

	return builtin_isanysubclass(cc, kc);
}


/*
 * Class:     java/lang/Class
 * Method:    isInstance
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isInstance(JNIEnv *env, jclass clazz, java_lang_Class *klass, java_lang_Object *o)
{
	classinfo         *c;
	java_objectheader *ob;

	c = (classinfo *) klass;
	ob = (java_objectheader *) o;

	if (!(c->state & CLASS_LINKED))
		if (!link_class(c))
			return 0;

	return builtin_instanceof(ob, c);
}


/*
 * Class:     java/lang/Class
 * Method:    isInterface
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isInterface(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo *c;

	c = (classinfo *) klass;

	if (c->flags & ACC_INTERFACE)
		return true;

	return false;
}


/*
 * Class:     java/lang/Class
 * Method:    isPrimitive
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isPrimitive(JNIEnv *env, jclass clazz, java_lang_Class *klass)
{
	classinfo *c;
	s4         i;

	c = (classinfo *) klass;

	/* search table of primitive classes */

	for (i = 0; i < PRIMITIVETYPE_COUNT; i++)
		if (primitivetype_table[i].class_primitive == c)
			return true;

	return false;
}


/*
 * Class:     java/lang/VMClass
 * Method:    throwException
 * Signature: (Ljava/lang/Throwable;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_throwException(JNIEnv *env, jclass clazz, java_lang_Throwable *t)
{
	*exceptionptr = (java_objectheader *) t;
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
