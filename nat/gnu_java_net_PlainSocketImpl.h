/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_NET_PLAINSOCKETIMPL_H
#define _GNU_JAVA_NET_PLAINSOCKETIMPL_H

/* Structure information for class: gnu/java/net/PlainSocketImpl */

typedef struct gnu_java_net_PlainSocketImpl {
   java_objectheader header;
   struct java_net_InetAddress* address;
   struct java_io_FileDescriptor* fd;
   s4 localport;
   s4 port;
   s4 native_fd;
   struct java_io_InputStream* in;
   struct java_io_OutputStream* out;
   s4 inChannelOperation;
} gnu_java_net_PlainSocketImpl;


/*
 * Class:     gnu/java/net/PlainSocketImpl
 * Method:    setOption
 * Signature: (ILjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainSocketImpl_setOption(JNIEnv *env, struct gnu_java_net_PlainSocketImpl* this, s4 par1, struct java_lang_Object* par2);


/*
 * Class:     gnu/java/net/PlainSocketImpl
 * Method:    getOption
 * Signature: (I)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_gnu_java_net_PlainSocketImpl_getOption(JNIEnv *env, struct gnu_java_net_PlainSocketImpl* this, s4 par1);


/*
 * Class:     gnu/java/net/PlainSocketImpl
 * Method:    create
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainSocketImpl_create(JNIEnv *env, struct gnu_java_net_PlainSocketImpl* this, s4 par1);


/*
 * Class:     gnu/java/net/PlainSocketImpl
 * Method:    connect
 * Signature: (Ljava/net/InetAddress;I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainSocketImpl_connect(JNIEnv *env, struct gnu_java_net_PlainSocketImpl* this, struct java_net_InetAddress* par1, s4 par2);


/*
 * Class:     gnu/java/net/PlainSocketImpl
 * Method:    bind
 * Signature: (Ljava/net/InetAddress;I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainSocketImpl_bind(JNIEnv *env, struct gnu_java_net_PlainSocketImpl* this, struct java_net_InetAddress* par1, s4 par2);


/*
 * Class:     gnu/java/net/PlainSocketImpl
 * Method:    listen
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainSocketImpl_listen(JNIEnv *env, struct gnu_java_net_PlainSocketImpl* this, s4 par1);


/*
 * Class:     gnu/java/net/PlainSocketImpl
 * Method:    accept
 * Signature: (Ljava/net/SocketImpl;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainSocketImpl_accept(JNIEnv *env, struct gnu_java_net_PlainSocketImpl* this, struct java_net_SocketImpl* par1);


/*
 * Class:     gnu/java/net/PlainSocketImpl
 * Method:    available
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_net_PlainSocketImpl_available(JNIEnv *env, struct gnu_java_net_PlainSocketImpl* this);


/*
 * Class:     gnu/java/net/PlainSocketImpl
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainSocketImpl_close(JNIEnv *env, struct gnu_java_net_PlainSocketImpl* this);


/*
 * Class:     gnu/java/net/PlainSocketImpl
 * Method:    read
 * Signature: ([BII)I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_net_PlainSocketImpl_read(JNIEnv *env, struct gnu_java_net_PlainSocketImpl* this, java_bytearray* par1, s4 par2, s4 par3);


/*
 * Class:     gnu/java/net/PlainSocketImpl
 * Method:    write
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainSocketImpl_write(JNIEnv *env, struct gnu_java_net_PlainSocketImpl* this, java_bytearray* par1, s4 par2, s4 par3);

#endif

