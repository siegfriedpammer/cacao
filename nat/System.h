/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/System */

typedef struct java_lang_System {
   java_objectheader header;
} java_lang_System;

/*
 * Class:     java/lang/System
 * Method:    arraycopy
 * Signature: (Ljava/lang/Object;ILjava/lang/Object;II)V
 */
JNIEXPORT void JNICALL Java_java_lang_System_arraycopy (JNIEnv *env , struct java_lang_Object* par1, s4 par2, struct java_lang_Object* par3, s4 par4, s4 par5);
/*
 * Class:     java/lang/System
 * Method:    currentTimeMillis
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_System_currentTimeMillis (JNIEnv *env );
/*
 * Class:     java/lang/System
 * Method:    getCallerClass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_System_getCallerClass (JNIEnv *env );
/*
 * Class:     java/lang/System
 * Method:    identityHashCode
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_System_identityHashCode (JNIEnv *env , struct java_lang_Object* par1);
/*
 * Class:     java/lang/System
 * Method:    initProperties
 * Signature: (Ljava/util/Properties;)Ljava/util/Properties;
 */
JNIEXPORT struct java_util_Properties* JNICALL Java_java_lang_System_initProperties (JNIEnv *env , struct java_util_Properties* par1);
/*
 * Class:     java/lang/System
 * Method:    mapLibraryName
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_System_mapLibraryName (JNIEnv *env , struct java_lang_String* par1);
/*
 * Class:     java/lang/System
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_System_registerNatives (JNIEnv *env );
/*
 * Class:     java/lang/System
 * Method:    setErr0
 * Signature: (Ljava/io/PrintStream;)V
 */
JNIEXPORT void JNICALL Java_java_lang_System_setErr0 (JNIEnv *env , struct java_io_PrintStream* par1);
/*
 * Class:     java/lang/System
 * Method:    setIn0
 * Signature: (Ljava/io/InputStream;)V
 */
JNIEXPORT void JNICALL Java_java_lang_System_setIn0 (JNIEnv *env , struct java_io_InputStream* par1);
/*
 * Class:     java/lang/System
 * Method:    setOut0
 * Signature: (Ljava/io/PrintStream;)V
 */
JNIEXPORT void JNICALL Java_java_lang_System_setOut0 (JNIEnv *env , struct java_io_PrintStream* par1);
