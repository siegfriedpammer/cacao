/* class: java/lang/Throwable */

/*
 * Class:     java/lang/Throwable
 * Method:    fillInStackTrace
 * Signature: ()Ljava/lang/Throwable;
 */
JNIEXPORT struct java_lang_Throwable* JNICALL Java_java_lang_Throwable_fillInStackTrace ( JNIEnv *env ,  struct java_lang_Throwable* this)
{
	this -> detailMessage = 
	  (java_lang_String*) (javastring_new_char ("no backtrace info!") );
	return this;
}

/*
 * Class:     java/lang/Throwable
 * Method:    printStackTrace0
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_Throwable_printStackTrace0 ( JNIEnv *env ,  struct java_lang_Throwable* this, struct java_lang_Object* par1)
{
  panic("Java_java_lang_Throwable_printStackTrace0 called");
}




