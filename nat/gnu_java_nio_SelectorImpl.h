/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_NIO_SELECTORIMPL_H
#define _GNU_JAVA_NIO_SELECTORIMPL_H

/* Structure information for class: gnu/java/nio/SelectorImpl */

typedef struct gnu_java_nio_SelectorImpl {
   java_objectheader header;
   s4 closed;
   struct java_nio_channels_spi_SelectorProvider* provider;
   struct java_util_Set* keys;
   struct java_util_Set* selected;
   struct java_lang_Object* selectThreadMutex;
   struct java_lang_Thread* selectThread;
   s4 unhandledWakeup;
} gnu_java_nio_SelectorImpl;


/*
 * Class:     gnu/java/nio/SelectorImpl
 * Method:    implSelect
 * Signature: ([I[I[IJ)I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_nio_SelectorImpl_implSelect(JNIEnv *env, jclass clazz, java_intarray* par1, java_intarray* par2, java_intarray* par3, s8 par4);

#endif

