/* class: java/lang/SecurityManager */



/*
 * Class:     java/lang/SecurityManager
 * Method:    currentClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT struct java_lang_ClassLoader* JNICALL Java_java_lang_VMSecurityManager_currentClassLoader ( JNIEnv *env, jclass clazz)
{
  init_systemclassloader();
  return SystemClassLoader;
}


/*
 * Class:     java/lang/SecurityManager
 * Method:    getClassContext
 * Signature: ()[Ljava/lang/Class;
 */
/* XXX delete */
#if 0
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMSecurityManager_getClassContext ( JNIEnv *env ,jclass clazz)
{
  log_text("Java_java_lang_VMSecurityManager_getClassContext  called");
#warning return something more usefull here
  return builtin_anewarray(0, class_java_lang_Class);
}
#endif
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMSecurityManager_getClassContext ( JNIEnv *env ,jclass clazz)
{
  log_text("Java_java_lang_VMSecurityManager_getClassContext  called");
#warning return something more usefull here
  /* XXX should use vftbl directly */
  return builtin_newarray(0,class_array_of(class_java_lang_Class)->vftbl);
}
