/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/nio/DirectByteBufferImpl */

typedef struct java_nio_DirectByteBufferImpl {
   java_objectheader header;
   s4 cap;
   s4 limit;
   s4 pos;
   s4 mark;
   struct java_nio_ByteOrder* endian;
   s4 array_offset;
   java_bytearray* backing_buffer;
   struct gnu_classpath_RawData* address;
   s4 offset;
   s4 readOnly;
} java_nio_DirectByteBufferImpl;

/*
 * Class:     java/nio/DirectByteBufferImpl
 * Method:    allocateImpl
 * Signature: (I)Lgnu/classpath/RawData;
 */
JNIEXPORT struct gnu_classpath_RawData* JNICALL Java_java_nio_DirectByteBufferImpl_allocateImpl (JNIEnv *env , jclass clazz , s4 par1);
/*
 * Class:     java/nio/DirectByteBufferImpl
 * Method:    freeImpl
 * Signature: (Lgnu/classpath/RawData;)V
 */
JNIEXPORT void JNICALL Java_java_nio_DirectByteBufferImpl_freeImpl (JNIEnv *env , jclass clazz , struct gnu_classpath_RawData* par1);
/*
 * Class:     java/nio/DirectByteBufferImpl
 * Method:    getImpl
 * Signature: (I)B
 */
JNIEXPORT s4 JNICALL Java_java_nio_DirectByteBufferImpl_getImpl (JNIEnv *env ,  struct java_nio_DirectByteBufferImpl* this , s4 par1);
/*
 * Class:     java/nio/DirectByteBufferImpl
 * Method:    putImpl
 * Signature: (IB)V
 */
JNIEXPORT void JNICALL Java_java_nio_DirectByteBufferImpl_putImpl (JNIEnv *env ,  struct java_nio_DirectByteBufferImpl* this , s4 par1, s4 par2);
