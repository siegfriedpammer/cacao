/* class: java/lang/String */

/*
 * Class:     java/lang/String
 * Method:    intern
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_String_intern ( JNIEnv *env ,  struct java_lang_String* this)
{
  if (this)
    /* search table so identical strings will get identical pointers */
    return (java_lang_String*) literalstring_u2 (this->value, this->count, true);
  else
    return NULL;
}


