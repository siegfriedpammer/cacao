/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/Throwable */

typedef struct java_lang_Throwable {
   java_objectheader header;
   struct java_lang_Object* backtrace;
   struct java_lang_String* detailMessage;
} java_lang_Throwable;

/*
 * Class:     java/lang/Throwable
 * Method:    fillInStackTrace
 * Signature: ()Ljava/lang/Throwable;
 */
JNIEXPORT struct java_lang_Throwable* JNICALL Java_java_lang_Throwable_fillInStackTrace (JNIEnv *env ,  struct java_lang_Throwable* this );
/*
 * Class:     java/lang/Throwable
 * Method:    printStackTrace0
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_Throwable_printStackTrace0 (JNIEnv *env ,  struct java_lang_Throwable* this , struct java_lang_Object* par1);
