/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_IO_VMFILE_H
#define _JAVA_IO_VMFILE_H

/* Structure information for class: java/io/VMFile */

typedef struct java_io_VMFile {
   java_objectheader header;
} java_io_VMFile;


/*
 * Class:     java/io/VMFile
 * Method:    lastModified
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT s8 JNICALL Java_java_io_VMFile_lastModified(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/io/VMFile
 * Method:    setReadOnly
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMFile_setReadOnly(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/io/VMFile
 * Method:    create
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMFile_create(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/io/VMFile
 * Method:    list
 * Signature: (Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_io_VMFile_list(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/io/VMFile
 * Method:    renameTo
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMFile_renameTo(JNIEnv *env, jclass clazz, struct java_lang_String* par1, struct java_lang_String* par2);


/*
 * Class:     java/io/VMFile
 * Method:    length
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT s8 JNICALL Java_java_io_VMFile_length(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/io/VMFile
 * Method:    exists
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMFile_exists(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/io/VMFile
 * Method:    delete
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMFile_delete(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/io/VMFile
 * Method:    setLastModified
 * Signature: (Ljava/lang/String;J)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMFile_setLastModified(JNIEnv *env, jclass clazz, struct java_lang_String* par1, s8 par2);


/*
 * Class:     java/io/VMFile
 * Method:    mkdir
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMFile_mkdir(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/io/VMFile
 * Method:    isFile
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMFile_isFile(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/io/VMFile
 * Method:    canWrite
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMFile_canWrite(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/io/VMFile
 * Method:    canRead
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMFile_canRead(JNIEnv *env, jclass clazz, struct java_lang_String* par1);


/*
 * Class:     java/io/VMFile
 * Method:    isDirectory
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMFile_isDirectory(JNIEnv *env, jclass clazz, struct java_lang_String* par1);

#endif

