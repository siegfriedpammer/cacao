/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/Object */

typedef struct java_lang_Object {
   java_objectheader header;
} java_lang_Object;

/*
 * Class:     java/lang/Object
 * Method:    clone
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_Object_clone (JNIEnv *env ,  struct java_lang_Object* this );
/*
 * Class:     java/lang/Object
 * Method:    getClass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_Object_getClass (JNIEnv *env ,  struct java_lang_Object* this );
/*
 * Class:     java/lang/Object
 * Method:    hashCode
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_Object_hashCode (JNIEnv *env ,  struct java_lang_Object* this );
/*
 * Class:     java/lang/Object
 * Method:    notify
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Object_notify (JNIEnv *env ,  struct java_lang_Object* this );
/*
 * Class:     java/lang/Object
 * Method:    notifyAll
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Object_notifyAll (JNIEnv *env ,  struct java_lang_Object* this );
/*
 * Class:     java/lang/Object
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Object_registerNatives (JNIEnv *env );
/*
 * Class:     java/lang/Object
 * Method:    wait
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_Object_wait (JNIEnv *env ,  struct java_lang_Object* this , s8 par1);
