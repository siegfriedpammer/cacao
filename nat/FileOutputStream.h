/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/io/FileOutputStream */

typedef struct java_io_FileOutputStream {
   java_objectheader header;
   struct java_io_FileDescriptor* fd;
} java_io_FileOutputStream;

/*
 * Class:     java/io/FileOutputStream
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_FileOutputStream_close (JNIEnv *env ,  struct java_io_FileOutputStream* this );
/*
 * Class:     java/io/FileOutputStream
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_FileOutputStream_initIDs (JNIEnv *env );
/*
 * Class:     java/io/FileOutputStream
 * Method:    open
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_java_io_FileOutputStream_open (JNIEnv *env ,  struct java_io_FileOutputStream* this , struct java_lang_String* par1);
/*
 * Class:     java/io/FileOutputStream
 * Method:    openAppend
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_java_io_FileOutputStream_openAppend (JNIEnv *env ,  struct java_io_FileOutputStream* this , struct java_lang_String* par1);
/*
 * Class:     java/io/FileOutputStream
 * Method:    write
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_io_FileOutputStream_write (JNIEnv *env ,  struct java_io_FileOutputStream* this , s4 par1);
/*
 * Class:     java/io/FileOutputStream
 * Method:    writeBytes
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL Java_java_io_FileOutputStream_writeBytes (JNIEnv *env ,  struct java_io_FileOutputStream* this , java_bytearray* par1, s4 par2, s4 par3);
