/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/VMSecurityManager */

typedef struct java_lang_VMSecurityManager {
   java_objectheader header;
} java_lang_VMSecurityManager;

/*
 * Class:     java/lang/VMSecurityManager
 * Method:    getClassContext
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMSecurityManager_getClassContext (JNIEnv *env , jclass clazz );
/*
 * Class:     java/lang/VMSecurityManager
 * Method:    currentClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT struct java_lang_ClassLoader* JNICALL Java_java_lang_VMSecurityManager_currentClassLoader (JNIEnv *env , jclass clazz );
