/* class: java/io/ObjectInputStream */

/*
 * Class:     java/io/ObjectInputStream
 * Method:    allocateNewArray
 * Signature: (Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_io_ObjectInputStream_allocateNewArray ( JNIEnv *env ,  struct java_lang_Class* componenttype, s4 size)
{
  return Java_java_lang_reflect_Array_newArray(env, componenttype, size);
}

/*
 * Class:     java/io/ObjectInputStream
 * Method:    allocateNewObject
 * Signature: (Ljava/lang/Class;Ljava/lang/Class;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_io_ObjectInputStream_allocateNewObject ( JNIEnv *env ,  struct java_lang_Class* aclass, struct java_lang_Class* initclass)
{
  return Java_java_lang_Class_newInstance0(env, aclass);
}

/*
 * Class:     java/io/ObjectInputStream
 * Method:    loadClass0
 * Signature: (Ljava/lang/Class;Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_io_ObjectInputStream_loadClass0 ( JNIEnv *env ,  struct java_io_ObjectInputStream* this, struct java_lang_Class* clazz, struct java_lang_String* name)
{
  /* class paramter ignored, since there is only the system-classloader */

  return Java_java_lang_Class_forName0(env, name, 0, SystemClassLoader);
}

