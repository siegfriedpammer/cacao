/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/net/PlainSocketImpl */

typedef struct java_net_PlainSocketImpl {
   java_objectheader header;
   struct java_io_FileDescriptor* fd;
   struct java_net_InetAddress* address;
   s4 port;
   s4 localport;
   s4 timeout;
} java_net_PlainSocketImpl;

/*
 * Class:     java/net/PlainSocketImpl
 * Method:    initProto
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_net_PlainSocketImpl_initProto (JNIEnv *env );
/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketAccept
 * Signature: (Ljava/net/SocketImpl;)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainSocketImpl_socketAccept (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , struct java_net_SocketImpl* par1);
/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketAvailable
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_net_PlainSocketImpl_socketAvailable (JNIEnv *env ,  struct java_net_PlainSocketImpl* this );
/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketBind
 * Signature: (Ljava/net/InetAddress;I)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainSocketImpl_socketBind (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , struct java_net_InetAddress* par1, s4 par2);
/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_net_PlainSocketImpl_socketClose (JNIEnv *env ,  struct java_net_PlainSocketImpl* this );
/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketConnect
 * Signature: (Ljava/net/InetAddress;I)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainSocketImpl_socketConnect (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , struct java_net_InetAddress* par1, s4 par2);
/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketCreate
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainSocketImpl_socketCreate (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , s4 par1);
/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketGetOption
 * Signature: (I)I
 */
JNIEXPORT s4 JNICALL Java_java_net_PlainSocketImpl_socketGetOption (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , s4 par1);
/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketListen
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainSocketImpl_socketListen (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , s4 par1);
/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketSetOption
 * Signature: (IZLjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_net_PlainSocketImpl_socketSetOption (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , s4 par1, s4 par2, struct java_lang_Object* par3);
