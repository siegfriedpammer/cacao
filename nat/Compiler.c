/* class: java/lang/Compiler */

/*
 * Class:     java/lang/Compiler
 * Method:    command
 * Signature: (Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_Compiler_command ( JNIEnv *env ,  struct java_lang_Object* par1)
{
  log_text("Java_java_lang_Compiler_command  called");
  return NULL;
}
/*
 * Class:     java/lang/Compiler
 * Method:    compileClass
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Compiler_compileClass ( JNIEnv *env ,  struct java_lang_Class* par1)
{
  log_text("Java_java_lang_Compiler_compileClass  called");
  return 0;
}
/*
 * Class:     java/lang/Compiler
 * Method:    compileClasses
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Compiler_compileClasses ( JNIEnv *env ,  struct java_lang_String* par1)
{
  log_text("Java_java_lang_Compiler_compileClasses  called");
  return 0;
}
/*
 * Class:     java/lang/Compiler
 * Method:    disable
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Compiler_disable ( JNIEnv *env  )
{
  log_text("Java_java_lang_Compiler_disable  called");
}
/*
 * Class:     java/lang/Compiler
 * Method:    enable
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Compiler_enable ( JNIEnv *env  )
{
  log_text("Java_java_lang_Compiler_enable  called");
}
/*
 * Class:     java/lang/Compiler
 * Method:    initialize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Compiler_initialize ( JNIEnv *env  )
{
  log_text("Java_java_lang_Compiler_initialize  called");
}
/*
 * Class:     java/lang/Compiler
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Compiler_registerNatives ( JNIEnv *env  )
{
  /* empty */
}





