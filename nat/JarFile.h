/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/util/jar/JarFile */

typedef struct java_util_jar_JarFile {
   java_objectheader header;
   s8 jzfile;
   struct java_lang_String* name;
   s4 total;
   struct java_util_Vector* inflaters;
   struct java_util_jar_Manifest* man;
   struct java_util_jar_JarEntry* manEntry;
   s4 manLoaded;
   struct java_util_jar_JarVerifier* jv;
   s4 jvInitialized;
   s4 verify;
} java_util_jar_JarFile;

/*
 * Class:     java/util/jar/JarFile
 * Method:    getMetaInfEntryNames
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_util_jar_JarFile_getMetaInfEntryNames (JNIEnv *env ,  struct java_util_jar_JarFile* this );
