/* nat/Thread.c - java/lang/Thread

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   $Id: Thread.c 1173 2004-06-16 14:56:18Z jowenn $

*/


#include "jni.h"
#include "builtin.h"
#include "types.h"
#include "native.h"
#include "loader.h"
#include "tables.h"
#include "threads/thread.h"
#include "toolbox/logging.h"
#include "java_lang_ThreadGroup.h"
#include "java_lang_Object.h"         /* needed for java_lang_Thread.h */
#include "java_lang_Throwable.h"      /* needed for java_lang_Thread.h */
#include "java_lang_VMThread.h"
#include "java_lang_Thread.h"


/*
 * Class:     java/lang/Thread
 * Method:    countStackFrames
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_countStackFrames(JNIEnv *env, java_lang_VMThread *this)
{
    log_text("java_lang_VMThread_countStackFrames called");

    return 0;
}

/*
 * Class:     java/lang/Thread
 * Method:    currentThread
 * Signature: ()Ljava/lang/Thread;
 */
JNIEXPORT java_lang_Thread* JNICALL Java_java_lang_VMThread_currentThread(JNIEnv *env, jclass clazz)
{
	java_lang_Thread *t;

	if (runverbose)
		log_text("java_lang_VMThread_currentThread called");

#if defined(USE_THREADS)
#if !defined(NATIVE_THREADS)
	t = (java_lang_Thread *) currentThread;
#else
	t = THREADOBJECT;
#endif
  
	if (!t->group) {
		log_text("java_lang_VMThread_currentThread: t->group=NULL");
		/* ThreadGroup of currentThread is not initialized */

		t->group = (java_lang_ThreadGroup *) 
			native_new_and_init(class_new(utf_new_char("java/lang/ThreadGroup")));

		if (t->group == 0) 
			log_text("unable to create ThreadGroup");
  	}

	return (java_lang_Thread *) t;
#else
	return 0;	
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    nativeInterrupt
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_interrupt(JNIEnv *env, java_lang_VMThread *this)
{
	log_text("Java_java_lang_VMThread_interrupt0 called");
}


/*
 * Class:     java/lang/Thread
 * Method:    isAlive
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_isAlive(JNIEnv *env, java_lang_VMThread *this)
{
	if (runverbose)
		log_text("java_lang_VMThread_isAlive called");

#if defined(USE_THREADS)
	return aliveThread((thread *) this->thread);
#endif
}



/*
 * Class:     java_lang_Thread
 * Method:    isInterrupted
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_isInterrupted(JNIEnv *env, java_lang_VMThread *this)
{
	log_text("Java_java_lang_VMThread_isInterrupted  called");
	return 0;
}


/*
 * Class:     java/lang/Thread
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_registerNatives(JNIEnv *env, jclass clazz)
{
	/* empty */
}


/*
 * Class:     java/lang/Thread
 * Method:    resume0
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
 * Class:     java/lang/Thread
 * Method:    setPriority0
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeSetPriority(JNIEnv *env, java_lang_VMThread *this, s4 par1)
{
    if (runverbose) 
		log_text("java_lang_VMThread_setPriority0 called");

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	setPriorityThread((thread *) this->thread, par1);
#endif
}


/*
 * Class:     java_lang_Thread
 * Method:    sleep
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_sleep(JNIEnv *env, jclass clazz, s8 millis, s4 par2)
{
	if (runverbose)
		log_text("java_lang_VMThread_sleep called");

#if defined(USE_THREADS)
	sleepThread(millis);
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    start
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_start(JNIEnv *env, java_lang_VMThread *this, s8 par1)
{
	if (runverbose) 
		log_text("java_lang_VMThread_start called");

#if defined(USE_THREADS)
#warning perhaps it would be better to always work with the vmthread structure in the thread code (jowenn)
	if (this->thread->vmThread==0)
		this->thread->vmThread=this;
	startThread((thread*)(this->thread));
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    stop0
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeStop(JNIEnv *env, java_lang_VMThread *this, java_lang_Throwable *par1)
{
	if (runverbose)
		log_text ("java_lang_VMThread_stop0 called");


#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	if (currentThread == (thread*)this->thread) {
		log_text("killing");
		killThread(0);
		/*
		  exceptionptr = proto_java_lang_ThreadDeath;
		  return;
		*/

	} else {
		/*CONTEXT((thread*)this)*/ this->flags |= THREAD_FLAGS_KILLED;
		resumeThread((thread*)this->thread);
	}
#endif
}


/*
 * Class:     java/lang/Thread
 * Method:    suspend0
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_suspend(JNIEnv *env, java_lang_VMThread *this)
{
	if (runverbose)
		log_text("java_lang_VMThread_suspend0 called");

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	suspendThread((thread*)this->thread);
#endif
}


/*
 * Class:     java/lang/Thread
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
 * Class:     java_lang_Thread
 * Method:    interrupted
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_interrupted(JNIEnv *env, jclass clazz)
{
	log_text("Java_java_lang_VMThread_interrupted");

	return 0;
}


/*
 * Class:     java_lang_Thread
 * Method:    nativeInit
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeInit(JNIEnv *env, java_lang_VMThread *this, s8 par1)
{
/*
	if (*exceptionptr)
		log_text("There has been an exception, strange...");*/

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initThread(this->thread);
#endif
	this->thread->priority = 5;
}


/*
 * Class:     java_lang_Thread
 * Method:    holdsLock
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_holdsLock(JNIEnv *env, jclass clazz, java_lang_Object *par1)
{
	return 0;
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
