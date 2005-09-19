/* src/native/vm/Method.c - java/lang/reflect/Method

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

   $Id: Method.c 3215 2005-09-19 13:05:24Z twisti $

*/


#include <assert.h>

#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_reflect_Method.h"
#include "toolbox/logging.h"
#include "vm/global.h"
#include "vm/builtin.h"
#include "vm/jit/stacktrace.h"
#include "vm/exceptions.h"
#include "vm/stringlocal.h"


/*
 * Class:     java/lang/reflect/Method
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Method_getModifiers(JNIEnv *env, java_lang_reflect_Method *this)
{
	classinfo  *c;
	methodinfo *m;

	c = (classinfo *) this->declaringClass;

	if ((this->slot < 0) || (this->slot >= c->methodscount)) {
		log_text("error illegal slot for method in class");
		assert(0);
	}

	m = &(c->methods[this->slot]);

	return (m->flags &
			(ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_ABSTRACT |
			 ACC_STATIC | ACC_FINAL | ACC_SYNCHRONIZED | ACC_NATIVE |
			 ACC_STRICT));
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

	if ((this->slot < 0) || (this->slot >= c->methodscount)) {
		log_text("error illegal slot for method in class");
		assert(0);
	}

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

	if ((this->slot < 0) || (this->slot >= c->methodscount)) {
		log_text("error illegal slot for method in class");
		assert(0);
	}

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

	if ((this->slot < 0) || (this->slot >= c->methodscount)) {
		log_text("error illegal slot for method in class");
		assert(0);
	}

	m = &(c->methods[this->slot]);

	return native_get_exceptiontypes(m);
}


/*
 * Class:     java/lang/reflect/Method
 * Method:    invokeNative
 * Signature: (Ljava/lang/Object;[Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_Method_invokeNative(JNIEnv *env, java_lang_reflect_Method *this, java_lang_Object *obj, java_objectarray *params, java_lang_Class *declaringClass, s4 slot)
{
	classinfo  *c;
	methodinfo *m;
	jfieldID    fid;

	c = (classinfo *) declaringClass;

	if ((slot < 0) || (slot >= c->methodscount)) {
		log_text("error illegal slot for method in class(getParameterTypes)");
		assert(0);
	}

	m = &(c->methods[slot]);

#if defined(__ALPHA__) || defined(__I386__) || defined(__X86_64__)
	fid = getFieldID_critical(env, this->header.vftbl->class, "flag", "Z");

	if (!getField(this, jboolean, fid)) {
		methodinfo *callingMethod;
		bool        throwAccess;

		if (!(m->flags & ACC_PUBLIC)) {
			callingMethod = cacao_callingMethod();
			throwAccess = false;

			if (m->flags & ACC_PRIVATE) {
				if (c != callingMethod->class)
					throwAccess = true;

			} else {
				if (m->flags & ACC_PROTECTED) {
					if (!builtin_isanysubclass(callingMethod->class, c))
						throwAccess = true;

				} else {
					/* default visibility */
					if (c->packagename != callingMethod->class->packagename)
						throwAccess = true;
				}
			}

			if (throwAccess) {
				*exceptionptr =
					new_exception(string_java_lang_IllegalAccessException);

				return NULL;
			}
		}
	}

#endif

    return (java_lang_Object *)
		jni_method_invokeNativeHelper(env, m, (jobject) obj, params);
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
