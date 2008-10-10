/* src/native/vm/gnuclasspath/java_lang_VMThread.cpp

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

#include "native/jni.hpp"
#include "native/llni.h"
#include "native/native.hpp"

#if defined(ENABLE_JNI_HEADERS)
# include "native/vm/include/java_lang_VMThread.h"
#endif

#include "threads/lock.hpp"
#include "threads/thread.hpp"

#include "vm/exceptions.hpp"
#include "vm/javaobjects.hpp"
#include "vm/string.hpp"
#include "vm/utf8.h"


// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     java/lang/VMThread
 * Method:    countStackFrames
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_java_lang_VMThread_countStackFrames(JNIEnv *env, jobject _this)
{
	log_println("Java_java_lang_VMThread_countStackFrames: Deprecated.  Not implemented.");

    return 0;
}


/*
 * Class:     java/lang/VMThread
 * Method:    start
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_start(JNIEnv *env, jobject _this, jlong stacksize)
{
#if defined(ENABLE_THREADS)
	java_lang_VMThread jlvmt(_this);

	threads_thread_start(jlvmt.get_thread());
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    interrupt
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_interrupt(JNIEnv *env, jobject _this)
{
#if defined(ENABLE_THREADS)
	java_handle_t *h;
	threadobject  *t;

	h = (java_handle_t *) _this;
	t = thread_get_thread(h);

	threads_thread_interrupt(t);
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    isInterrupted
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_VMThread_isInterrupted(JNIEnv *env, jobject _this)
{
#if defined(ENABLE_THREADS)
	java_handle_t *h;
	threadobject  *t;

	h = (java_handle_t *) _this;
	t = thread_get_thread(h);

	return thread_is_interrupted(t);
#else
	return 0;
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    suspend
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_suspend(JNIEnv *env, jobject _this)
{
#if defined(ENABLE_THREADS)
	log_println("Java_java_lang_VMThread_suspend: Deprecated.  Not implemented.");
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    resume
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_resume(JNIEnv *env, jobject _this)
{
#if defined(ENABLE_THREADS)
	log_println("Java_java_lang_VMThread_resume: Deprecated.  Not implemented.");
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    nativeSetPriority
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeSetPriority(JNIEnv *env, jobject _this, jint priority)
{
#if defined(ENABLE_THREADS)
	java_handle_t *h;
	threadobject  *t;

	h = (java_handle_t *) _this;
	t = thread_get_thread(h);

	threads_set_thread_priority(t->tid, priority);
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    nativeStop
 * Signature: (Ljava/lang/Throwable;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeStop(JNIEnv *env, jobject _this, jobject t)
{
#if defined(ENABLE_THREADS)
	log_println("Java_java_lang_VMThread_nativeStop: Deprecated.  Not implemented.");
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    currentThread
 * Signature: ()Ljava/lang/Thread;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMThread_currentThread(JNIEnv *env, jclass clazz)
{
	java_handle_t* h;

	h = thread_get_current_object();

	return (jobject) h;
}


/*
 * Class:     java/lang/VMThread
 * Method:    yield
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_yield(JNIEnv *env, jclass clazz)
{
#if defined(ENABLE_THREADS)
	threads_yield();
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    sleep
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_sleep(JNIEnv *env, jclass clazz, int64_t ms, int32_t ns)
{
#if defined(ENABLE_THREADS)
	threads_sleep(ms, ns);
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    interrupted
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_VMThread_interrupted(JNIEnv *env, jclass clazz)
{
#if defined(ENABLE_THREADS)
	threadobject *t;
	int32_t       interrupted;

	t = thread_get_current();

	interrupted = thread_is_interrupted(t);

	if (interrupted)
		thread_set_interrupted(t, false);

	return interrupted;
#else
	return 0;
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    holdsLock
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_VMThread_holdsLock(JNIEnv *env, jclass clazz, jobject o)
{
#if defined(ENABLE_THREADS)
	java_handle_t *h;

	h = (java_handle_t *) o;

	if (h == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}

	return lock_is_held_by_current_thread(h);
#else
	return 0;
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    getState
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_java_lang_VMThread_getState(JNIEnv *env, jobject _this)
{
#if defined(ENABLE_THREADS)
	java_handle_t *h;
	threadobject  *t;
	int            state;
	utf           *u;
	java_handle_t *o;

	h = (java_handle_t *) _this;
	t = thread_get_thread(h);

	state = cacaothread_get_state(t);
	
	switch (state) {
	case THREAD_STATE_NEW:
		u = utf_new_char("NEW");
		break;
	case THREAD_STATE_RUNNABLE:
		u = utf_new_char("RUNNABLE");
		break;
	case THREAD_STATE_BLOCKED:
		u = utf_new_char("BLOCKED");
		break;
	case THREAD_STATE_WAITING:
		u = utf_new_char("WAITING");
		break;
	case THREAD_STATE_TIMED_WAITING:
		u = utf_new_char("TIMED_WAITING");
		break;
	case THREAD_STATE_TERMINATED:
		u = utf_new_char("TERMINATED");
		break;
	default:
		vm_abort("Java_java_lang_VMThread_getState: unknown thread state %d", state);

		/* Keep compiler happy. */

		u = NULL;
	}

	o = javastring_new(u);

	return (jstring) o;
#else
	return NULL;
#endif
}

} // extern "C"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ (char*) "countStackFrames",  (char*) "()I",                      (void*) (uintptr_t) &Java_java_lang_VMThread_countStackFrames  },
	{ (char*) "start",             (char*) "(J)V",                     (void*) (uintptr_t) &Java_java_lang_VMThread_start             },
	{ (char*) "interrupt",         (char*) "()V",                      (void*) (uintptr_t) &Java_java_lang_VMThread_interrupt         },
	{ (char*) "isInterrupted",     (char*) "()Z",                      (void*) (uintptr_t) &Java_java_lang_VMThread_isInterrupted     },
	{ (char*) "suspend",           (char*) "()V",                      (void*) (uintptr_t) &Java_java_lang_VMThread_suspend           },
	{ (char*) "resume",            (char*) "()V",                      (void*) (uintptr_t) &Java_java_lang_VMThread_resume            },
	{ (char*) "nativeSetPriority", (char*) "(I)V",                     (void*) (uintptr_t) &Java_java_lang_VMThread_nativeSetPriority },
	{ (char*) "nativeStop",        (char*) "(Ljava/lang/Throwable;)V", (void*) (uintptr_t) &Java_java_lang_VMThread_nativeStop        },
	{ (char*) "currentThread",     (char*) "()Ljava/lang/Thread;",     (void*) (uintptr_t) &Java_java_lang_VMThread_currentThread     },
	{ (char*) "yield",             (char*) "()V",                      (void*) (uintptr_t) &Java_java_lang_VMThread_yield             },
	{ (char*) "sleep",             (char*) "(JI)V",                    (void*) (uintptr_t) &Java_java_lang_VMThread_sleep             },
	{ (char*) "interrupted",       (char*) "()Z",                      (void*) (uintptr_t) &Java_java_lang_VMThread_interrupted       },
	{ (char*) "holdsLock",         (char*) "(Ljava/lang/Object;)Z",    (void*) (uintptr_t) &Java_java_lang_VMThread_holdsLock         },
	{ (char*) "getState",          (char*) "()Ljava/lang/String;",     (void*) (uintptr_t) &Java_java_lang_VMThread_getState          },
};


/* _Jv_java_lang_VMThread_init *************************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_VMThread_init(void)
{
	utf* u = utf_new_char("java/lang/VMThread");

	NativeMethods& nm = VM::get_current()->get_nativemethods();
	nm.register_methods(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */