/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_LANG_VMCLASS_H
#define _JAVA_LANG_VMCLASS_H

/* Structure information for class: java/lang/VMClass */

typedef struct java_lang_VMClass {
   java_objectheader header;
} java_lang_VMClass;


/*
 * Class:     java/lang/VMClass
 * Method:    isInstance
 * Signature: (Ljava/lang/Class;Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isInstance(JNIEnv *env, jclass clazz, struct java_lang_Class* par1, struct java_lang_Object* par2);


/*
 * Class:     java/lang/VMClass
 * Method:    isAssignableFrom
 * Signature: (Ljava/lang/Class;Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isAssignableFrom(JNIEnv *env, jclass clazz, struct java_lang_Class* par1, struct java_lang_Class* par2);


/*
 * Class:     java/lang/VMClass
 * Method:    isInterface
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isInterface(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    isPrimitive
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isPrimitive(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    getName
 * Signature: (Ljava/lang/Class;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_VMClass_getName(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    getSuperclass
 * Signature: (Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClass_getSuperclass(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    getInterfaces
 * Signature: (Ljava/lang/Class;)[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getInterfaces(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    getComponentType
 * Signature: (Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClass_getComponentType(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    getModifiers
 * Signature: (Ljava/lang/Class;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_getModifiers(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaringClass
 * Signature: (Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClass_getDeclaringClass(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredClasses
 * Signature: (Ljava/lang/Class;Z)[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredClasses(JNIEnv *env, jclass clazz, struct java_lang_Class* par1, s4 par2);


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredFields
 * Signature: (Ljava/lang/Class;Z)[Ljava/lang/reflect/Field;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredFields(JNIEnv *env, jclass clazz, struct java_lang_Class* par1, s4 par2);


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredMethods
 * Signature: (Ljava/lang/Class;Z)[Ljava/lang/reflect/Method;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredMethods(JNIEnv *env, jclass clazz, struct java_lang_Class* par1, s4 par2);


/*
 * Class:     java/lang/VMClass
 * Method:    getDeclaredConstructors
 * Signature: (Ljava/lang/Class;Z)[Ljava/lang/reflect/Constructor;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredConstructors(JNIEnv *env, jclass clazz, struct java_lang_Class* par1, s4 par2);


/*
 * Class:     java/lang/VMClass
 * Method:    getClassLoader
 * Signature: (Ljava/lang/Class;)Ljava/lang/ClassLoader;
 */
JNIEXPORT struct java_lang_ClassLoader* JNICALL Java_java_lang_VMClass_getClassLoader(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    forName
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClass_forName(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    isArray
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isArray(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    initialize
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_initialize(JNIEnv *env, jclass clazz, struct java_lang_Class* par1);


/*
 * Class:     java/lang/VMClass
 * Method:    loadArrayClass
 * Signature: (Ljava/lang/String;Ljava/lang/ClassLoader;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClass_loadArrayClass(JNIEnv *env, jclass clazz, struct java_lang_String* par1, struct java_lang_ClassLoader* par2);


/*
 * Class:     java/lang/VMClass
 * Method:    throwException
 * Signature: (Ljava/lang/Throwable;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_throwException(JNIEnv *env, jclass clazz, struct java_lang_Throwable* par1);

#endif

