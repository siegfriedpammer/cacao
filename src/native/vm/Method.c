/* src/native/vm/Method.c - java/lang/reflect/Method

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

   $Id: Method.c 4807 2006-04-21 13:08:00Z edwin $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_reflect_Method.h"
#include "vm/access.h"
#include "vm/global.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/initialize.h"
#include "vm/stringlocal.h"


/*
 * Class:     java/lang/reflect/Method
 * Method:    getModifiersInternal
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Method_getModifiersInternal(JNIEnv *env, java_lang_reflect_Method *this)
{
	classinfo  *c;
	methodinfo *m;

	c = (classinfo *) this->declaringClass;
	m = &(c->methods[this->slot]);

	return m->flags;
}


/*
 * Class:     java/lang/reflect/Method
 * Method:    getReturnType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_reflect_Method_getReturnType(JNIEnv *env, java_lang_reflect_Method *this)
{
	classinfo  *c;
	methodinfo *m;

	c = (classinfo *) this->declaringClass;
	m = &(c->methods[this->slot]);

	return (java_lang_Class *) native_get_returntype(m);
}


/*
 * Class:     java/lang/reflect/Method
 * Method:    getParameterTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_reflect_Method_getParameterTypes(JNIEnv *env, java_lang_reflect_Method *this)
{
	classinfo  *c;
	methodinfo *m;

	c = (classinfo *) this->declaringClass;
	m = &(c->methods[this->slot]);

	return native_get_parametertypes(m);
}


/*
 * Class:     java/lang/reflect/Method
 * Method:    getExceptionTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_reflect_Method_getExceptionTypes(JNIEnv *env, java_lang_reflect_Method *this)
{
	classinfo  *c;
	methodinfo *m;

	c = (classinfo *) this->declaringClass;
	m = &(c->methods[this->slot]);

	return native_get_exceptiontypes(m);
}


/*
 * Class:     java/lang/reflect/Method
 * Method:    invokeNative
 * Signature: (Ljava/lang/Object;[Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_Method_invokeNative(JNIEnv *env, java_lang_reflect_Method *this, java_lang_Object *o, java_objectarray *args, java_lang_Class *declaringClass, s4 slot)
{
	classinfo        *c;
	methodinfo       *m;
	java_objectarray *oa;
	classinfo        *callerclass;

	c = (classinfo *) declaringClass;
	m = &(c->methods[slot]);

	/* check method access */

	if (!(m->flags & ACC_PUBLIC) || !(c->flags & ACC_PUBLIC)) {
		/* check if we should bypass security checks (AccessibleObject) */

		if (this->flag == false) {
			/* get the calling class */

			oa = stacktrace_getClassContext();
			if (!oa)
				return NULL;

			/* this function is always called like this:

			       java.lang.reflect.Method.invokeNative (Native Method)
			   [0] java.lang.reflect.Method.invoke (Method.java:329)
			   [1] <caller>
			*/

			callerclass = (classinfo *) oa->data[1];

			if (!access_is_accessible_class(callerclass,c)
				|| !access_is_accessible_member(callerclass, c, m->flags)) 
			{
				*exceptionptr =
					new_exception(string_java_lang_IllegalAccessException);
				return NULL;
			}
		}
	}

	/* check if method class is initialized */

	if (!(c->state & CLASS_INITIALIZED))
		if (!initialize_class(c))
			return NULL;

	/* call the Java method via a helper function */

	return (java_lang_Object *) _Jv_jni_invokeNative(m, (jobject) o, args);
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
