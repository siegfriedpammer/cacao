#include "jni.h"
#include "java_lang_ClassLoader.h"


/*
 * Class:     java_lang_reflect_Proxy
 * Method:    getProxyClass0
 * Signature: (Ljava/lang/ClassLoader;[Ljava/lang/Class;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_reflect_Proxy_getProxyClass0 (JNIEnv *env , jclass clazz, struct java_lang_ClassLoader* par1, java_objectarray* par2) {
	log_text("Java_java_lang_reflect_Proxy_getProxyClass0");
	return 0;
}
/*
 * Class:     java_lang_reflect_Proxy
 * Method:    getProxyData0
 * Signature: (Ljava/lang/ClassLoader;[Ljava/lang/Class;)Ljava/lang/reflect/Proxy$ProxyData;
 */
JNIEXPORT struct java_lang_reflect_Proxy_ProxyData* JNICALL Java_java_lang_reflect_Proxy_getProxyData0 (JNIEnv *env , jclass clazz, struct java_lang_ClassLoader* par1, java_objectarray* par2) {
	log_text("Java_java_lang_reflect_Proxy_getProxyClass0");
	return 0;
}

/*
 * Class:     java_lang_reflect_Proxy
 * Method:    generateProxyClass0
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/reflect/Proxy$ProxyData;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_reflect_Proxy_generateProxyClass0 (JNIEnv *env , jclass clazz, struct java_lang_ClassLoader* par1, struct java_lang_reflect_Proxy_ProxyData* par2) {
	log_text("Java_java_lang_reflect_Proxy_getProxyClass0");
	return 0;
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
