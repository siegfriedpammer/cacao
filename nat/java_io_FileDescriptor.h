/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/io/FileDescriptor */

typedef struct java_io_FileDescriptor {
   java_objectheader header;
   s8 nativeFd;
} java_io_FileDescriptor;

/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_FileDescriptor_nativeInit (JNIEnv *env , jclass clazz );
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeOpen
 * Signature: (Ljava/lang/String;I)J
 */
JNIEXPORT s8 JNICALL Java_java_io_FileDescriptor_nativeOpen (JNIEnv *env ,  struct java_io_FileDescriptor* this , struct java_lang_String* par1, s4 par2);
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeClose
 * Signature: (J)J
 */
JNIEXPORT s8 JNICALL Java_java_io_FileDescriptor_nativeClose (JNIEnv *env ,  struct java_io_FileDescriptor* this , s8 par1);
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeWriteByte
 * Signature: (JI)J
 */
JNIEXPORT s8 JNICALL Java_java_io_FileDescriptor_nativeWriteByte (JNIEnv *env ,  struct java_io_FileDescriptor* this , s8 par1, s4 par2);
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeWriteBuf
 * Signature: (J[BII)J
 */
JNIEXPORT s8 JNICALL Java_java_io_FileDescriptor_nativeWriteBuf (JNIEnv *env ,  struct java_io_FileDescriptor* this , s8 par1, java_bytearray* par2, s4 par3, s4 par4);
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeReadByte
 * Signature: (J)I
 */
JNIEXPORT s4 JNICALL Java_java_io_FileDescriptor_nativeReadByte (JNIEnv *env ,  struct java_io_FileDescriptor* this , s8 par1);
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeReadBuf
 * Signature: (J[BII)I
 */
JNIEXPORT s4 JNICALL Java_java_io_FileDescriptor_nativeReadBuf (JNIEnv *env ,  struct java_io_FileDescriptor* this , s8 par1, java_bytearray* par2, s4 par3, s4 par4);
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeAvailable
 * Signature: (J)I
 */
JNIEXPORT s4 JNICALL Java_java_io_FileDescriptor_nativeAvailable (JNIEnv *env ,  struct java_io_FileDescriptor* this , s8 par1);
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeSeek
 * Signature: (JJIZ)J
 */
JNIEXPORT s8 JNICALL Java_java_io_FileDescriptor_nativeSeek (JNIEnv *env ,  struct java_io_FileDescriptor* this , s8 par1, s8 par2, s4 par3, s4 par4);
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeGetFilePointer
 * Signature: (J)J
 */
JNIEXPORT s8 JNICALL Java_java_io_FileDescriptor_nativeGetFilePointer (JNIEnv *env ,  struct java_io_FileDescriptor* this , s8 par1);
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeGetLength
 * Signature: (J)J
 */
JNIEXPORT s8 JNICALL Java_java_io_FileDescriptor_nativeGetLength (JNIEnv *env ,  struct java_io_FileDescriptor* this , s8 par1);
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeSetLength
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_java_io_FileDescriptor_nativeSetLength (JNIEnv *env ,  struct java_io_FileDescriptor* this , s8 par1, s8 par2);
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeValid
 * Signature: (J)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_FileDescriptor_nativeValid (JNIEnv *env ,  struct java_io_FileDescriptor* this , s8 par1);
/*
 * Class:     java/io/FileDescriptor
 * Method:    nativeSync
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_io_FileDescriptor_nativeSync (JNIEnv *env ,  struct java_io_FileDescriptor* this , s8 par1);
