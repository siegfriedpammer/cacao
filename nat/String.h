/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/String */

typedef struct java_lang_String {
   java_objectheader header;
   java_chararray* value;
   s4 offset;
   s4 count;
} java_lang_String;

/*
 * Class:     java/lang/String
 * Method:    intern
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_String_intern (JNIEnv *env ,  struct java_lang_String* this );
