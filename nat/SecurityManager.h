/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/SecurityManager */

typedef struct java_lang_SecurityManager {
   java_objectheader header;
   s4 inCheck;
   s4 initialized;
} java_lang_SecurityManager;

/*
 * Class:     java/lang/SecurityManager
 * Method:    classDepth
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_SecurityManager_classDepth (JNIEnv *env ,  struct java_lang_SecurityManager* this , struct java_lang_String* par1);
/*
 * Class:     java/lang/SecurityManager
 * Method:    classLoaderDepth0
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_SecurityManager_classLoaderDepth0 (JNIEnv *env ,  struct java_lang_SecurityManager* this );
/*
 * Class:     java/lang/SecurityManager
 * Method:    currentClassLoader0
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT struct java_lang_ClassLoader* JNICALL Java_java_lang_SecurityManager_currentClassLoader0 (JNIEnv *env ,  struct java_lang_SecurityManager* this );
/*
 * Class:     java/lang/SecurityManager
 * Method:    currentLoadedClass0
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_SecurityManager_currentLoadedClass0 (JNIEnv *env ,  struct java_lang_SecurityManager* this );
/*
 * Class:     java/lang/SecurityManager
 * Method:    getClassContext
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_SecurityManager_getClassContext (JNIEnv *env ,  struct java_lang_SecurityManager* this );
