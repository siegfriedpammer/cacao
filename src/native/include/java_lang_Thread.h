/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/Thread */

typedef struct java_lang_Thread {
   java_objectheader header;
   struct java_lang_ThreadGroup* group;
   struct java_lang_Runnable* toRun;
   struct java_lang_String* name;
   s8 PrivateInfo;
   struct java_lang_Thread* next;
   s4 daemon;
   s4 priority;
   struct java_lang_ClassLoader* contextClassLoader;
} java_lang_Thread;

/*
 * Class:     java/lang/Thread
 * Method:    currentThread
 * Signature: ()Ljava/lang/Thread;
 */
JNIEXPORT struct java_lang_Thread* JNICALL Java_java_lang_Thread_currentThread (JNIEnv *env , jclass clazz );
/*
 * Class:     java/lang/Thread
 * Method:    yield
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_yield (JNIEnv *env , jclass clazz );
/*
 * Class:     java/lang/Thread
 * Method:    sleep
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_sleep (JNIEnv *env , jclass clazz , s8 par1, s4 par2);
/*
 * Class:     java/lang/Thread
 * Method:    start
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_start (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    interrupted
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_interrupted (JNIEnv *env , jclass clazz );
/*
 * Class:     java/lang/Thread
 * Method:    isInterrupted
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_isInterrupted (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    isAlive
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_isAlive (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    countStackFrames
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_countStackFrames (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    holdsLock
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_holdsLock (JNIEnv *env , jclass clazz , struct java_lang_Object* par1);
/*
 * Class:     java/lang/Thread
 * Method:    nativeInit
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_nativeInit (JNIEnv *env ,  struct java_lang_Thread* this , s8 par1);
/*
 * Class:     java/lang/Thread
 * Method:    nativeStop
 * Signature: (Ljava/lang/Throwable;)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_nativeStop (JNIEnv *env ,  struct java_lang_Thread* this , struct java_lang_Throwable* par1);
/*
 * Class:     java/lang/Thread
 * Method:    nativeInterrupt
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_nativeInterrupt (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    nativeSuspend
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_nativeSuspend (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    nativeResume
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_nativeResume (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    nativeSetPriority
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_nativeSetPriority (JNIEnv *env ,  struct java_lang_Thread* this , s4 par1);
