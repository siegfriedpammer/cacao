/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/net/DatagramPacket */

typedef struct java_net_DatagramPacket {
   java_objectheader header;
   java_bytearray* buf;
   s4 offset;
   s4 length;
   struct java_net_InetAddress* address;
   s4 port;
} java_net_DatagramPacket;

/*
 * Class:     java/net/DatagramPacket
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_net_DatagramPacket_init (JNIEnv *env );
