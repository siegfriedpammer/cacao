/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_LANG_RUNTIME_H
#define _JAVA_LANG_RUNTIME_H

/* Structure information for class: java/lang/Runtime */

typedef struct java_lang_Runtime {
   java_objectheader header;
   java_objectarray* libpath;
   struct java_lang_Thread* exitSequence;
   struct java_util_Set* shutdownHooks;
} java_lang_Runtime;


/*
 * Class:     java/lang/Runtime
 * Method:    availableProcessors
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_Runtime_availableProcessors(JNIEnv *env, struct java_lang_Runtime* this);


/*
 * Class:     java/lang/Runtime
 * Method:    freeMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_Runtime_freeMemory(JNIEnv *env, struct java_lang_Runtime* this);


/*
 * Class:     java/lang/Runtime
 * Method:    totalMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_Runtime_totalMemory(JNIEnv *env, struct java_lang_Runtime* this);


/*
 * Class:     java/lang/Runtime
 * Method:    maxMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_Runtime_maxMemory(JNIEnv *env, struct java_lang_Runtime* this);


/*
 * Class:     java/lang/Runtime
 * Method:    gc
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_gc(JNIEnv *env, struct java_lang_Runtime* this);


/*
 * Class:     java/lang/Runtime
 * Method:    runFinalization
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_runFinalization(JNIEnv *env, struct java_lang_Runtime* this);


/*
 * Class:     java/lang/Runtime
 * Method:    traceInstructions
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_traceInstructions(JNIEnv *env, struct java_lang_Runtime* this, s4 par1);


/*
 * Class:     java/lang/Runtime
 * Method:    traceMethodCalls
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_traceMethodCalls(JNIEnv *env, struct java_lang_Runtime* this, s4 par1);


/*
 * Class:     java/lang/Runtime
 * Method:    runFinalizersOnExitInternal
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_runFinalizersOnExitInternal(JNIEnv *env, jclass clazz, s4 par1);


/*
 * Class:     java/lang/Runtime
 * Method:    exitInternal
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_exitInternal(JNIEnv *env, struct java_lang_Runtime* this, s4 par1);


/*
 * Class:     java/lang/Runtime
 * Method:    nativeLoad
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_Runtime_nativeLoad(JNIEnv *env, struct java_lang_Runtime* this, struct java_lang_String* par1);


/*
 * Class:     java/lang/Runtime
 * Method:    nativeGetLibname
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_Runtime_nativeGetLibname(JNIEnv *env, jclass clazz, struct java_lang_String* par1, struct java_lang_String* par2);


/*
 * Class:     java/lang/Runtime
 * Method:    execInternal
 * Signature: ([Ljava/lang/String;[Ljava/lang/String;Ljava/io/File;)Ljava/lang/Process;
 */
JNIEXPORT struct java_lang_Process* JNICALL Java_java_lang_Runtime_execInternal(JNIEnv *env, struct java_lang_Runtime* this, java_objectarray* par1, java_objectarray* par2, struct java_io_File* par3);


/*
 * Class:     java/lang/Runtime
 * Method:    insertSystemProperties
 * Signature: (Ljava/util/Properties;)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_insertSystemProperties(JNIEnv *env, jclass clazz, struct java_util_Properties* par1);

#endif

