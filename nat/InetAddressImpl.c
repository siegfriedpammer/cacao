/* class: java/net/InetAddressImpl */

/*
 * Class:     java/net/InetAddressImpl
 * Method:    getHostByAddr
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_net_InetAddressImpl_getHostByAddr (JNIEnv *env ,  struct java_net_InetAddressImpl* this , s4 par1)
{
  log_text("Java_java_net_InetAddressImpl_getHostByAddr called");
}

/*
 * Class:     java/net/InetAddressImpl
 * Method:    getInetFamily
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_net_InetAddressImpl_getInetFamily (JNIEnv *env ,  struct java_net_InetAddressImpl* this )
{
  log_text("Java_java_net_InetAddressImpl_getInetFamily called");
}

/*
 * Class:     java/net/InetAddressImpl
 * Method:    getLocalHostName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_net_InetAddressImpl_getLocalHostName (JNIEnv *env ,  struct java_net_InetAddressImpl* this )
{
  log_text("Java_java_net_InetAddressImpl_getLocalHostName called");
}

/*
 * Class:     java/net/InetAddressImpl
 * Method:    lookupAllHostAddr
 * Signature: (Ljava/lang/String;)[[B
 */
JNIEXPORT java_arrayarray* JNICALL Java_java_net_InetAddressImpl_lookupAllHostAddr (JNIEnv *env ,  struct java_net_InetAddressImpl* this , struct java_lang_String* par1)
{
  log_text("Java_java_net_InetAddressImpl_lookupAllHostAddr called");
}

/*
 * Class:     java/net/InetAddressImpl
 * Method:    makeAnyLocalAddress
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL Java_java_net_InetAddressImpl_makeAnyLocalAddress (JNIEnv *env ,  struct java_net_InetAddressImpl* this , struct java_net_InetAddress* par1)
{
  log_text("Java_java_net_InetAddressImpl_makeAnyLocalAddress called");
}

