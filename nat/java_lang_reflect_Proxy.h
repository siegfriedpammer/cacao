/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_LANG_REFLECT_PROXY_H
#define _JAVA_LANG_REFLECT_PROXY_H

/* Structure information for class: java/lang/reflect/Proxy */

typedef struct java_lang_reflect_Proxy {
   java_objectheader header;
   struct java_lang_reflect_InvocationHandler* h;
} java_lang_reflect_Proxy;


/*
 * Class:     java/lang/reflect/Proxy
 * Method:    getProxyClass0
 * Signature: (Ljava/lang/ClassLoader;[Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_reflect_Proxy_getProxyClass0(JNIEnv *env, jclass clazz, struct java_lang_ClassLoader* par1, java_objectarray* par2);


/*
 * Class:     java/lang/reflect/Proxy
 * Method:    getProxyData0
 * Signature: (Ljava/lang/ClassLoader;[Ljava/lang/Class;)Ljava/lang/reflect/Proxy$ProxyData;
 */
JNIEXPORT struct java_lang_reflect_Proxy_ProxyData* JNICALL Java_java_lang_reflect_Proxy_getProxyData0(JNIEnv *env, jclass clazz, struct java_lang_ClassLoader* par1, java_objectarray* par2);


/*
 * Class:     java/lang/reflect/Proxy
 * Method:    generateProxyClass0
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/reflect/Proxy$ProxyData;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_reflect_Proxy_generateProxyClass0(JNIEnv *env, jclass clazz, struct java_lang_ClassLoader* par1, struct java_lang_reflect_Proxy_ProxyData* par2);

#endif

