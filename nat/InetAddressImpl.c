/* class: java/net/InetAddressImpl */

#include <netinet/in.h>
#include <netdb.h>

#define HOSTNMSZ 80

/*
 * Class:     java/net/InetAddressImpl
 * Method:    getHostByAddr
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_net_InetAddressImpl_getHostByAddr (JNIEnv *env ,  struct java_net_InetAddressImpl* this , s4 addr)
{
    struct hostent* ent;

    if (runverbose)
	log_text("Java_java_net_InetAddressImpl_getHostByAddr called");

    addr = htonl(addr);
    ent = gethostbyaddr((char*)&addr, sizeof(s4), AF_INET);
    if (ent == 0) {
	exceptionptr = native_new_and_init (class_java_net_UnknownHostException);
	return 0;
    }

    return (java_lang_String*)javastring_new_char((char*)ent->h_name);
}

/*
 * Class:     java/net/InetAddressImpl
 * Method:    getInetFamily
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_net_InetAddressImpl_getInetFamily (JNIEnv *env ,  struct java_net_InetAddressImpl* this )
{
    if (runverbose)
	log_text("Java_java_net_InetAddressImpl_getInetFamily called");

    return (AF_INET);
}

/*
 * Class:     java/net/InetAddressImpl
 * Method:    getLocalHostName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_net_InetAddressImpl_getLocalHostName (JNIEnv *env ,  struct java_net_InetAddressImpl* this )
{
    char hostname[HOSTNMSZ];

    if (runverbose)
	log_text("Java_java_net_InetAddressImpl_getLocalHostName called");

    if (gethostname(hostname, HOSTNMSZ-1) < 0) {
	return (java_lang_String*)javastring_new_char("localhost");
    }
    return (java_lang_String*)javastring_new_char(hostname);
}

/*
 * Class:     java/net/InetAddressImpl
 * Method:    lookupAllHostAddr
 * Signature: (Ljava/lang/String;)[[B
 */
JNIEXPORT java_arrayarray* JNICALL Java_java_net_InetAddressImpl_lookupAllHostAddr (JNIEnv *env ,  struct java_net_InetAddressImpl* this , struct java_lang_String* str)
{
    char *name;
    struct hostent* ent;
    java_arrayarray *array;
    int i;
    int alength;
    constant_arraydescriptor arraydesc;

    if (runverbose)
	log_text("Java_java_net_InetAddressImpl_lookupAllHostAddr called");

    name = javastring_tochar((java_objectheader*)str);

    ent = gethostbyname(name);
    if (ent == 0) {
	exceptionptr = native_new_and_init (class_java_net_UnknownHostException);
	return 0;
    }

    for (alength = 0; ent->h_addr_list[alength]; alength++)
	;

    arraydesc.arraytype = ARRAYTYPE_BYTE;
    array = builtin_newarray_array(alength, &arraydesc);
    assert(array != 0);

    for (i = 0; i < alength; i++) {
	java_bytearray *obj;

	/* Copy in the network address */
	obj = builtin_newarray_byte(sizeof(s4));
	assert(obj != 0);
	*(s4*)obj->data = *(s4*)ent->h_addr_list[i];
	array->data[i] = (java_arrayheader*)obj;
    }

    return array;
}

/*
 * Class:     java/net/InetAddressImpl
 * Method:    makeAnyLocalAddress
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL Java_java_net_InetAddressImpl_makeAnyLocalAddress (JNIEnv *env ,  struct java_net_InetAddressImpl* this , struct java_net_InetAddress* par1)
{
    if (runverbose)
	log_text("Java_java_net_InetAddressImpl_makeAnyLocalAddress called");

    /* par1->hostName = 0; */
    par1->address = htonl(INADDR_ANY);
    par1->family = AF_INET;
}
