/* class: java/lang/Runtime */

/*
 * Class:     java/lang/Runtime
 * Method:    execInternal
 * Signature: ([Ljava/lang/String;[Ljava/lang/String;)Ljava/lang/Process;
 */
JNIEXPORT struct java_lang_Process* JNICALL Java_java_lang_Runtime_execInternal ( JNIEnv *env ,  struct java_lang_Runtime* this, java_objectarray* par1, java_objectarray* par2)
{
  log_text("Java_java_lang_Runtime_execInternal  called");
  return NULL;
}
/*
 * Class:     java/lang/Runtime
 * Method:    exitInternal
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_exitInternal ( JNIEnv *env ,  struct java_lang_Runtime* this, s4 par1)
{
	cacao_shutdown (par1);
}
/*
 * Class:     java/lang/Runtime
 * Method:    freeMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_Runtime_freeMemory ( JNIEnv *env ,  struct java_lang_Runtime* this)
{
	log_text ("java_lang_Runtime_freeMemory called");
	return builtin_i2l (0);
}
/*
 * Class:     java/lang/Runtime
 * Method:    gc
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_gc ( JNIEnv *env ,  struct java_lang_Runtime* this)
{
	gc_call();
}
/*
 * Class:     java/lang/Runtime
 * Method:    runFinalization0
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_runFinalization0 ( JNIEnv *env  )
{
  log_text("Java_java_lang_Runtime_runFinalization0  called");
}
/*
 * Class:     java/lang/Runtime
 * Method:    runFinalizersOnExit0
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_runFinalizersOnExit0 ( JNIEnv *env ,  s4 par1)
{
  log_text("Java_java_lang_Runtime_runFinalizersOnExit0  called");
}
/*
 * Class:     java/lang/Runtime
 * Method:    totalMemory
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_Runtime_totalMemory ( JNIEnv *env ,  struct java_lang_Runtime* this)
{
	log_text ("java_lang_Runtime_totalMemory called");
	return builtin_i2l (0);
}
/*
 * Class:     java/lang/Runtime
 * Method:    traceInstructions
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_traceInstructions ( JNIEnv *env ,  struct java_lang_Runtime* this, s4 par1)
{
  log_text("Java_java_lang_Runtime_traceInstructions  called");
}
/*
 * Class:     java/lang/Runtime
 * Method:    traceMethodCalls
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_Runtime_traceMethodCalls ( JNIEnv *env ,  struct java_lang_Runtime* this, s4 par1)
{
  log_text("Java_java_lang_Runtime_traceMethodCalls  called");
}
