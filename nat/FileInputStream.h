/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/io/FileInputStream */

typedef struct java_io_FileInputStream {
   java_objectheader header;
   struct java_io_FileDescriptor* fd;
} java_io_FileInputStream;

/*
 * Class:     java/io/FileInputStream
 * Method:    available
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_io_FileInputStream_available (JNIEnv *env ,  struct java_io_FileInputStream* this );
/*
 * Class:     java/io/FileInputStream
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_FileInputStream_close (JNIEnv *env ,  struct java_io_FileInputStream* this );
/*
 * Class:     java/io/FileInputStream
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_FileInputStream_initIDs (JNIEnv *env );
/*
 * Class:     java/io/FileInputStream
 * Method:    open
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_java_io_FileInputStream_open (JNIEnv *env ,  struct java_io_FileInputStream* this , struct java_lang_String* par1);
/*
 * Class:     java/io/FileInputStream
 * Method:    read
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_io_FileInputStream_read (JNIEnv *env ,  struct java_io_FileInputStream* this );
/*
 * Class:     java/io/FileInputStream
 * Method:    readBytes
 * Signature: ([BII)I
 */
JNIEXPORT s4 JNICALL Java_java_io_FileInputStream_readBytes (JNIEnv *env ,  struct java_io_FileInputStream* this , java_bytearray* par1, s4 par2, s4 par3);
/*
 * Class:     java/io/FileInputStream
 * Method:    skip
 * Signature: (J)J
 */
JNIEXPORT s8 JNICALL Java_java_io_FileInputStream_skip (JNIEnv *env ,  struct java_io_FileInputStream* this , s8 par1);
