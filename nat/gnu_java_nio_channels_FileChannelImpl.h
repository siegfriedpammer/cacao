/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_NIO_CHANNELS_FILECHANNELIMPL_H
#define _GNU_JAVA_NIO_CHANNELS_FILECHANNELIMPL_H

/* Structure information for class: gnu/java/nio/channels/FileChannelImpl */

typedef struct gnu_java_nio_channels_FileChannelImpl {
   java_objectheader header;
   s4 opened;
   s4 fd;
   s4 mode;
} gnu_java_nio_channels_FileChannelImpl;


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_nio_channels_FileChannelImpl_init(JNIEnv *env, jclass clazz);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    open
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_nio_channels_FileChannelImpl_open(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this, struct java_lang_String* par1, s4 par2);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    available
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_nio_channels_FileChannelImpl_available(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    implPosition
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_gnu_java_nio_channels_FileChannelImpl_implPosition(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    seek
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_gnu_java_nio_channels_FileChannelImpl_seek(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this, s8 par1);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    implTruncate
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_gnu_java_nio_channels_FileChannelImpl_implTruncate(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this, s8 par1);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    unlock
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_gnu_java_nio_channels_FileChannelImpl_unlock(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this, s8 par1, s8 par2);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    size
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_gnu_java_nio_channels_FileChannelImpl_size(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    implCloseChannel
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_nio_channels_FileChannelImpl_implCloseChannel(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    read
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_nio_channels_FileChannelImpl_read__(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    read
 * Signature: ([BII)I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_nio_channels_FileChannelImpl_read___3BII(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this, java_bytearray* par1, s4 par2, s4 par3);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    write
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_nio_channels_FileChannelImpl_write___3BII(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this, java_bytearray* par1, s4 par2, s4 par3);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    write
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_nio_channels_FileChannelImpl_write__I(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this, s4 par1);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    mapImpl
 * Signature: (CJI)Ljava/nio/MappedByteBuffer;
 */
JNIEXPORT struct java_nio_MappedByteBuffer* JNICALL Java_gnu_java_nio_channels_FileChannelImpl_mapImpl(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this, s4 par1, s8 par2, s4 par3);


/*
 * Class:     gnu/java/nio/channels/FileChannelImpl
 * Method:    lock
 * Signature: (JJZZ)Z
 */
JNIEXPORT s4 JNICALL Java_gnu_java_nio_channels_FileChannelImpl_lock(JNIEnv *env, struct gnu_java_nio_channels_FileChannelImpl* this, s8 par1, s8 par2, s4 par3, s4 par4);

#endif

