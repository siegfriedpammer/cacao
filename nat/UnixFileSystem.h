/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/io/UnixFileSystem */

typedef struct java_io_UnixFileSystem {
   java_objectheader header;
   s4 slash;
   s4 colon;
} java_io_UnixFileSystem;

/*
 * Class:     java/io/UnixFileSystem
 * Method:    isAbsolute
 * Signature: (Ljava/io/File;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_isAbsolute (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* par1);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    canonicalize
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_io_UnixFileSystem_canonicalize (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_lang_String* par1);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    getBooleanAttributes0
 * Signature: (Ljava/io/File;)I
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_getBooleanAttributes0 (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* par1);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    checkAccess
 * Signature: (Ljava/io/File;Z)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_checkAccess (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* par1, s4 par2);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    getLastModifiedTime
 * Signature: (Ljava/io/File;)J
 */
JNIEXPORT s8 JNICALL Java_java_io_UnixFileSystem_getLastModifiedTime (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* par1);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    getLength
 * Signature: (Ljava/io/File;)J
 */
JNIEXPORT s8 JNICALL Java_java_io_UnixFileSystem_getLength (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* par1);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    createFileExclusively
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_createFileExclusively (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_lang_String* par1);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    delete
 * Signature: (Ljava/io/File;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_delete (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* par1);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    deleteOnExit
 * Signature: (Ljava/io/File;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_deleteOnExit (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* par1);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    list
 * Signature: (Ljava/io/File;)[Ljava/lang/String;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_io_UnixFileSystem_list (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* par1);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    createDirectory
 * Signature: (Ljava/io/File;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_createDirectory (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* par1);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    rename
 * Signature: (Ljava/io/File;Ljava/io/File;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_rename (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* par1, struct java_io_File* par2);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    setLastModifiedTime
 * Signature: (Ljava/io/File;J)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_setLastModifiedTime (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* par1, s8 par2);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    setReadOnly
 * Signature: (Ljava/io/File;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_setReadOnly (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* par1);
/*
 * Class:     java/io/UnixFileSystem
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_UnixFileSystem_initIDs (JNIEnv *env );
