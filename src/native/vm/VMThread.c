/* src/native/vm/VMThread.c - java/lang/VMThread

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

   $Id: VMThread.c 2865 2005-06-28 18:47:50Z twisti $

*/


#include "config.h"
#include "types.h"

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_ThreadGroup.h"
#include "native/include/java_lang_Object.h"            /* java_lang_Thread.h */
#include "native/include/java_lang_Throwable.h"         /* java_lang_Thread.h */
#include "native/include/java_lang_VMThread.h"
#include "native/include/java_lang_Thread.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
# endif
#endif

#include "toolbox/logging.h"
#include "vm/exceptions.h"
#include "vm/options.h"


/*
 * Class:     java/lang/VMThread
 * Method:    countStackFrames
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_countStackFrames(JNIEnv *env, java_lang_VMThread *this)
{
    log_text("java_lang_VMThread_countStackFrames called");

    return 0;
}


/*
 * Class:     java/lang/VMThread
 * Method:    start
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_start(JNIEnv *env, java_lang_VMThread *this, s8 stacksize)
{
	if (runverbose) 
		log_text("java_lang_VMThread_start called");

#if defined(USE_THREADS)
	this->thread->vmThread = this;
	startThread((thread *) this->thread);
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    interrupt
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_interrupt(JNIEnv *env, java_lang_VMThread *this)
{
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	interruptThread(this);
#else
	log_text("Java_java_lang_VMThread_interrupt called");
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    isInterrupted
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_isInterrupted(JNIEnv *env, java_lang_VMThread *this)
{
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	return isInterruptedThread(this);
#else
	log_text("Java_java_lang_VMThread_isInterrupted called");
	return 0;
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    suspend
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_suspend(JNIEnv *env, java_lang_VMThread *this)
{
	if (runverbose)
		log_text("java_lang_VMThread_suspend called");

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	suspendThread((thread *) this->thread);
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    resume
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_resume(JNIEnv *env, java_lang_VMThread *this)
{
	if (runverbose)
		log_text("java_lang_VMThread_resume0 called");

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	resumeThread((thread *) this->thread);
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    nativeSetPriority
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeSetPriority(JNIEnv *env, java_lang_VMThread *this, s4 priority)
{
    if (runverbose) 
		log_text("java_lang_VMThread_nativeSetPriority called");

#if defined(USE_THREADS)
	setPriorityThread((thread *) this->thread, priority);
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    nativeStop
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeStop(JNIEnv *env, java_lang_VMThread *this, java_lang_Throwable *t)
{
	if (runverbose)
		log_text ("java_lang_VMThread_nativeStop called");

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	if (currentThread == (thread *) this->thread) {
		log_text("killing");
		killThread(0);
		/*
		  exceptionptr = proto_java_lang_ThreadDeath;
		  return;
		*/

	} else {
		/*CONTEXT((thread*)this)*/ this->flags |= THREAD_FLAGS_KILLED;
		resumeThread((thread *) this->thread);
	}
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    currentThread
 * Signature: ()Ljava/lang/Thread;
 */
JNIEXPORT java_lang_Thread* JNICALL Java_java_lang_VMThread_currentThread(JNIEnv *env, jclass clazz)
{
	java_lang_Thread *t;

#if defined(USE_THREADS)
#if !defined(NATIVE_THREADS)
	t = (java_lang_Thread *) currentThread;
#else
	t = ((threadobject*) THREADOBJECT)->o.thread;
#endif

	if (t == NULL)
		log_text("t ptr is NULL\n");
  
	if (!t->group) {
		/* ThreadGroup of currentThread is not initialized */

		t->group = (java_lang_ThreadGroup *)
			native_new_and_init(class_java_lang_ThreadGroup);

		if (t->group == NULL)
			log_text("unable to create ThreadGroup");
  	}

	return t;
#else
	return NULL;
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    yield
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_yield(JNIEnv *env, jclass clazz)
{
	if (runverbose)
		log_text("java_lang_VMThread_yield called");

#if defined(USE_THREADS)
	yieldThread();
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    interrupted
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_interrupted(JNIEnv *env, jclass clazz)
{
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	return interruptedThread();
#else
	log_text("Java_java_lang_VMThread_interrupted");
	return 0;
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    holdsLock
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_holdsLock(JNIEnv *env, jclass clazz, java_lang_Object* o)
{
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	return threadHoldsLock((threadobject*) THREADOBJECT,
						   (java_objectheader *) o);
#else
	/* I don't know how to find out [stefan] */
	return 0;
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
