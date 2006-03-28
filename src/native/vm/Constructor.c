/* src/native/vm/Constructor.c - java/lang/reflect/Constructor

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

   $Id: Constructor.c 4691 2006-03-28 09:41:33Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <stdlib.h>

#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_reflect_Constructor.h"
#include "toolbox/logging.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/method.h"
#include "vm/stringlocal.h"


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    newInstance
 * Signature: ([Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_Constructor_constructNative(JNIEnv *env, java_lang_reflect_Constructor *this, java_objectarray *args, java_lang_Class *declaringClass, s4 slot)
{
	classinfo         *c;
	methodinfo        *m;
	java_objectheader *o;

	c = (classinfo *) declaringClass;

#if 0
	/* find initializer */

	if (!args) {
		if (this->parameterTypes->header.size != 0) {
			*exceptionptr =
				new_exception_message(string_java_lang_IllegalArgumentException,
									  "wrong number of arguments");
			return NULL;
		}

	} else {
		if (this->parameterTypes->header.size != args->header.size) {
			*exceptionptr =
				new_exception_message(string_java_lang_IllegalArgumentException,
									  "wrong number of arguments");
			return NULL;
		}
	}
#endif

	if (this->slot >= c->methodscount) {
		log_text("illegal index in methods table");
		return NULL;
	}

	/* create object */

	o = builtin_new(c);

	if (!o)
		return NULL;
        
	m = &(c->methods[this->slot]);

	if (m->name != utf_init) {
		/* XXX throw an exception here, although this should never happen */

		assert(0);
	}

	/* call initializer */

	(void) _Jv_jni_invokeNative(m, o, args);

	return (java_lang_Object *) o;
}


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getModifiersInternal
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Constructor_getModifiersInternal(JNIEnv *env, java_lang_reflect_Constructor *this)
{
	classinfo  *c;
	methodinfo *m;

	c = (classinfo *) (this->clazz);
	m = &(c->methods[this->slot]);

	return m->flags;
}


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getParameterTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_reflect_Constructor_getParameterTypes(JNIEnv *env, java_lang_reflect_Constructor *this)
{
	classinfo  *c;
	methodinfo *m;

	c = (classinfo *) this->clazz;

	if ((this->slot < 0) || (this->slot >= c->methodscount)) {
		log_text("error illegal slot for constructor in class");
		assert(0);
	}

	m = &(c->methods[this->slot]);

	return native_get_parametertypes(m);
}


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getExceptionTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_reflect_Constructor_getExceptionTypes(JNIEnv *env, java_lang_reflect_Constructor *this)
{
	classinfo  *c;
	methodinfo *m;

	c = (classinfo *) this->clazz;

	if ((this->slot < 0) || (this->slot >= c->methodscount)) {
		log_text("error illegal slot for constructor in class");
		assert(0);
	}

	m = &(c->methods[this->slot]);

	return native_get_exceptiontypes(m);
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
