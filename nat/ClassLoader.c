/* class: java/lang/ClassLoader */

/*
 * Class:     java/lang/ClassLoader
 * Method:    defineClass0
 * Signature: (Ljava/lang/String;[BII)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_ClassLoader_defineClass0 ( JNIEnv *env ,  struct java_lang_ClassLoader* this, struct java_lang_String* name, java_bytearray* buf, s4 off, s4 len)
{
    classinfo *c;

    log_text("Java_java_lang_ClassLoader_defineClass0 called");

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
    classinfo *c;
    bool result;
    utf *transformed_name;

    if (runverbose)
    {
	log_text("Java_java_lang_ClassLoader_findBootstrapClass called");
	log_text(javastring_tochar((java_objectheader*)name));
    }

    /* check whether the class exists */
    transformed_name = javastring_toutf(name, true);
    result = suck_start(transformed_name);
    /* suck_stop(); */

    if (!result)
    {
	log_text("could not find class");
	exceptionptr = native_new_and_init(class_java_lang_ClassNotFoundException);
	return NULL;
    }

    /* load the class */
    c = loader_load(transformed_name);

    if (c == NULL)
    {
	log_text("could not load class");
	exceptionptr = native_new_and_init(class_java_lang_ClassNotFoundException);
	return NULL;
    }

    use_class_as_object(c);

    return (java_lang_Class*)c;
}

/*
 * Class:     java/lang/ClassLoader
 * Method:    findLoadedClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_ClassLoader_findLoadedClass ( JNIEnv *env ,  struct java_lang_ClassLoader* this, struct java_lang_String* name)
{  
    classinfo *class;

    if (runverbose)
	log_text("Java_java_lang_ClassLoader_findLoadedClass called");

    class = class_get(javastring_toutf(name, true));
    if (class == NULL)
	return NULL;

    use_class_as_object(class);

    return (java_lang_Class*)class;
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

