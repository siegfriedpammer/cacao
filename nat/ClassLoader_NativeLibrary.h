/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/ClassLoader_NativeLibrary */

typedef struct java_lang_ClassLoader_NativeLibrary {
   java_objectheader header;
   s8 handle;
   s4 jniVersion;
   struct java_lang_Class* fromClass;
   struct java_lang_String* name;
} java_lang_ClassLoader_NativeLibrary;

/*
 * Class:     java/lang/ClassLoader_NativeLibrary
 * Method:    find
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT s8 JNICALL Java_java_lang_ClassLoader_NativeLibrary_find (JNIEnv *env ,  struct java_lang_ClassLoader_NativeLibrary* this , struct java_lang_String* par1);
/*
 * Class:     java/lang/ClassLoader_NativeLibrary
 * Method:    load
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_java_lang_ClassLoader_NativeLibrary_load (JNIEnv *env ,  struct java_lang_ClassLoader_NativeLibrary* this , struct java_lang_String* par1);
/*
 * Class:     java/lang/ClassLoader_NativeLibrary
 * Method:    unload
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_ClassLoader_NativeLibrary_unload (JNIEnv *env ,  struct java_lang_ClassLoader_NativeLibrary* this );
