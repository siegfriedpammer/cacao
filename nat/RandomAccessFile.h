/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/io/RandomAccessFile */

typedef struct java_io_RandomAccessFile {
   java_objectheader header;
   struct java_io_FileDescriptor* fd;
} java_io_RandomAccessFile;

/*
 * Class:     java/io/RandomAccessFile
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_close (JNIEnv *env ,  struct java_io_RandomAccessFile* this );
/*
 * Class:     java/io/RandomAccessFile
 * Method:    getFilePointer
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_io_RandomAccessFile_getFilePointer (JNIEnv *env ,  struct java_io_RandomAccessFile* this );
/*
 * Class:     java/io/RandomAccessFile
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_initIDs (JNIEnv *env );
/*
 * Class:     java/io/RandomAccessFile
 * Method:    length
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_io_RandomAccessFile_length (JNIEnv *env ,  struct java_io_RandomAccessFile* this );
/*
 * Class:     java/io/RandomAccessFile
 * Method:    open
 * Signature: (Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_open (JNIEnv *env ,  struct java_io_RandomAccessFile* this , struct java_lang_String* par1, s4 par2);
/*
 * Class:     java/io/RandomAccessFile
 * Method:    read
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_io_RandomAccessFile_read (JNIEnv *env ,  struct java_io_RandomAccessFile* this );
/*
 * Class:     java/io/RandomAccessFile
 * Method:    readBytes
 * Signature: ([BII)I
 */
JNIEXPORT s4 JNICALL Java_java_io_RandomAccessFile_readBytes (JNIEnv *env ,  struct java_io_RandomAccessFile* this , java_bytearray* par1, s4 par2, s4 par3);
/*
 * Class:     java/io/RandomAccessFile
 * Method:    seek
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_seek (JNIEnv *env ,  struct java_io_RandomAccessFile* this , s8 par1);
/*
 * Class:     java/io/RandomAccessFile
 * Method:    setLength
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_setLength (JNIEnv *env ,  struct java_io_RandomAccessFile* this , s8 par1);
/*
 * Class:     java/io/RandomAccessFile
 * Method:    write
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_write (JNIEnv *env ,  struct java_io_RandomAccessFile* this , s4 par1);
/*
 * Class:     java/io/RandomAccessFile
 * Method:    writeBytes
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_writeBytes (JNIEnv *env ,  struct java_io_RandomAccessFile* this , java_bytearray* par1, s4 par2, s4 par3);
