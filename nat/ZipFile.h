/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/util/zip/ZipFile */

typedef struct java_util_zip_ZipFile {
   java_objectheader header;
   s8 jzfile;
   struct java_lang_String* name;
   s4 total;
   struct java_util_Vector* inflaters;
} java_util_zip_ZipFile;

/*
 * Class:     java/util/zip/ZipFile
 * Method:    close
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_util_zip_ZipFile_close (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/ZipFile
 * Method:    getCSize
 * Signature: (J)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_ZipFile_getCSize (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/ZipFile
 * Method:    getEntry
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT s8 JNICALL Java_java_util_zip_ZipFile_getEntry (JNIEnv *env , s8 par1, struct java_lang_String* par2);
/*
 * Class:     java/util/zip/ZipFile
 * Method:    getMethod
 * Signature: (J)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_ZipFile_getMethod (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/ZipFile
 * Method:    getNextEntry
 * Signature: (JI)J
 */
JNIEXPORT s8 JNICALL Java_java_util_zip_ZipFile_getNextEntry (JNIEnv *env , s8 par1, s4 par2);
/*
 * Class:     java/util/zip/ZipFile
 * Method:    getTotal
 * Signature: (J)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_ZipFile_getTotal (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/ZipFile
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_util_zip_ZipFile_initIDs (JNIEnv *env );
/*
 * Class:     java/util/zip/ZipFile
 * Method:    open
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT s8 JNICALL Java_java_util_zip_ZipFile_open (JNIEnv *env , struct java_lang_String* par1);
/*
 * Class:     java/util/zip/ZipFile
 * Method:    read
 * Signature: (JJI[BII)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_ZipFile_read (JNIEnv *env , s8 par1, s8 par2, s4 par3, java_bytearray* par4, s4 par5, s4 par6);
