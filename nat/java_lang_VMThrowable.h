/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_LANG_VMTHROWABLE_H
#define _JAVA_LANG_VMTHROWABLE_H

/* Structure information for class: java/lang/VMThrowable */

typedef struct java_lang_VMThrowable {
   java_objectheader header;
   struct gnu_classpath_RawData* vmData;
} java_lang_VMThrowable;


/*
 * Class:     java/lang/VMThrowable
 * Method:    fillInStackTrace
 * Signature: (Ljava/lang/Throwable;)Ljava/lang/VMThrowable;
 */
JNIEXPORT struct java_lang_VMThrowable* JNICALL Java_java_lang_VMThrowable_fillInStackTrace(JNIEnv *env, jclass clazz, struct java_lang_Throwable* par1);


/*
 * Class:     java/lang/VMThrowable
 * Method:    getStackTrace
 * Signature: (Ljava/lang/Throwable;)[Ljava/lang/StackTraceElement;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMThrowable_getStackTrace(JNIEnv *env, struct java_lang_VMThrowable* this, struct java_lang_Throwable* par1);

#endif

