/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/net/SocketInputStream */

typedef struct java_net_SocketInputStream {
   java_objectheader header;
   struct java_io_FileDescriptor* fd;
   s4 eof;
   struct java_net_SocketImpl* impl;
   java_bytearray* temp;
} java_net_SocketInputStream;

/*
 * Class:     java/net/SocketInputStream
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_net_SocketInputStream_init (JNIEnv *env );
/*
 * Class:     java/net/SocketInputStream
 * Method:    socketRead
 * Signature: ([BII)I
 */
JNIEXPORT s4 JNICALL Java_java_net_SocketInputStream_socketRead (JNIEnv *env ,  struct java_net_SocketInputStream* this , java_bytearray* par1, s4 par2, s4 par3);
