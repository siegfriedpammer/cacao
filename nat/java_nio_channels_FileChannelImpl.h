/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/nio/channels/FileChannelImpl */

typedef struct java_nio_channels_FileChannelImpl {
   java_objectheader header;
   s4 opened;
   struct gnu_classpath_RawData* map_address;
   s4 length;
   struct java_io_FileDescriptor* fd;
   struct java_nio_MappedByteBuffer* buf;
   struct java_lang_Object* file_obj;
} java_nio_channels_FileChannelImpl;

/*
 * Class:     java/nio/channels/FileChannelImpl
 * Method:    implPosition
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_nio_channels_FileChannelImpl_implPosition (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this );
/*
 * Class:     java/nio/channels/FileChannelImpl
 * Method:    implPosition
 * Signature: (J)Ljava/nio/channels/FileChannel;
 */
JNIEXPORT struct java_nio_channels_FileChannel* JNICALL Java_java_nio_channels_FileChannelImpl0_implPosition (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this , s8 par1);
/*
 * Class:     java/nio/channels/FileChannelImpl
 * Method:    implTruncate
 * Signature: (J)Ljava/nio/channels/FileChannel;
 */
JNIEXPORT struct java_nio_channels_FileChannel* JNICALL Java_java_nio_channels_FileChannelImpl_implTruncate (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this , s8 par1);
/*
 * Class:     java/nio/channels/FileChannelImpl
 * Method:    nio_mmap_file
 * Signature: (JJI)Lgnu/classpath/RawData;
 */
JNIEXPORT struct gnu_classpath_RawData* JNICALL Java_java_nio_channels_FileChannelImpl_nio_mmap_file (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this , s8 par1, s8 par2, s4 par3);
/*
 * Class:     java/nio/channels/FileChannelImpl
 * Method:    nio_unmmap_file
 * Signature: (Lgnu/classpath/RawData;I)V
 */
JNIEXPORT void JNICALL Java_java_nio_channels_FileChannelImpl_nio_unmmap_file (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this , struct gnu_classpath_RawData* par1, s4 par2);
/*
 * Class:     java/nio/channels/FileChannelImpl
 * Method:    nio_msync
 * Signature: (Lgnu/classpath/RawData;I)V
 */
JNIEXPORT void JNICALL Java_java_nio_channels_FileChannelImpl_nio_msync (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this , struct gnu_classpath_RawData* par1, s4 par2);
/*
 * Class:     java/nio/channels/FileChannelImpl
 * Method:    size
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_nio_channels_FileChannelImpl_size (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this );
/*
 * Class:     java/nio/channels/FileChannelImpl
 * Method:    implRead
 * Signature: ([BII)I
 */
JNIEXPORT s4 JNICALL Java_java_nio_channels_FileChannelImpl_implRead (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this , java_bytearray* par1, s4 par2, s4 par3);
/*
 * Class:     java/nio/channels/FileChannelImpl
 * Method:    implWrite
 * Signature: ([BII)I
 */
JNIEXPORT s4 JNICALL Java_java_nio_channels_FileChannelImpl_implWrite (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this , java_bytearray* par1, s4 par2, s4 par3);
