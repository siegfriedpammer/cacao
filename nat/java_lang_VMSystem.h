/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_LANG_VMSYSTEM_H
#define _JAVA_LANG_VMSYSTEM_H

/* Structure information for class: java/lang/VMSystem */

typedef struct java_lang_VMSystem {
   java_objectheader header;
} java_lang_VMSystem;


/*
 * Class:     java/lang/VMSystem
 * Method:    arraycopy
 * Signature: (Ljava/lang/Object;ILjava/lang/Object;II)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMSystem_arraycopy(JNIEnv *env, jclass clazz, struct java_lang_Object* par1, s4 par2, struct java_lang_Object* par3, s4 par4, s4 par5);


/*
 * Class:     java/lang/VMSystem
 * Method:    identityHashCode
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMSystem_identityHashCode(JNIEnv *env, jclass clazz, struct java_lang_Object* par1);


/*
 * Class:     java/lang/VMSystem
 * Method:    isWordsBigEndian
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMSystem_isWordsBigEndian(JNIEnv *env, jclass clazz);


/*
 * Class:     java/lang/VMSystem
 * Method:    setIn
 * Signature: (Ljava/io/InputStream;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMSystem_setIn(JNIEnv *env, jclass clazz, struct java_io_InputStream* par1);


/*
 * Class:     java/lang/VMSystem
 * Method:    setOut
 * Signature: (Ljava/io/PrintStream;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMSystem_setOut(JNIEnv *env, jclass clazz, struct java_io_PrintStream* par1);


/*
 * Class:     java/lang/VMSystem
 * Method:    setErr
 * Signature: (Ljava/io/PrintStream;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMSystem_setErr(JNIEnv *env, jclass clazz, struct java_io_PrintStream* par1);


/*
 * Class:     java/lang/VMSystem
 * Method:    currentTimeMillis
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_VMSystem_currentTimeMillis(JNIEnv *env, jclass clazz);


/*
 * Class:     java/lang/VMSystem
 * Method:    getenv
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_VMSystem_getenv(JNIEnv *env, jclass clazz, struct java_lang_String* par1);

#endif

