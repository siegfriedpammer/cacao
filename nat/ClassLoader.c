/* class: java/lang/ClassLoader */

/*
 * Class:     java/lang/ClassLoader
 * Method:    defineClass0
 * Signature: (Ljava/lang/String;[BII)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_ClassLoader_defineClass0 ( JNIEnv *env ,  struct java_lang_ClassLoader* this, struct java_lang_String* name, java_bytearray* buf, s4 off, s4 len)
{
    classinfo *c;

    /* call JNI-function to load the class */
    c = env->DefineClass(env, javastring_tochar((java_objectheader*) name), (jobject) this, (const jbyte *) &buf[off], len);
    use_class_as_object (c);    
    return (java_lang_Class*) c;
}

/*
 * Class:     java/lang/ClassLoader
 * Method:    findBootstrapClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_ClassLoader_findBootstrapClass ( JNIEnv *env ,  struct java_lang_ClassLoader* this, struct java_lang_String* name)
{
  /* load the class */
  return Java_java_lang_Class_forName0(env,name,0,this);
}

/*
 * Class:     java/lang/ClassLoader
 * Method:    findLoadedClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_ClassLoader_findLoadedClass ( JNIEnv *env ,  struct java_lang_ClassLoader* this, struct java_lang_String* name)
{  
  return Java_java_lang_Class_forName0(env,name,0,this);
}

/*
 * Class:     java/lang/ClassLoader
 * Method:    getCallerClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT struct java_lang_ClassLoader* JNICALL Java_java_lang_ClassLoader_getCallerClassLoader ( JNIEnv *env  )
{
  init_systemclassloader();
  return SystemClassLoader;
}

/*
 * Class:     java/lang/ClassLoader
 * Method:    resolveClass0
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_java_lang_ClassLoader_resolveClass0 ( JNIEnv *env ,  struct java_lang_ClassLoader* this, struct java_lang_Class* par1)
{
  /* class already linked, so return */
  return;
}

