/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_NET_DATAGRAMPACKET_H
#define _JAVA_NET_DATAGRAMPACKET_H

/* Structure information for class: java/net/DatagramPacket */

typedef struct java_net_DatagramPacket {
   java_objectheader header;
   java_bytearray* buffer;
   s4 offset;
   s4 length;
   s4 maxlen;
   struct java_net_InetAddress* address;
   s4 port;
} java_net_DatagramPacket;

#endif

