/* class: java/lang/SecurityManager */

/*
 * Class:     java/lang/SecurityManager
 * Method:    classDepth
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_SecurityManager_classDepth ( JNIEnv *env ,  struct java_lang_SecurityManager* this, struct java_lang_String* par1)
{
  log_text("Java_java_lang_SecurityManager_classDepth  called");
  return 0;
}

/*
 * Class:     java/lang/SecurityManager
 * Method:    classLoaderDepth0
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_SecurityManager_classLoaderDepth0 ( JNIEnv *env ,  struct java_lang_SecurityManager* this)
{
  log_text("Java_java_lang_SecurityManager_classLoaderDepth0  called");
  return 0;
}

/*
 * Class:     java/lang/SecurityManager
 * Method:    currentClassLoader0
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT struct java_lang_ClassLoader* JNICALL Java_java_lang_SecurityManager_currentClassLoader0 ( JNIEnv *env ,  struct java_lang_SecurityManager* this)
{
  init_systemclassloader();
  return SystemClassLoader;
}

/*
 * Class:     java/lang/SecurityManager
 * Method:    currentLoadedClass0
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_SecurityManager_currentLoadedClass0 ( JNIEnv *env ,  struct java_lang_SecurityManager* this)
{
    log_text("Java_java_lang_SecurityManager_currentLoadedClass0 called");
    return NULL;
}

/*
 * Class:     java/lang/SecurityManager
 * Method:    getClassContext
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_SecurityManager_getClassContext ( JNIEnv *env ,  struct java_lang_SecurityManager* this)
{
  log_text("Java_java_lang_SecurityManager_getClassContext  called");
  return NULL;
}
