/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_NET_SOCKETIMPL_H
#define _JAVA_NET_SOCKETIMPL_H

/* Structure information for class: java/net/SocketImpl */

typedef struct java_net_SocketImpl {
   java_objectheader header;
   struct java_net_InetAddress* address;
   struct java_io_FileDescriptor* fd;
   s4 localport;
   s4 port;
} java_net_SocketImpl;

#endif

