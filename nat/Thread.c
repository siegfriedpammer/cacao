/* class: java/lang/Thread */


#include "jni.h"
#include "types.h"
#include "native.h"
#include "loader.h"
#include "tables.h"
#include "threads/thread.h"
#include "toolbox/loging.h"
#include "java_lang_ThreadGroup.h"
#include "java_lang_Object.h"         /* needed for java_lang_Thread.h */
#include "java_lang_Throwable.h"      /* needed for java_lang_Thread.h */
#include "java_lang_Thread.h"


/*
 * Class:     java/lang/Thread
 * Method:    countStackFrames
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_countStackFrames ( JNIEnv *env ,  struct java_lang_Thread* this)
{
    log_text ("java_lang_Thread_countStackFrames called");
    return 0;         /* not yet implemented */
}

/*
 * Class:     java/lang/Thread
 * Method:    currentThread
 * Signature: ()Ljava/lang/Thread;
 */
JNIEXPORT struct java_lang_Thread* JNICALL Java_java_lang_Thread_currentThread ( JNIEnv *env ,jclass clazz )
{
  struct java_lang_Thread* t;

  if (runverbose)
    log_text ("java_lang_Thread_currentThread called");


  #ifdef USE_THREADS

 	 t = (struct java_lang_Thread*) currentThread; 
  
	 if (!t->group) {
		log_text("java_lang_Thread_currentThread: t->group=NULL");
    		/* ThreadGroup of currentThread is not initialized */

    		t->group = (java_lang_ThreadGroup *) 
			native_new_and_init(loader_load(utf_new_char("java/lang/ThreadGroup")));

    		if (t->group == 0) 
      			log_text("unable to create ThreadGroup");
  	}

	return (struct java_lang_Thread*) currentThread;
  #else
	return 0;	
  #endif
}

/*
 * Class:     java/lang/Thread
 * Method:    nativeInterrupt
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_nativeInterrupt ( JNIEnv *env ,  struct java_lang_Thread* this)
{
  log_text("Java_java_lang_Thread_interrupt0  called");
  /* not yet implemented */
}

/*
 * Class:     java/lang/Thread
 * Method:    isAlive
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_isAlive ( JNIEnv *env ,  struct java_lang_Thread* this)
{
    if (runverbose)
	log_text ("java_lang_Thread_isAlive called");

#ifdef USE_THREADS
    return aliveThread((thread*)this);
#else
    return 0;
#endif
}



/*
 * Class:     java_lang_Thread
 * Method:    isInterrupted
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_isInterrupted (JNIEnv *env ,  struct java_lang_Thread* this )
{
    log_text("Java_java_lang_Thread_isInterrupted  called");
    return 0;			/* not yet implemented */
}

/*
 * Class:     java/lang/Thread
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_registerNatives ( JNIEnv *env ,jclass clazz )
{
  /* empty */
}

/*
 * Class:     java/lang/Thread
 * Method:    resume0
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_nativeResume ( JNIEnv *env ,  struct java_lang_Thread* this)
{
    if (runverbose)
	log_text ("java_lang_Thread_resume0 called");

#ifdef USE_THREADS
    resumeThread((thread*)this);
#endif
}

/*
 * Class:     java/lang/Thread
 * Method:    setPriority0
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_nativeSetPriority ( JNIEnv *env ,  struct java_lang_Thread* this, s4 par1)
{
    if (runverbose) 
	log_text ("java_lang_Thread_setPriority0 called");

#ifdef USE_THREADS
    setPriorityThread((thread*)this, par1);
#endif
}


/*
 * Class:     java_lang_Thread
 * Method:    sleep
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_sleep (JNIEnv *env , jclass clazz, s8 millis, s4 par2)
{
    if (runverbose)
	log_text ("java_lang_Thread_sleep called");

#ifdef USE_THREADS
    sleepThread(millis);
#endif
}

/*
 * Class:     java/lang/Thread
 * Method:    start
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_start ( JNIEnv *env ,  struct java_lang_Thread* this)
{
  if (runverbose) 
    log_text ("java_lang_Thread_start called");
    /*printf("THREAD PRIORITY: %d\n",this->priority);*/

  #ifdef USE_THREADS
	startThread((thread*)this);
  #endif
}

/*
 * Class:     java/lang/Thread
 * Method:    stop0
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_nativeStop ( JNIEnv *env ,  struct java_lang_Thread* this, struct java_lang_Throwable* par1)
{
  if (runverbose)
    log_text ("java_lang_Thread_stop0 called");


  #ifdef USE_THREADS
	if (currentThread == (thread*)this)
	{
	    log_text("killing");
	    killThread(0);
	    /*
	        exceptionptr = proto_java_lang_ThreadDeath;
	        return;
	    */
	}
	else
	{
	        CONTEXT((thread*)this).flags |= THREAD_FLAGS_KILLED;
	        resumeThread((thread*)this);
	}
   #endif
}

/*
 * Class:     java/lang/Thread
 * Method:    suspend0
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_nativeSuspend ( JNIEnv *env ,  struct java_lang_Thread* this)
{
  if (runverbose)
    log_text ("java_lang_Thread_suspend0 called");

  #ifdef USE_THREADS
	suspendThread((thread*)this);
  #endif

}

/*
 * Class:     java/lang/Thread
 * Method:    yield
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_yield ( JNIEnv *env  ,jclass clazz)
{
  if (runverbose)
    log_text ("java_lang_Thread_yield called");
  #ifdef USE_THREADS
	yieldThread();
  #endif
}

/*
 * Class:     java_lang_Thread
 * Method:    interrupted
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_interrupted (JNIEnv *env ,jclass clazz) {
	log_text("Java_java_lang_Thread_interrupted");
	return 0;
}
/*
 * Class:     java_lang_Thread
 * Method:    nativeInit
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_nativeInit (JNIEnv *env ,  struct java_lang_Thread* this , s8 par1) {
	log_text("Thread_nativeInit");
	if (*exceptionptr) log_text("There has been an exception, strange...");
	this->priority=5;
}

/*
 * Class:     java_lang_Thread
 * Method:    holdsLock
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_holdsLock (JNIEnv *env , jclass clazz, struct java_lang_Object* par1)
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
