/* This file is machine generated, don't edit it !*/

/* Structure information for class: gnu/java/awt/image/GdkPixbufDecoder */

typedef struct gnu_java_awt_image_GdkPixbufDecoder {
   java_objectheader header;
   struct java_util_Vector* consumers;
   struct java_lang_String* filename;
   struct java_net_URL* url;
} gnu_java_awt_image_GdkPixbufDecoder;

/*
 * Class:     gnu/java/awt/image/GdkPixbufDecoder
 * Method:    initState
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_image_GdkPixbufDecoder_initState (JNIEnv *env , jclass clazz );
/*
 * Class:     gnu/java/awt/image/GdkPixbufDecoder
 * Method:    loaderWrite
 * Signature: (Ljava/util/Vector;Ljava/io/FileDescriptor;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_image_GdkPixbufDecoder_loaderWrite (JNIEnv *env ,  struct gnu_java_awt_image_GdkPixbufDecoder* this , struct java_util_Vector* par1, struct java_io_FileDescriptor* par2);
