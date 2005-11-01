/* src/native/vm/VMClass.c - java/lang/VMClass

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Roman Obermaiser

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: VMClass.c 3526 2005-11-01 15:26:34Z twisti $

*/


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
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/resolve.h"


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

	if (!use_class_as_object(c))
		return NULL;

	return (java_lang_Class *) c;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT java_lang_ClassLoader* JNICALL Java_java_lang_VMClass_getClassLoader(JNIEnv *env, jclass clazz, java_lang_Class *that)
{  
	return (java_lang_ClassLoader *) ((classinfo *) that)->classloader;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getComponentType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClass_getComponentType(JNIEnv *env, jclass clazz, java_lang_Class *that)
{
    classinfo *thisclass = (classinfo *) that;
    classinfo *c = NULL;
    arraydescriptor *desc;
    
    if ((desc = thisclass->vftbl->arraydesc) != NULL) {
        if (desc->arraytype == ARRAYTYPE_OBJECT)
            c = desc->componentvftbl->class;
        else
            c = primitivetype_table[desc->arraytype].class_primitive;
        
        /* set vftbl */
		use_class_as_object(c);
    }
    
    return (java_lang_Class *) c;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredConstructors
 * Signature: (Z)[Ljava/lang/reflect/Constructor;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredConstructors(JNIEnv *env, jclass clazz, java_lang_Class *that, s4 public_only)
{
  
    classinfo         *c;
    java_objectheader *o;
    java_objectarray  *array_constructor;  /* result: array of Method-objects */
#if 0
    java_objectarray *exceptiontypes;  /* the exceptions thrown by the method */
#endif
    methodinfo *m;			/* the current method to be represented */    
    int public_methods = 0;		/* number of public methods of the class */
    int pos = 0;
    int i;

    c = (classinfo *) that;

    /* determine number of constructors */

    for (i = 0; i < c->methodscount; i++) 
		if (((c->methods[i].flags & ACC_PUBLIC) || !public_only) && 
			(c->methods[i].name == utf_init))
			public_methods++;

    array_constructor =
		builtin_anewarray(public_methods, class_java_lang_reflect_Constructor);

    if (!array_constructor) 
		return NULL;

    for (i = 0; i < c->methodscount; i++) {
		if ((c->methods[i].flags & ACC_PUBLIC) || !public_only){
			m = &c->methods[i];	    
			if (m->name != utf_init)
				continue;

			o = native_new_and_init(class_java_lang_reflect_Constructor);
			array_constructor->data[pos++] = o;

#if 0
			/* array of exceptions declared to be thrown, information not available !! */
			exceptiontypes = builtin_anewarray(0, class_java_lang_Class);

			/*	    class_showconstantpool(class_constructor);*/
			/* initialize instance fields */
			/*	    ((java_lang_reflect_Constructor*)o)->flag=(m->flags & (ACC_PRIVATE | ACC_PUBLIC | ACC_PROTECTED));*/
#endif

			setfield_critical(class_java_lang_reflect_Constructor,
							  o,
							  "clazz",
							  "Ljava/lang/Class;",
							  jobject,
							  (jobject) c /*this*/);

			setfield_critical(class_java_lang_reflect_Constructor,
							  o,
							  "slot",
							  "I",
							  jint,
							  i);

#if 0
			/*	    setfield_critical(class_constructor,o,"flag",           "I",		     jint,    (m->flags & (ACC_PRIVATE | 
					ACC_PUBLIC | ACC_PROTECTED))); */

			setfield_critical(class_java_lang_reflect_Constructor,
							  o,
							  "exceptionTypes",
							  "[Ljava/lang/Class;",
							  jobject,
							  (jobject) exceptiontypes);

			setfield_critical(class_java_lang_reflect_Constructor,
							  o,
							  "parameterTypes",
							  "[Ljava/lang/Class;",
							  jobject,
							  (jobject) native_get_parametertypes(m));
#endif
        }
	}

	return array_constructor;
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

			if (!use_class_as_object(inner))
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

				if (!use_class_as_object(outer))
					return NULL;

				return (java_lang_Class *) outer;
			}
		}
	}

	/* return NULL for arrayclasses and primitive classes */

	return NULL;
}


