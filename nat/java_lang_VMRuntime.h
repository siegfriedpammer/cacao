/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_LANG_VMRUNTIME_H
#define _JAVA_LANG_VMRUNTIME_H

/* Structure information for class: java/lang/VMRuntime */

typedef struct java_lang_VMRuntime {
   java_objectheader header;
} java_lang_VMRuntime;


/*
 * Class:     java/lang/VMRuntime
 * Method:    availableProcessors
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMRuntime_availableProcessors(JNIEnv *env, jclass clazz);


/*
 * Class:     java/lang/VMRuntime
 * Method:    freeMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_VMRuntime_freeMemory(JNIEnv *env, jclass clazz);


/*
 * Class:     java/lang/VMRuntime
 * Method:    totalMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_VMRuntime_totalMemory(JNIEnv *env, jclass clazz);


/*
 * Class:     java/lang/VMRuntime
 * Method:    maxMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_VMRuntime_maxMemory(JNIEnv *env, jclass clazz);


/*
 * Class:     java/lang/VMRuntime
 * Method:    gc
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_gc(JNIEnv *env, jclass clazz);


/*
 * Class:     java/lang/VMRuntime
 * Method:    runFinalization
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalization(JNIEnv *env, jclass clazz);


/*
 * Class:     java/lang/VMRuntime
 * Method:    runFinalizationForExit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalizationForExit(JNIEnv *env, jclass clazz);


/*
 * Class:     java/lang/VMRuntime
 * Method:    traceInstructions
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_traceInstructions(JNIEnv *env, jclass clazz, s4 par1);


/*
 * Class:     java/lang/VMRuntime
 * Method:    traceMethodCalls
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_traceMethodCalls(JNIEnv *env, jclass clazz, s4 par1);


/*
 * Class:     java/lang/VMRuntime
 * Method:    runFinalizersOnExit
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalizersOnExit(JNIEnv *env, jclass clazz, s4 par1);


/*
 * Class:     java/lang/VMRuntime
 * Method:    exit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_exit(JNIEnv *env, jclass clazz, s4 par1);


/*
 * Class:     java/lang/VMRuntime
 * Method:    nativeLoad
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMRuntime_nativeLoad(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/lang/VMRuntime
 * Method:    nativeGetLibname
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_VMRuntime_nativeGetLibname(JNIEnv *env, jclass clazz, struct java_lang_String* par1, struct java_lang_String* par2);


/*
 * Class:     java/lang/VMRuntime
 * Method:    insertSystemProperties
 * Signature: (Ljava/util/Properties;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_insertSystemProperties(JNIEnv *env, jclass clazz, struct java_util_Properties* par1);

#endif

