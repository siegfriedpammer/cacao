/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/reflect/Method */

typedef struct java_lang_reflect_Method {
   java_objectheader header;
   s4 override;
   struct java_lang_Class* clazz;
   s4 slot;
   struct java_lang_String* name;
   struct java_lang_Class* returnType;
   java_objectarray* parameterTypes;
   java_objectarray* exceptionTypes;
   s4 modifiers;
} java_lang_reflect_Method;

/*
 * Class:     java/lang/reflect/Method
 * Method:    invoke
 * Signature: (Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_Method_invoke (JNIEnv *env ,  struct java_lang_reflect_Method* this , struct java_lang_Object* par1, java_objectarray* par2);
