/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_NET_PLAINDATAGRAMSOCKETIMPL_H
#define _GNU_JAVA_NET_PLAINDATAGRAMSOCKETIMPL_H

/* Structure information for class: gnu/java/net/PlainDatagramSocketImpl */

typedef struct gnu_java_net_PlainDatagramSocketImpl {
   java_objectheader header;
   s4 localPort;
   struct java_io_FileDescriptor* fd;
   s4 native_fd;
   struct java_lang_Object* RECEIVE_LOCK;
   struct java_lang_Object* SEND_LOCK;
} gnu_java_net_PlainDatagramSocketImpl;


/*
 * Class:     gnu/java/net/PlainDatagramSocketImpl
 * Method:    bind
 * Signature: (ILjava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainDatagramSocketImpl_bind(JNIEnv *env, struct gnu_java_net_PlainDatagramSocketImpl* this, s4 par1, struct java_net_InetAddress* par2);


/*
 * Class:     gnu/java/net/PlainDatagramSocketImpl
 * Method:    create
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainDatagramSocketImpl_create(JNIEnv *env, struct gnu_java_net_PlainDatagramSocketImpl* this);


/*
 * Class:     gnu/java/net/PlainDatagramSocketImpl
 * Method:    sendto
 * Signature: (Ljava/net/InetAddress;I[BII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainDatagramSocketImpl_sendto(JNIEnv *env, struct gnu_java_net_PlainDatagramSocketImpl* this, struct java_net_InetAddress* par1, s4 par2, java_bytearray* par3, s4 par4, s4 par5);


/*
 * Class:     gnu/java/net/PlainDatagramSocketImpl
 * Method:    receive0
 * Signature: (Ljava/net/DatagramPacket;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainDatagramSocketImpl_receive0(JNIEnv *env, struct gnu_java_net_PlainDatagramSocketImpl* this, struct java_net_DatagramPacket* par1);


/*
 * Class:     gnu/java/net/PlainDatagramSocketImpl
 * Method:    setOption
 * Signature: (ILjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainDatagramSocketImpl_setOption(JNIEnv *env, struct gnu_java_net_PlainDatagramSocketImpl* this, s4 par1, struct java_lang_Object* par2);


/*
 * Class:     gnu/java/net/PlainDatagramSocketImpl
 * Method:    getOption
 * Signature: (I)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_gnu_java_net_PlainDatagramSocketImpl_getOption(JNIEnv *env, struct gnu_java_net_PlainDatagramSocketImpl* this, s4 par1);


/*
 * Class:     gnu/java/net/PlainDatagramSocketImpl
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainDatagramSocketImpl_close(JNIEnv *env, struct gnu_java_net_PlainDatagramSocketImpl* this);


/*
 * Class:     gnu/java/net/PlainDatagramSocketImpl
 * Method:    join
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainDatagramSocketImpl_join(JNIEnv *env, struct gnu_java_net_PlainDatagramSocketImpl* this, struct java_net_InetAddress* par1);


/*
 * Class:     gnu/java/net/PlainDatagramSocketImpl
 * Method:    leave
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_net_PlainDatagramSocketImpl_leave(JNIEnv *env, struct gnu_java_net_PlainDatagramSocketImpl* this, struct java_net_InetAddress* par1);

#endif

