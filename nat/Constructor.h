/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/reflect/Constructor */

typedef struct java_lang_reflect_Constructor {
   java_objectheader header;
   s4 override;
   struct java_lang_Class* clazz;
   s4 slot;
   java_objectarray* parameterTypes;
   java_objectarray* exceptionTypes;
   s4 modifiers;
} java_lang_reflect_Constructor;

/*
 * Class:     java/lang/reflect/Constructor
 * Method:    newInstance
 * Signature: ([Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_Constructor_newInstance (JNIEnv *env ,  struct java_lang_reflect_Constructor* this , java_objectarray* par1);
