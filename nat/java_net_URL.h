/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_NET_URL_H
#define _JAVA_NET_URL_H

/* Structure information for class: java/net/URL */

typedef struct java_net_URL {
   java_objectheader header;
   struct java_lang_String* protocol;
   struct java_lang_String* authority;
   struct java_lang_String* host;
   struct java_lang_String* userInfo;
   s4 port;
   struct java_lang_String* file;
   struct java_lang_String* ref;
   s4 hashCode;
   struct java_net_URLStreamHandler* ph;
} java_net_URL;

#endif

