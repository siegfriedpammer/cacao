/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/net/PlainDatagramSocketImpl */

typedef struct java_net_PlainDatagramSocketImpl {
   java_objectheader header;
   s4 localPort;
   struct java_io_FileDescriptor* fd;
   s4 timeout;
} java_net_PlainDatagramSocketImpl;

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    bind
 * Signature: (ILjava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainDatagramSocketImpl_bind (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , s4 par1, struct java_net_InetAddress* par2);
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    datagramSocketClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_net_PlainDatagramSocketImpl_datagramSocketClose (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this );
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    datagramSocketCreate
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_net_PlainDatagramSocketImpl_datagramSocketCreate (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this );
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    getTTL
 * Signature: ()B
 */
JNIEXPORT s4 JNICALL Java_java_net_PlainDatagramSocketImpl_getTTL (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this );
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    getTimeToLive
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_net_PlainDatagramSocketImpl_getTimeToLive (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this );
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_net_PlainDatagramSocketImpl_init (JNIEnv *env );
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    join
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainDatagramSocketImpl_join (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , struct java_net_InetAddress* par1);
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    leave
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainDatagramSocketImpl_leave (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , struct java_net_InetAddress* par1);
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    peek
 * Signature: (Ljava/net/InetAddress;)I
 */
JNIEXPORT s4 JNICALL Java_java_net_PlainDatagramSocketImpl_peek (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , struct java_net_InetAddress* par1);
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    receive
 * Signature: (Ljava/net/DatagramPacket;)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainDatagramSocketImpl_receive (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , struct java_net_DatagramPacket* par1);
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    send
 * Signature: (Ljava/net/DatagramPacket;)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainDatagramSocketImpl_send (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , struct java_net_DatagramPacket* par1);
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    setTTL
 * Signature: (B)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainDatagramSocketImpl_setTTL (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , s4 par1);
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    setTimeToLive
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainDatagramSocketImpl_setTimeToLive (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , s4 par1);
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    socketGetOption
 * Signature: (I)I
 */
JNIEXPORT s4 JNICALL Java_java_net_PlainDatagramSocketImpl_socketGetOption (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , s4 par1);
/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    socketSetOption
 * Signature: (ILjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainDatagramSocketImpl_socketSetOption (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , s4 par1, struct java_lang_Object* par2);
