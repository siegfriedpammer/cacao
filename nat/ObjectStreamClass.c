/* class: java/io/ObjectStreamClass */
 
/*
 * Class:     java/io/ObjectStreamClass
 * Method:    hasStaticInitializer
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_ObjectStreamClass_hasStaticInitializer ( JNIEnv *env ,  struct java_lang_Class* clazz)
{
    methodinfo *m;

    m = class_findmethod ((classinfo*) clazz, 
	                  utf_new_char ("<clinit>"), 
	                  utf_new_char ("()V"));
 
    if (m) return true;
    return false;
}
