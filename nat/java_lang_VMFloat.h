/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_LANG_VMFLOAT_H
#define _JAVA_LANG_VMFLOAT_H

/* Structure information for class: java/lang/VMFloat */

typedef struct java_lang_VMFloat {
   java_objectheader header;
} java_lang_VMFloat;


/*
 * Class:     java/lang/VMFloat
 * Method:    floatToIntBits
 * Signature: (F)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMFloat_floatToIntBits(JNIEnv *env, jclass clazz, float par1);


/*
 * Class:     java/lang/VMFloat
 * Method:    floatToRawIntBits
 * Signature: (F)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMFloat_floatToRawIntBits(JNIEnv *env, jclass clazz, float par1);


/*
 * Class:     java/lang/VMFloat
 * Method:    intBitsToFloat
 * Signature: (I)F
 */
JNIEXPORT float JNICALL Java_java_lang_VMFloat_intBitsToFloat(JNIEnv *env, jclass clazz, s4 par1);

#endif

