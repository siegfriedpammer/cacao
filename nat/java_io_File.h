/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/io/File */

typedef struct java_io_File {
   java_objectheader header;
   struct java_lang_String* path;
} java_io_File;

/*
 * Class:     java/io/File
 * Method:    isFileInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_File_isFileInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1);
/*
 * Class:     java/io/File
 * Method:    isDirectoryInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_File_isDirectoryInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1);
/*
 * Class:     java/io/File
 * Method:    canReadInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_File_canReadInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1);
/*
 * Class:     java/io/File
 * Method:    canWriteInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_File_canWriteInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1);
/*
 * Class:     java/io/File
 * Method:    deleteInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_File_deleteInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1);
/*
 * Class:     java/io/File
 * Method:    existsInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_File_existsInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1);
/*
 * Class:     java/io/File
 * Method:    lastModifiedInternal
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT s8 JNICALL Java_java_io_File_lastModifiedInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1);
/*
 * Class:     java/io/File
 * Method:    lengthInternal
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT s8 JNICALL Java_java_io_File_lengthInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1);
/*
 * Class:     java/io/File
 * Method:    listInternal
 * Signature: (Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_io_File_listInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1);
/*
 * Class:     java/io/File
 * Method:    mkdirInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_File_mkdirInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1);
/*
 * Class:     java/io/File
 * Method:    createInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_File_createInternal (JNIEnv *env , jclass clazz , struct java_lang_String* par1);
/*
 * Class:     java/io/File
 * Method:    setReadOnlyInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_File_setReadOnlyInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1);
/*
 * Class:     java/io/File
 * Method:    renameToInternal
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_File_renameToInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1, struct java_lang_String* par2);
/*
 * Class:     java/io/File
 * Method:    setLastModifiedInternal
 * Signature: (Ljava/lang/String;J)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_File_setLastModifiedInternal (JNIEnv *env ,  struct java_io_File* this , struct java_lang_String* par1, s8 par2);
