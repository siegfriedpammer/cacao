/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/reflect/Constructor */

typedef struct java_lang_reflect_Constructor {
   java_objectheader header;
   s4 flag;
   struct java_lang_Class* clazz;
   s4 slot;
   java_objectarray* parameterTypes;
   java_objectarray* exceptionTypes;
} java_lang_reflect_Constructor;

/*
 * Class:     java/lang/reflect/Constructor
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Constructor_getModifiers (JNIEnv *env ,  struct java_lang_reflect_Constructor* this );
/*
 * Class:     java/lang/reflect/Constructor
 * Method:    constructNative
 * Signature: ([Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_Constructor_constructNative (JNIEnv *env ,  struct java_lang_reflect_Constructor* this , java_objectarray* par1, struct java_lang_Class* par2, s4 par3);
