/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/net/NetworkInterface */

typedef struct java_net_NetworkInterface {
   java_objectheader header;
   struct java_lang_String* name;
   struct java_util_Vector* inetAddresses;
} java_net_NetworkInterface;

/*
 * Class:     java/net/NetworkInterface
 * Method:    getRealNetworkInterfaces
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT struct java_util_Vector* JNICALL Java_java_net_NetworkInterface_getRealNetworkInterfaces (JNIEnv *env , jclass clazz );
