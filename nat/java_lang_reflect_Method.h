/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/reflect/Method */

typedef struct java_lang_reflect_Method {
   java_objectheader header;
   s4 flag;
   struct java_lang_Class* declaringClass;
   struct java_lang_String* name;
   s4 slot;
} java_lang_reflect_Method;

/*
 * Class:     java/lang/reflect/Method
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Method_getModifiers (JNIEnv *env ,  struct java_lang_reflect_Method* this );
/*
 * Class:     java/lang/reflect/Method
 * Method:    getReturnType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_reflect_Method_getReturnType (JNIEnv *env ,  struct java_lang_reflect_Method* this );
/*
 * Class:     java/lang/reflect/Method
 * Method:    getParameterTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_reflect_Method_getParameterTypes (JNIEnv *env ,  struct java_lang_reflect_Method* this );
/*
 * Class:     java/lang/reflect/Method
 * Method:    getExceptionTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_reflect_Method_getExceptionTypes (JNIEnv *env ,  struct java_lang_reflect_Method* this );
/*
 * Class:     java/lang/reflect/Method
 * Method:    invokeNative
 * Signature: (Ljava/lang/Object;[Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_Method_invokeNative (JNIEnv *env ,  struct java_lang_reflect_Method* this , struct java_lang_Object* par1, java_objectarray* par2, struct java_lang_Class* par3, s4 par4);
