/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/Thread */

typedef struct java_lang_Thread {
   java_objectheader header;
   java_chararray* name;
   s4 priority;
   struct java_lang_Thread* threadQ;
   s8 eetop;
   s4 single_step;
   s4 daemon;
   s4 stillborn;
   struct java_lang_Runnable* target;
   struct java_lang_ThreadGroup* group;
   struct java_lang_ClassLoader* contextClassLoader;
   struct java_security_AccessControlContext* inheritedAccessControlContext;
   struct java_lang_InheritableThreadLocal_Entry* values;
} java_lang_Thread;

/*
 * Class:     java/lang/Thread
 * Method:    countStackFrames
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_countStackFrames (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    currentThread
 * Signature: ()Ljava/lang/Thread;
 */
JNIEXPORT struct java_lang_Thread* JNICALL Java_java_lang_Thread_currentThread (JNIEnv *env );
/*
 * Class:     java/lang/Thread
 * Method:    interrupt0
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_interrupt0 (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    isAlive
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_isAlive (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    isInterrupted
 * Signature: (Z)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Thread_isInterrupted (JNIEnv *env ,  struct java_lang_Thread* this , s4 par1);
/*
 * Class:     java/lang/Thread
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_registerNatives (JNIEnv *env );
/*
 * Class:     java/lang/Thread
 * Method:    resume0
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_resume0 (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    setPriority0
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_setPriority0 (JNIEnv *env ,  struct java_lang_Thread* this , s4 par1);
/*
 * Class:     java/lang/Thread
 * Method:    sleep
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_sleep (JNIEnv *env , s8 par1);
/*
 * Class:     java/lang/Thread
 * Method:    start
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_start (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    stop0
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_stop0 (JNIEnv *env ,  struct java_lang_Thread* this , struct java_lang_Object* par1);
/*
 * Class:     java/lang/Thread
 * Method:    suspend0
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_suspend0 (JNIEnv *env ,  struct java_lang_Thread* this );
/*
 * Class:     java/lang/Thread
 * Method:    yield
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Thread_yield (JNIEnv *env );
