/* class: java/lang/Thread */

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
JNIEXPORT struct java_lang_Thread* JNICALL Java_java_lang_Thread_currentThread ( JNIEnv *env  )
{
  struct java_lang_Thread* t;

  if (runverbose)
    log_text ("java_lang_Thread_currentThread called");


  #ifdef USE_THREADS

 	 t = (struct java_lang_Thread*) currentThread; 
  
	 if (!t->group) {

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
 * Method:    interrupt0
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_interrupt0 ( JNIEnv *env ,  struct java_lang_Thread* this)
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
 * Class:     java/lang/Thread
 * Method:    isInterrupted
 * Signature: (Z)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_isInterrupted ( JNIEnv *env ,  struct java_lang_Thread* this, s4 par1)
{
    log_text("Java_java_lang_Thread_isInterrupted  called");
    return 0;			/* not yet implemented */
}

/*
 * Class:     java/lang/Thread
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_registerNatives ( JNIEnv *env  )
{
  /* empty */
}

/*
 * Class:     java/lang/Thread
 * Method:    resume0
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_resume0 ( JNIEnv *env ,  struct java_lang_Thread* this)
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
JNIEXPORT void JNICALL Java_java_lang_Thread_setPriority0 ( JNIEnv *env ,  struct java_lang_Thread* this, s4 par1)
{
    if (runverbose) 
	log_text ("java_lang_Thread_setPriority0 called");

#ifdef USE_THREADS
    setPriorityThread((thread*)this, par1);
#endif
}

/*
 * Class:     java/lang/Thread
 * Method:    sleep
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_sleep (JNIEnv *env, s8 millis)
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

#ifdef USE_THREADS
    startThread((thread*)this);
#endif
}

/*
 * Class:     java/lang/Thread
 * Method:    stop0
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_stop0 ( JNIEnv *env ,  struct java_lang_Thread* this, struct java_lang_Object* par1)
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
JNIEXPORT void JNICALL Java_java_lang_Thread_suspend0 ( JNIEnv *env ,  struct java_lang_Thread* this)
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
JNIEXPORT void JNICALL Java_java_lang_Thread_yield ( JNIEnv *env  )
{
  if (runverbose)
    log_text ("java_lang_Thread_yield called");
  #ifdef USE_THREADS
	yieldThread();
  #endif
}








