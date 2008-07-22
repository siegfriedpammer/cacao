/* src/native/vm/gnuclasspath/java_lang_VMObject.c - java/lang/VMObject

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

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Class.h"            /* required by j.l.VMO */
#include "native/include/java_lang_Cloneable.h"        /* required by j.l.VMO */
#include "native/include/java_lang_Object.h"           /* required by j.l.VMO */

#include "native/include/java_lang_VMObject.h"

#include "threads/lock-common.h"

#include "vm/builtin.h"
#include "vm/exceptions.hpp"

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
	classinfo *c;

	if (obj == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	LLNI_class_get(obj, c);

	return LLNI_classinfo_wrap(c);
}


/*
 * Class:     java/lang/VMObject
 * Method:    clone
 * Signature: (Ljava/lang/Cloneable;)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_VMObject_clone(JNIEnv *env, jclass clazz, java_lang_Cloneable *this)
{
	java_handle_t *o;
	java_handle_t *co;

	o = (java_handle_t *) this;

	co = builtin_clone(NULL, o);

	return (java_lang_Object *) co;
}


/*
 * Class:     java/lang/VMObject
 * Method:    notify
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_notify(JNIEnv *env, jclass clazz, java_lang_Object *this)
{
#if defined(ENABLE_THREADS)
	lock_notify_object((java_handle_t *) this);
#endif
}


/*
 * Class:     java/lang/VMObject
 * Method:    notifyAll
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_notifyAll(JNIEnv *env, jclass clazz, java_lang_Object *this)
{
#if defined(ENABLE_THREADS)
	lock_notify_all_object((java_handle_t *) this);
#endif
}


/*
 * Class:     java/lang/VMObject
 * Method:    wait
 * Signature: (Ljava/lang/Object;JI)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_wait(JNIEnv *env, jclass clazz, java_lang_Object *o, int64_t ms, int32_t ns)
{
#if defined(ENABLE_JVMTI)
	/* Monitor Wait */
	if (jvmti) jvmti_MonitorWaiting(true, o, ms);
#endif

#if defined(ENABLE_THREADS)
	lock_wait_for_object((java_handle_t *) o, ms, ns);
#endif

#if defined(ENABLE_JVMTI)
	/* Monitor Waited */
	/* XXX: How do you know if wait timed out ?*/
	if (jvmti) jvmti_MonitorWaiting(false, o, 0);
#endif
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
