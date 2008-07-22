/* src/native/vm/cldc1.1/java_lang_Object.c

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

#include <stdlib.h>

#include "vm/types.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_String.h"             /* required by j.l.C */
#include "native/include/java_lang_Class.h"

#include "native/include/java_lang_Object.h"

#include "threads/lock-common.h"

#include "vm/exceptions.hpp"


/* native methods implemented by this file ************************************/
 
static JNINativeMethod methods[] = {
	{ "getClass",  "()Ljava/lang/Class;",                   (void *) (ptrint) &Java_java_lang_Object_getClass  },
	{ "hashCode",  "()I",                                   (void *) (ptrint) &Java_java_lang_Object_hashCode  },
	{ "notify",    "()V",                                   (void *) (ptrint) &Java_java_lang_Object_notify    },
	{ "notifyAll", "()V",                                   (void *) (ptrint) &Java_java_lang_Object_notifyAll },
	{ "wait",      "(J)V",                                  (void *) (ptrint) &Java_java_lang_Object_wait      },
};
 
 
/* _Jv_java_lang_Object_init ***************************************************
 
   Register native functions.
 
*******************************************************************************/
 
void _Jv_java_lang_Object_init(void)
{
	utf *u;
 
	u = utf_new_char("java/lang/Object");
 
	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/Object
 * Method:    getClass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_Object_getClass(JNIEnv *env, java_lang_Object *obj)
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
 * Class:     java/lang/Object
 * Method:    hashCode
 * Signature: ()I
 */
JNIEXPORT int32_t JNICALL Java_java_lang_Object_hashCode(JNIEnv *env, java_lang_Object *this)
{
#if defined(ENABLE_GC_CACAO)
	assert(0);
#else
	return (int32_t) ((intptr_t) this);
#endif
}


/*
 * Class:     java/lang/Object
 * Method:    notify
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Object_notify(JNIEnv *env, java_lang_Object *this)
{
#if defined(ENABLE_THREADS)
	lock_notify_object((java_handle_t *) this);
#endif
}


/*
 * Class:     java/lang/Object
 * Method:    notifyAll
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Object_notifyAll(JNIEnv *env, java_lang_Object *this)
{
#if defined(ENABLE_THREADS)
	lock_notify_all_object((java_handle_t *) this);
#endif
}


/*
 * Class:     java/lang/Object
 * Method:    wait
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_Object_wait(JNIEnv *env, java_lang_Object *this, s8 timeout)
{
#if defined(ENABLE_JVMTI)
	/* Monitor Wait */
	if (jvmti) jvmti_MonitorWaiting(true, this, timeout);
#endif

#if defined(ENABLE_THREADS)
	lock_wait_for_object((java_handle_t *) this, timeout, 0);
#endif

#if defined(ENABLE_JVMTI)
	/* Monitor Waited */
	/* XXX: How do you know if wait timed out ?*/
	if (jvmti) jvmti_MonitorWaiting(false, this, 0);
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
 * vim:noexpandtab:sw=4:ts=4:
 */
