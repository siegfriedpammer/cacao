/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_NIO_FILELOCKIMPL_H
#define _GNU_JAVA_NIO_FILELOCKIMPL_H

/* Structure information for class: gnu/java/nio/FileLockImpl */

typedef struct gnu_java_nio_FileLockImpl {
   java_objectheader header;
   struct java_nio_channels_FileChannel* channel;
   s8 position;
   s8 size;
   s4 shared;
   struct java_io_FileDescriptor* fd;
   s4 released;
} gnu_java_nio_FileLockImpl;


/*
 * Class:     gnu/java/nio/FileLockImpl
 * Method:    releaseImpl
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_nio_FileLockImpl_releaseImpl(JNIEnv *env, struct gnu_java_nio_FileLockImpl* this);

#endif

