/* class: java/lang/reflect/Method */

/*
 * Class:     java/lang/reflect/Method
 * Method:    invoke
 * Signature: (Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_Method_invoke ( JNIEnv *env ,  struct java_lang_reflect_Method* this, struct java_lang_Object* par1, java_objectarray* par2)
{
  log_text("Java_java_lang_reflect_Method_invoke called");
}
