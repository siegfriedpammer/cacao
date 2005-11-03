/* This file is machine generated, don't edit it! */

#ifndef _JAVA_LANG_VMTHREAD_H
#define _JAVA_LANG_VMTHREAD_H

/* Structure information for class: java/lang/VMThread */

typedef struct java_lang_VMThread {
   java_objectheader header;
   struct java_lang_Thread* thread;
   s4 running;
   s4 status;
   s4 priority;
   s4 restorePoint;
   struct gnu_classpath_Pointer* stackMem;
   struct gnu_classpath_Pointer* stackBase;
   struct gnu_classpath_Pointer* stackEnd;
   struct gnu_classpath_Pointer* usedStackTop;
   s8 time;
   struct java_lang_Throwable* texceptionptr;
   struct java_lang_Thread* nextlive;
   struct java_lang_Thread* next;
   s4 flags;
   struct java_lang_Object* vmdata;
} java_lang_VMThread;


/*
 * Class:     java/lang/VMThread
 * Method:    countStackFrames
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_countStackFrames(JNIEnv *env, struct java_lang_VMThread* this);


/*
 * Class:     java/lang/VMThread
 * Method:    start
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_start(JNIEnv *env, struct java_lang_VMThread* this, s8 par1);


/*
 * Class:     java/lang/VMThread
 * Method:    interrupt
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_interrupt(JNIEnv *env, struct java_lang_VMThread* this);


/*
 * Class:     java/lang/VMThread
 * Method:    isInterrupted
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_isInterrupted(JNIEnv *env, struct java_lang_VMThread* this);


/*
 * Class:     java/lang/VMThread
 * Method:    suspend
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_suspend(JNIEnv *env, struct java_lang_VMThread* this);


/*
 * Class:     java/lang/VMThread
 * Method:    resume
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_resume(JNIEnv *env, struct java_lang_VMThread* this);


/*
 * Class:     java/lang/VMThread
 * Method:    nativeSetPriority
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeSetPriority(JNIEnv *env, struct java_lang_VMThread* this, s4 par1);


/*
 * Class:     java/lang/VMThread
 * Method:    nativeStop
 * Signature: (Ljava/lang/Throwable;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeStop(JNIEnv *env, struct java_lang_VMThread* this, struct java_lang_Throwable* par1);


/*
 * Class:     java/lang/VMThread
 * Method:    currentThread
 * Signature: ()Ljava/lang/Thread;
 */
JNIEXPORT struct java_lang_Thread* JNICALL Java_java_lang_VMThread_currentThread(JNIEnv *env, jclass clazz);


/*
 * Class:     java/lang/VMThread
 * Method:    yield
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_yield(JNIEnv *env, jclass clazz);


/*
 * Class:     java/lang/VMThread
 * Method:    interrupted
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_interrupted(JNIEnv *env, jclass clazz);


/*
 * Class:     java/lang/VMThread
 * Method:    holdsLock
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_holdsLock(JNIEnv *env, jclass clazz, struct java_lang_Object* par1);

#endif

