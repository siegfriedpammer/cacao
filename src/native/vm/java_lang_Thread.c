/* src/native/vm/java_lang_Thread.c - java/lang/Thread functions

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
            Joseph Wenninger
            Christian Thalinger

   $Id: java_lang_VMThread.c 6213 2006-12-18 17:36:06Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_ThreadGroup.h"
#include "native/include/java_lang_Object.h"            /* java_lang_Thread.h */
#include "native/include/java_lang_Throwable.h"         /* java_lang_Thread.h */
#include "native/include/java_lang_Thread.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "toolbox/logging.h"
#include "vm/options.h"


/*
 * Class:     java/lang/Thread
 * Method:    countStackFrames
 * Signature: ()I
 */
s4 _Jv_java_lang_Thread_countStackFrames(java_lang_Thread *this)
{
    log_text("java_lang_Thread_countStackFrames called");

    return 0;
}


/*
 * Class:     java/lang/Thread
 * Method:    start
 * Signature: (J)V
 */
void _Jv_java_lang_Thread_start(java_lang_Thread *this, s8 stacksize)
{
#if defined(ENABLE_THREADS)
	threadobject *thread;

	thread = (threadobject *) this;

	/* don't pass a function pointer (NULL) since we want Thread.run()V here */

	threads_start_thread(thread, NULL);
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    interrupt
 * Signature: ()V
 */
void _Jv_java_lang_Thread_interrupt(java_lang_Thread *this)
{
#if defined(ENABLE_THREADS)
	threadobject *thread;

	thread = (threadobject *) this;

	threads_thread_interrupt(thread);
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    isInterrupted
 * Signature: ()Z
 */
s4 _Jv_java_lang_Thread_isInterrupted(java_lang_Thread *this)
{
#if defined(ENABLE_THREADS)
	threadobject *thread;

	thread = (threadobject *) this;

	return threads_thread_has_been_interrupted(thread);
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    suspend
 * Signature: ()V
 */
void _Jv_java_lang_Thread_suspend(java_lang_Thread *this)
{
#if defined(ENABLE_THREADS)
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    resume
 * Signature: ()V
 */
void _Jv_java_lang_Thread_resume(java_lang_Thread *this)
{
#if defined(ENABLE_THREADS)
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    setPriority
 * Signature: (I)V
 */
void _Jv_java_lang_Thread_setPriority(java_lang_Thread *this, s4 priority)
{
#if defined(ENABLE_THREADS)
	threadobject *thread;

	thread = (threadobject *) this;

	threads_set_thread_priority(thread->tid, priority);
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    stop
 * Signature: (Ljava/lang/Object;)V
 */
void _Jv_java_lang_Thread_stop(java_lang_Thread *this, java_lang_Throwable *t)
{
#if defined(ENABLE_THREADS)
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    currentThread
 * Signature: ()Ljava/lang/Thread;
 */
java_lang_Thread *_Jv_java_lang_Thread_currentThread(void)
{
	threadobject     *thread;
	java_lang_Thread *t;

#if defined(ENABLE_THREADS)
	thread = THREADOBJECT;

	t = (java_lang_Thread *) thread;

	if (t == NULL)
		log_text("t ptr is NULL\n");
  
	if (t->group == NULL) {
		/* ThreadGroup of currentThread is not initialized */

		t->group = (java_lang_ThreadGroup *)
			native_new_and_init(class_java_lang_ThreadGroup);

		if (t->group == NULL)
			log_text("unable to create ThreadGroup");
  	}
#else
	/* we just return a fake java.lang.Thread object, otherwise we get
       NullPointerException's in GNU classpath */

	t = (java_lang_Thread *) builtin_new(class_java_lang_Thread);
#endif

	return t;
}


/*
 * Class:     java/lang/Thread
 * Method:    yield
 * Signature: ()V
 */
void _Jv_java_lang_Thread_yield(void)
{
#if defined(ENABLE_THREADS)
	threads_yield();
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    interrupted
 * Signature: ()Z
 */
s4 _Jv_java_lang_Thread_interrupted(void)
{
#if defined(ENABLE_THREADS)
	return threads_check_if_interrupted_and_reset();
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    holdsLock
 * Signature: (Ljava/lang/Object;)Z
 */
s4 _Jv_java_lang_Thread_holdsLock(java_lang_Object* obj)
{
#if defined(ENABLE_THREADS)
	java_objectheader *o;

	o = (java_objectheader *) obj;

	if (o == NULL) {
		exceptions_throw_nullpointerexception();
		return;
	}

	return lock_is_held_by_current_thread(o);
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    getState
 * Signature: ()Ljava/lang/String;
 */
java_lang_String *_Jv_java_lang_Thread_getState(java_lang_Thread *this)
{
	vm_abort("Java_java_lang_Thread_getState: IMPLEMENT ME!");

	return NULL;
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
