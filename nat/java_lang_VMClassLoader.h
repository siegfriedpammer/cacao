/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_LANG_VMCLASSLOADER_H
#define _JAVA_LANG_VMCLASSLOADER_H

/* Structure information for class: java/lang/VMClassLoader */

typedef struct java_lang_VMClassLoader {
   java_objectheader header;
} java_lang_VMClassLoader;


/*
 * Class:     java/lang/VMClassLoader
 * Method:    defineClass
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/String;[BII)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClassLoader_defineClass(JNIEnv *env, jclass clazz, struct java_lang_ClassLoader* par1, struct java_lang_String* par2, java_bytearray* par3, s4 par4, s4 par5);


/*
 * Class:     java/lang/VMClassLoader
 * Method:    resolveClass
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClassLoader_resolveClass(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClassLoader
 * Method:    loadClass
 * Signature: (Ljava/lang/String;Z)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClassLoader_loadClass(JNIEnv *env, jclass clazz, struct java_lang_String* par1, s4 par2);


/*
 * Class:     java/lang/VMClassLoader
 * Method:    getPrimitiveClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClassLoader_getPrimitiveClass(JNIEnv *env, jclass clazz, struct java_lang_String* par1);

#endif

