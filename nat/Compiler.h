/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/Compiler */

typedef struct java_lang_Compiler {
   java_objectheader header;
} java_lang_Compiler;

/*
 * Class:     java/lang/Compiler
 * Method:    command
 * Signature: (Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_Compiler_command (JNIEnv *env , struct java_lang_Object* par1);
/*
 * Class:     java/lang/Compiler
 * Method:    compileClass
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Compiler_compileClass (JNIEnv *env , struct java_lang_Class* par1);
/*
 * Class:     java/lang/Compiler
 * Method:    compileClasses
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Compiler_compileClasses (JNIEnv *env , struct java_lang_String* par1);
/*
 * Class:     java/lang/Compiler
 * Method:    disable
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Compiler_disable (JNIEnv *env );
/*
 * Class:     java/lang/Compiler
 * Method:    enable
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Compiler_enable (JNIEnv *env );
/*
 * Class:     java/lang/Compiler
 * Method:    initialize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Compiler_initialize (JNIEnv *env );
/*
 * Class:     java/lang/Compiler
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Compiler_registerNatives (JNIEnv *env );
