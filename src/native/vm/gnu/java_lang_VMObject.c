/* src/native/vm/gnu/java_lang_VMObject.c - java/lang/VMObject

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

   $Id: java_lang_VMObject.c 8111 2007-06-20 13:51:38Z twisti $

*/


#include "config.h"

#include <stdint.h>

#include "native/jni.h"
#include "native/native.h"

#include "native/include/java_lang_Class.h"            /* required by j.l.VMO */
#include "native/include/java_lang_Cloneable.h"        /* required by j.l.VMO */
#include "native/include/java_lang_Object.h"           /* required by j.l.VMO */

#include "native/include/java_lang_VMObject.h"

#include "native/vm/java_lang_Object.h"

#include "vmcore/utf8.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "getClass",  "(Ljava/lang/Object;)Ljava/lang/Class;",     (void *) (intptr_t) &Java_java_lang_VMObject_getClass  },
	{ "clone",     "(Ljava/lang/Cloneable;)Ljava/lang/Object;", (void *) (intptr_t) &Java_java_lang_VMObject_clone     },
	{ "notify",    "(Ljava/lang/Object;)V",                     (void *) (intptr_t) &Java_java_lang_VMObject_notify    },
	{ "notifyAll", "(Ljava/lang/Object;)V",                     (void *) (intptr_t) &Java_java_lang_VMObject_notifyAll },
	{ "wait",      "(Ljava/lang/Object;JI)V",                   (void *) (intptr_t) &Java_java_lang_VMObject_wait      },
};


/* _Jv_java_lang_VMObject_init *************************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_VMObject_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/VMObject");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/VMObject
 * Method:    getClass
 * Signature: (Ljava/lang/Object;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMObject_getClass(JNIEnv *env, jclass clazz, java_lang_Object *obj)
{
	return _Jv_java_lang_Object_getClass(obj);
}


/*
 * Class:     java/lang/VMObject
 * Method:    clone
 * Signature: (Ljava/lang/Cloneable;)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_VMObject_clone(JNIEnv *env, jclass clazz, java_lang_Cloneable *this)
{
	return _Jv_java_lang_Object_clone(this);
}


/*
 * Class:     java/lang/VMObject
 * Method:    notify
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_notify(JNIEnv *env, jclass clazz, java_lang_Object *this)
{
	_Jv_java_lang_Object_notify(this);
}


/*
 * Class:     java/lang/VMObject
 * Method:    notifyAll
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_notifyAll(JNIEnv *env, jclass clazz, java_lang_Object *this)
{
	_Jv_java_lang_Object_notifyAll(this);
}


/*
 * Class:     java/lang/VMObject
 * Method:    wait
 * Signature: (Ljava/lang/Object;JI)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_wait(JNIEnv *env, jclass clazz, java_lang_Object *o, int64_t ms, int32_t ns)
{
	_Jv_java_lang_Object_wait(o, ms, ns);
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
