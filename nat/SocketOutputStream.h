/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/net/SocketOutputStream */

typedef struct java_net_SocketOutputStream {
   java_objectheader header;
   struct java_io_FileDescriptor* fd;
   struct java_net_SocketImpl* impl;
   java_bytearray* temp;
} java_net_SocketOutputStream;

/*
 * Class:     java/net/SocketOutputStream
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_net_SocketOutputStream_init (JNIEnv *env );
/*
 * Class:     java/net/SocketOutputStream
 * Method:    socketWrite
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL Java_java_net_SocketOutputStream_socketWrite (JNIEnv *env ,  struct java_net_SocketOutputStream* this , java_bytearray* par1, s4 par2, s4 par3);
