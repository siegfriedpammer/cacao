/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_LANG_VMDOUBLE_H
#define _JAVA_LANG_VMDOUBLE_H

/* Structure information for class: java/lang/VMDouble */

typedef struct java_lang_VMDouble {
   java_objectheader header;
} java_lang_VMDouble;


/*
 * Class:     java/lang/VMDouble
 * Method:    doubleToLongBits
 * Signature: (D)J
 */
JNIEXPORT s8 JNICALL Java_java_lang_VMDouble_doubleToLongBits(JNIEnv *env, jclass clazz, double par1);


/*
 * Class:     java/lang/VMDouble
 * Method:    doubleToRawLongBits
 * Signature: (D)J
 */
JNIEXPORT s8 JNICALL Java_java_lang_VMDouble_doubleToRawLongBits(JNIEnv *env, jclass clazz, double par1);


/*
 * Class:     java/lang/VMDouble
 * Method:    longBitsToDouble
 * Signature: (J)D
 */
JNIEXPORT double JNICALL Java_java_lang_VMDouble_longBitsToDouble(JNIEnv *env, jclass clazz, s8 par1);

#endif

