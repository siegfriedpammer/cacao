/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/ClassLoader */

typedef struct java_lang_ClassLoader {
   java_objectheader header;
   s4 initialized;
   struct java_lang_ClassLoader* parent;
   struct java_util_Vector* classes;
   struct java_util_HashMap* packages;
   struct java_security_ProtectionDomain* defaultDomain;
   struct java_util_Vector* nativeLibraries;
} java_lang_ClassLoader;

/*
 * Class:     java/lang/ClassLoader
 * Method:    defineClass0
 * Signature: (Ljava/lang/String;[BII)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_ClassLoader_defineClass0 (JNIEnv *env ,  struct java_lang_ClassLoader* this , struct java_lang_String* par1, java_bytearray* par2, s4 par3, s4 par4);
/*
 * Class:     java/lang/ClassLoader
 * Method:    findBootstrapClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_ClassLoader_findBootstrapClass (JNIEnv *env ,  struct java_lang_ClassLoader* this , struct java_lang_String* par1);
/*
 * Class:     java/lang/ClassLoader
 * Method:    findLoadedClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_ClassLoader_findLoadedClass (JNIEnv *env ,  struct java_lang_ClassLoader* this , struct java_lang_String* par1);
/*
 * Class:     java/lang/ClassLoader
 * Method:    getCallerClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT struct java_lang_ClassLoader* JNICALL Java_java_lang_ClassLoader_getCallerClassLoader (JNIEnv *env );
/*
 * Class:     java/lang/ClassLoader
 * Method:    resolveClass0
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_java_lang_ClassLoader_resolveClass0 (JNIEnv *env ,  struct java_lang_ClassLoader* this , struct java_lang_Class* par1);