java_lang_reflect_Field* cacao_getField0(JNIEnv *env, java_lang_Class *that, java_lang_String *name, s4 public_only)
{
    classinfo *c;
    fieldinfo *f;               /* the field to be represented */
    java_lang_reflect_Field *o; /* result: field-object */
    int idx;

	c = (classinfo *) that;

    /* create Field object */

    o = (java_lang_reflect_Field *)
		native_new_and_init(class_java_lang_reflect_Field);

    /* get fieldinfo entry */
	
    idx = class_findfield_index_by_name(c, javastring_toutf(name, false));

    if (idx < 0) {
	    *exceptionptr = new_exception(string_java_lang_NoSuchFieldException);
	    return NULL;
	}

    f = &(c->fields[idx]);

    if (f) {
		if (public_only && !(f->flags & ACC_PUBLIC)) {
			/* field is not public  and public only had been requested*/
			*exceptionptr =
				new_exception(string_java_lang_NoSuchFieldException);
			return NULL;
		}

		/* initialize instance fields */

		setfield_critical(class_java_lang_reflect_Field,
						  o,
						  "declaringClass",
						  "Ljava/lang/Class;",
						  jobject,
						  (jobject) that /*this*/);

		/*      ((java_lang_reflect_Field*)(o))->flag=f->flags;*/
		/* save type in slot-field for faster processing */
		/* setfield_critical(c,o,"flag",           "I",		    jint,    (jint) f->flags); */
		/*o->flag = f->flags;*/

		setfield_critical(class_java_lang_reflect_Field,
						  o,
						  "slot",
						  "I",
						  jint,
						  (jint) idx);

		setfield_critical(class_java_lang_reflect_Field,
						  o,
						  "name",
						  "Ljava/lang/String;",
						  jstring,
						  (jstring) name);

		/*setfield_critical(c,o,"type",           "Ljava/lang/Class;",  jclass,  fieldtype);*/

		return o;
    }

    return NULL;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredFields
 * Signature: (Z)[Ljava/lang/reflect/Field;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredFields(JNIEnv *env, jclass clazz, java_lang_Class *klass, s4 publicOnly)
{
    classinfo        *c;
    java_objectarray *array_field; /* result: array of field-objects */
	java_lang_String *s;
    int public_fields = 0;         /* number of elements in field-array */
    int pos = 0;
    int i;

    c = (classinfo *) klass;

    /* determine number of fields */

    for (i = 0; i < c->fieldscount; i++)
		if ((c->fields[i].flags & ACC_PUBLIC) || (publicOnly == 0))
			public_fields++;

    /* create array of fields */

    array_field =
		builtin_anewarray(public_fields, class_java_lang_reflect_Field);

    /* creation of array failed */

    if (!array_field)
		return NULL;

    /* get the fields and store in the array */

    for (i = 0; i < c->fieldscount; i++) {
		if ((c->fields[i].flags & ACC_PUBLIC) || (publicOnly == 0)) {
			s = (java_lang_String *) javastring_new(c->fields[i].name);

			array_field->data[pos++] = (java_objectheader *)
				cacao_getField0(env, klass, s, publicOnly);
		}
	}

    return array_field;
}


/*
 * Class:     java/lang/VMClass
 * Method:    getInterfaces
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getInterfaces(JNIEnv *env, jclass clazz, java_lang_Class *that)
{
	classinfo        *c;
	java_objectarray *oa;
	u4                i;

	c = (classinfo *) that;

	oa = builtin_anewarray(c->interfacescount, class_java_lang_Class);

	if (!oa)
		return NULL;

	for (i = 0; i < c->interfacescount; i++) {
		if (!use_class_as_object(c->interfaces[i].cls))
			return NULL;

		oa->data[i] = (java_objectheader *) c->interfaces[i].cls;
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
	java_objectarray *array_method;     /* result: array of Method-objects */
	java_objectarray *exceptiontypes;  /* the exceptions thrown by the method */
	methodinfo               *m;      /* the current method to be represented */
	s4 public_methods;          /* number of public methods of the class */
	s4 pos;
	s4 i;

	c = (classinfo *) klass;    
	public_methods = 0;

	/* JOWENN: array classes do not declare methods according to mauve test.  */
	/* It should be considered, if we should return to my old clone method    */
	/* overriding instead of declaring it as a member function.               */

	if (Java_java_lang_VMClass_isArray(env, clazz, klass))
		return builtin_anewarray(0, class_java_lang_reflect_Method);

    /* determine number of methods */

    for (i = 0; i < c->methodscount; i++) {
		if (((c->methods[i].flags & ACC_PUBLIC) || (publicOnly == false)) &&
			((c->methods[i].name != utf_init) &&
			 (c->methods[i].name != utf_clinit))
			)
			public_methods++;
	}

    array_method =
		builtin_anewarray(public_methods, class_java_lang_reflect_Method);

    if (!array_method) 
		return NULL;

    for (i = 0, pos = 0; i < c->methodscount; i++) {
		if (((c->methods[i].flags & ACC_PUBLIC) || (publicOnly == false)) && 
			((c->methods[i].name != utf_init) &&
			 (c->methods[i].name != utf_clinit))
			) {

			m = &c->methods[i];
			o = native_new_and_init(class_java_lang_reflect_Method);
			array_method->data[pos++] = o;

			/* array of exceptions declared to be thrown, information not available !! */
			exceptiontypes = builtin_anewarray(0, class_java_lang_Class);


			/* initialize instance fields */

			rm = (java_lang_reflect_Method *) o;

			rm->declaringClass = klass;
			rm->name           = javastring_new(m->name);
			rm->slot           = i;
        }
	}

    return array_method;
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
JNIEXPORT java_lang_String* JNICALL Java_java_lang_VMClass_getName(JNIEnv *env, jclass clazz, java_lang_Class *that)
{
	classinfo        *c;
	java_lang_String *s;
	u4                i;

	c = (classinfo *) that;
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
 * Class:     java/lang/VMClass
 * Method:    getBeautifiedName
 * Signature: (Ljava/lang/Class;)Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_VMClass_getBeautifiedName(JNIEnv *env, jclass clazz, java_lang_Class *par1)
{
    u4 dimCnt;
    classinfo *c = (classinfo *) (par1);

    char *utf__ptr = c->name->text;      /* current position in utf-text */
    char **utf_ptr = &utf__ptr;
    char *desc_end = UTF_END(c->name);   /* points behind utf string     */
    java_lang_String *s;
    char *str = NULL;
    s4   len;
    s4   i;
    
#if 0
    log_text("Java_java_lang_VMClass_getBeautifiedName");
    utf_display(c->name);
    log_text("beautifying");
#endif
    dimCnt=0;
    while ( *utf_ptr != desc_end ) {
		if (utf_nextu2(utf_ptr)=='[') dimCnt++;
		else break;
    }
    utf__ptr = (*utf_ptr) - 1;

    len = 0;

#if 0	
    log_text("------>");
    utf_display(c->name);
    log_text("<------");
#endif 

    if (((*utf_ptr) + 1) == desc_end) {
	    for (i = 0; i < PRIMITIVETYPE_COUNT; i++) {
			if (primitivetype_table[i].typesig == (*utf__ptr)) {
				len = dimCnt * 2 + strlen(primitivetype_table[i].name);
				str = MNEW(char, len + 1);
				strcpy(str, primitivetype_table[i].name);
				break;
			}
	    }
    }

    if (len == 0) {
		if (dimCnt>0) {
			len = dimCnt + strlen(c->name->text) - 2;
			str = MNEW(char, len + 1);
			strncpy(str, ++utf__ptr, len - 2 * dimCnt);	   
		} else {
			len = strlen(c->name->text);
			str = MNEW(char, len + 1);
			strncpy(str, utf__ptr, len);	   
		}
		
    }	

    dimCnt = len - 2 * dimCnt;
    str[len] = 0;
    for (i = len - 1; i >= dimCnt; i = i - 2) {
		str[i] = ']';
		str[i - 1] = '[';
    }

    s = javastring_new(utf_new_char(str));
    MFREE(str, char, len + 1);

    if (!s) return NULL;

	/* return string where '/' is replaced by '.' */
	for (i = 0; i < s->value->header.size; i++) {
		if (s->value->data[i] == '/') s->value->data[i] = '.';
	}
	
	return s;
}



/*
 * Class:     java/lang/Class
 * Method:    getSuperclass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClass_getSuperclass(JNIEnv *env, jclass clazz, java_lang_Class *that)
{
	classinfo *c;
	classinfo *sc;

	c = (classinfo *) that;
	sc = c->super.cls;

	if (c->flags & ACC_INTERFACE)
		return NULL;

	if (!sc)
		return NULL;

	if (!use_class_as_object(sc))
		return NULL;

	return (java_lang_Class *) sc;
}


/*
 * Class:     java/lang/Class
 * Method:    isArray
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isArray(JNIEnv *env, jclass clazz, java_lang_Class *that)
{
    classinfo *c = (classinfo *) that;

    return c->vftbl->arraydesc != NULL;
}


/*
 * Class:     java/lang/Class
 * Method:    isAssignableFrom
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isAssignableFrom(JNIEnv *env, jclass clazz, java_lang_Class *klass, java_lang_Class *c)
{
	if (c == NULL) {
		*exceptionptr = new_nullpointerexception();
		return 0;
	}

	/* XXX this may be wrong for array classes */
	return builtin_isanysubclass((classinfo *) c, (classinfo *) klass);
}


/*
 * Class:     java/lang/Class
 * Method:    isInstance
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isInstance(JNIEnv *env, jclass clazz, java_lang_Class *that, java_lang_Object *obj)
{
	return builtin_instanceof((java_objectheader *) obj, (classinfo *) that);
}


/*
 * Class:     java/lang/Class
 * Method:    isInterface
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isInterface(JNIEnv *env, jclass clazz, java_lang_Class *that)
{
	classinfo *c;

	c = (classinfo *) that;

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
 */
