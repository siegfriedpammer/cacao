/* src/native/vm/cldc1.1/java_lang_Thread.c

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

#include <stdint.h>

#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"

#include "native/include/java_lang_Thread.h"

#include "threads/threads-common.h"

#include "toolbox/logging.h"

#include "vm/builtin.h"


/* native methods implemented by this file ************************************/
 
static JNINativeMethod methods[] = {
	{ "currentThread", "()Ljava/lang/Thread;", (void *) (ptrint) &Java_java_lang_Thread_currentThread },
	{ "setPriority0",  "(II)V",                (void *) (ptrint) &Java_java_lang_Thread_setPriority0  },
	{ "sleep",         "(J)V",                 (void *) (ptrint) &Java_java_lang_Thread_sleep         },
	{ "start0",        "()V",                  (void *) (ptrint) &Java_java_lang_Thread_start0        },
	{ "isAlive",       "()Z",                  (void *) (ptrint) &Java_java_lang_Thread_isAlive       },
#if 0
	{ "activeCount",   "()I",                  (void *) (ptrint) &Java_java_lang_Thread_activeCount   },
	{ "setPriority0",  "(II)V",                (void *) (ptrint) &Java_java_lang_Thread_setPriority0  },
	{ "interrupt0",    "()V",                  (void *) (ptrint) &Java_java_lang_Thread_interrupt0    },
	{ "internalExit",  "()V",                  (void *) (ptrint) &Java_java_lang_Thread_internalExit  },
#endif
	{ "yield",         "()V",                  (void *) (ptrint) &Java_java_lang_Thread_yield         },
};


/* _Jv_java_lang_Thread_init ***************************************************
 
   Register native functions.
 
*******************************************************************************/
 
void _Jv_java_lang_Thread_init(void)
{
	utf *u;
 
	u = utf_new_char("java/lang/Thread");
 
	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/Thread
 * Method:    currentThread
 * Signature: ()Ljava/lang/Thread;
 */
JNIEXPORT java_lang_Thread* JNICALL Java_java_lang_Thread_currentThread(JNIEnv *env, jclass clazz)
{
	java_lang_Thread *thread;

	thread = (java_lang_Thread *) threads_get_current_object();

	return thread;
}


/*
 * Class:     java/lang/Thread
 * Method:    setPriority0
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_setPriority0(JNIEnv *env, java_lang_Thread *this, s4 oldPriority, s4 newPriority)
{
#if defined(ENABLE_THREADS)
	threadobject *t;

	t = (threadobject *) this->vm_thread;

	/* The threadobject is null when a thread is created in Java. The
	   priority is set later during startup. */

	if (t == NULL)
		return;

	threads_set_thread_priority(t->tid, newPriority);
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    sleep
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_sleep(JNIEnv *env, jclass clazz, s8 millis)
{
#if defined(ENABLE_THREADS)
	threads_sleep(millis, 0);
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    start0
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_start0(JNIEnv *env, java_lang_Thread *this)
{
#if defined(ENABLE_THREADS)
	threads_thread_start((java_handle_t *) this);
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    isAlive
 * Signature: ()Z
 */
JNIEXPORT int32_t JNICALL Java_java_lang_Thread_isAlive(JNIEnv *env, java_lang_Thread *this)
{
#if defined(ENABLE_THREADS)
	threadobject *t;
	bool          result;

	t = (threadobject *) this->vm_thread;

	if (t == NULL)
		return 0;

	result = threads_thread_is_alive(t);

	return result;
#else
	/* If threads are disabled, the only thread running is alive. */

	return 1;
#endif
}


#if 0
/*
 * Class:     java/lang/Thread
 * Method:    activeCount
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_activeCount(JNIEnv *env, jclass clazz)
{
}


/*
 * Class:     java/lang/Thread
 * Method:    setPriority0
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_setPriority0(JNIEnv *env, struct java_lang_Thread* this, s4 par1, s4 par2)
{
}


/*
 * Class:     java/lang/Thread
 * Method:    interrupt0
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_interrupt0(JNIEnv *env, struct java_lang_Thread* this)
{
}


/*
 * Class:     java/lang/Thread
 * Method:    internalExit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_internalExit(JNIEnv *env, struct java_lang_Thread* this)
{
}
#endif


/*
 * Class:     java/lang/Thread
 * Method:    yield
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_yield(JNIEnv *env, jclass clazz)
{
#if defined(ENABLE_THREADS)
	threads_yield();
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
