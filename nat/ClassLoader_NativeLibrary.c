/* class: java/lang/ClassLoader$NativeLibrary */

/*
 * Class:     java/lang/ClassLoader$NativeLibrary
 * Method:    find
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT s8 JNICALL Java_java_lang_ClassLoader_NativeLibrary_find (JNIEnv *env ,  struct java_lang_ClassLoader_NativeLibrary* this , struct java_lang_String* par1)
{
  if (verbose)
      log_text("Java_java_lang_ClassLoader_NativeLibrary_find called");
}

/*
 * Class:     java/lang/ClassLoader$NativeLibrary
 * Method:    load
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_java_lang_ClassLoader_NativeLibrary_load (JNIEnv *env ,  struct java_lang_ClassLoader_NativeLibrary* this , struct java_lang_String* par1)
{
      /* only initialize instance-fields, since native-functions are not loaded dynamically */
      this->jniVersion = env->GetVersion(env);
      this->handle     = 1;  /* not 0, so it looks valid */
}

/*
 * Class:     java/lang/ClassLoader$NativeLibrary
 * Method:    unload
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_ClassLoader_NativeLibrary_unload (JNIEnv *env ,  struct java_lang_ClassLoader_NativeLibrary* this )
{
  if (verbose)
      log_text("Java_java_lang_ClassLoader_NativeLibrary_unload called");
}







