/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/net/InetAddressImpl */

typedef struct java_net_InetAddressImpl {
   java_objectheader header;
} java_net_InetAddressImpl;

/*
 * Class:     java/net/InetAddressImpl
 * Method:    getHostByAddr
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_net_InetAddressImpl_getHostByAddr (JNIEnv *env ,  struct java_net_InetAddressImpl* this , s4 par1);
/*
 * Class:     java/net/InetAddressImpl
 * Method:    getInetFamily
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_net_InetAddressImpl_getInetFamily (JNIEnv *env ,  struct java_net_InetAddressImpl* this );
/*
 * Class:     java/net/InetAddressImpl
 * Method:    getLocalHostName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_net_InetAddressImpl_getLocalHostName (JNIEnv *env ,  struct java_net_InetAddressImpl* this );
/*
 * Class:     java/net/InetAddressImpl
 * Method:    lookupAllHostAddr
 * Signature: (Ljava/lang/String;)[[B
 */
JNIEXPORT java_arrayarray* JNICALL Java_java_net_InetAddressImpl_lookupAllHostAddr (JNIEnv *env ,  struct java_net_InetAddressImpl* this , struct java_lang_String* par1);
/*
 * Class:     java/net/InetAddressImpl
 * Method:    makeAnyLocalAddress
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL Java_java_net_InetAddressImpl_makeAnyLocalAddress (JNIEnv *env ,  struct java_net_InetAddressImpl* this , struct java_net_InetAddress* par1);
