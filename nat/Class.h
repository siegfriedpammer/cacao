/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/Class */

typedef struct java_lang_Class {
   java_objectheader header;
} java_lang_Class;

/*
 * Class:     java/lang/Class
 * Method:    forName0
 * Signature: (Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_Class_forName0 (JNIEnv *env , struct java_lang_String* par1, s4 par2, struct java_lang_ClassLoader* par3);
/*
 * Class:     java/lang/Class
 * Method:    getClassLoader0
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT struct java_lang_ClassLoader* JNICALL Java_java_lang_Class_getClassLoader0 (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    getComponentType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_Class_getComponentType (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    getConstructor0
 * Signature: ([Ljava/lang/Class;I)Ljava/lang/reflect/Constructor;
 */
JNIEXPORT struct java_lang_reflect_Constructor* JNICALL Java_java_lang_Class_getConstructor0 (JNIEnv *env ,  struct java_lang_Class* this , java_objectarray* par1, s4 par2);
/*
 * Class:     java/lang/Class
 * Method:    getConstructors0
 * Signature: (I)[Ljava/lang/reflect/Constructor;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Class_getConstructors0 (JNIEnv *env ,  struct java_lang_Class* this , s4 par1);
/*
 * Class:     java/lang/Class
 * Method:    getDeclaredClasses0
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Class_getDeclaredClasses0 (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    getDeclaringClass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_Class_getDeclaringClass (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    getField0
 * Signature: (Ljava/lang/String;I)Ljava/lang/reflect/Field;
 */
JNIEXPORT struct java_lang_reflect_Field* JNICALL Java_java_lang_Class_getField0 (JNIEnv *env ,  struct java_lang_Class* this , struct java_lang_String* par1, s4 par2);
/*
 * Class:     java/lang/Class
 * Method:    getFields0
 * Signature: (I)[Ljava/lang/reflect/Field;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Class_getFields0 (JNIEnv *env ,  struct java_lang_Class* this , s4 par1);
/*
 * Class:     java/lang/Class
 * Method:    getInterfaces
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Class_getInterfaces (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    getMethod0
 * Signature: (Ljava/lang/String;[Ljava/lang/Class;I)Ljava/lang/reflect/Method;
 */
JNIEXPORT struct java_lang_reflect_Method* JNICALL Java_java_lang_Class_getMethod0 (JNIEnv *env ,  struct java_lang_Class* this , struct java_lang_String* par1, java_objectarray* par2, s4 par3);
/*
 * Class:     java/lang/Class
 * Method:    getMethods0
 * Signature: (I)[Ljava/lang/reflect/Method;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Class_getMethods0 (JNIEnv *env ,  struct java_lang_Class* this , s4 par1);
/*
 * Class:     java/lang/Class
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_getModifiers (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_Class_getName (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    getPrimitiveClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_Class_getPrimitiveClass (JNIEnv *env , struct java_lang_String* par1);
/*
 * Class:     java/lang/Class
 * Method:    getProtectionDomain0
 * Signature: ()Ljava/security/ProtectionDomain;
 */
JNIEXPORT struct java_security_ProtectionDomain* JNICALL Java_java_lang_Class_getProtectionDomain0 (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    getSigners
 * Signature: ()[Ljava/lang/Object;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Class_getSigners (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    getSuperclass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_Class_getSuperclass (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    isArray
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_isArray (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    isAssignableFrom
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_isAssignableFrom (JNIEnv *env ,  struct java_lang_Class* this , struct java_lang_Class* par1);
/*
 * Class:     java/lang/Class
 * Method:    isInstance
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_isInstance (JNIEnv *env ,  struct java_lang_Class* this , struct java_lang_Object* par1);
/*
 * Class:     java/lang/Class
 * Method:    isInterface
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_isInterface (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    isPrimitive
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_isPrimitive (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    newInstance0
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_Class_newInstance0 (JNIEnv *env ,  struct java_lang_Class* this );
/*
 * Class:     java/lang/Class
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Class_registerNatives (JNIEnv *env );
/*
 * Class:     java/lang/Class
 * Method:    setProtectionDomain0
 * Signature: (Ljava/security/ProtectionDomain;)V
 */
JNIEXPORT void JNICALL Java_java_lang_Class_setProtectionDomain0 (JNIEnv *env ,  struct java_lang_Class* this , struct java_security_ProtectionDomain* par1);
/*
 * Class:     java/lang/Class
 * Method:    setSigners
 * Signature: ([Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_Class_setSigners (JNIEnv *env ,  struct java_lang_Class* this , java_objectarray* par1);
