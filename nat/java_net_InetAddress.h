/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/net/InetAddress */

typedef struct java_net_InetAddress {
   java_objectheader header;
   java_bytearray* addr;
   struct java_lang_String* hostName;
   struct java_lang_String* hostname_alias;
   s8 lookup_time;
   s4 address;
   s4 family;
} java_net_InetAddress;

/*
 * Class:     java/net/InetAddress
 * Method:    getLocalHostName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_net_InetAddress_getLocalHostName (JNIEnv *env , jclass clazz );
/*
 * Class:     java/net/InetAddress
 * Method:    lookupInaddrAny
 * Signature: ()[B
 */
JNIEXPORT java_bytearray* JNICALL Java_java_net_InetAddress_lookupInaddrAny (JNIEnv *env , jclass clazz );
/*
 * Class:     java/net/InetAddress
 * Method:    getHostByAddr
 * Signature: ([B)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_net_InetAddress_getHostByAddr (JNIEnv *env , jclass clazz , java_bytearray* par1);
/*
 * Class:     java/net/InetAddress
 * Method:    getHostByName
 * Signature: (Ljava/lang/String;)[[B
 */
JNIEXPORT java_objectarray* JNICALL Java_java_net_InetAddress_getHostByName (JNIEnv *env , jclass clazz , struct java_lang_String* par1);
