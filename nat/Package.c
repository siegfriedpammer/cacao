/* class: java/lang/Package */

/*
 * Class:     java/lang/Package
 * Method:    getSystemPackage0
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_Package_getSystemPackage0 ( JNIEnv *env ,  struct java_lang_String* par1)
{
  log_text("Java_java_lang_Package_getSystemPackage0  called");
}

/*
 * Class:     java/lang/Package
 * Method:    getSystemPackages0
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Package_getSystemPackages0 ( JNIEnv *env  )
{
  log_text("Java_java_lang_Package_getSystemPackages0  called");
}
