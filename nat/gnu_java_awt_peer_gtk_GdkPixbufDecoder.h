/* This file is machine generated, don't edit it !*/

/* Structure information for class: gnu/java/awt/peer/gtk/GdkPixbufDecoder */

typedef struct gnu_java_awt_peer_gtk_GdkPixbufDecoder {
   java_objectheader header;
   struct java_util_Vector* consumers;
   struct java_lang_String* filename;
   struct java_net_URL* url;
   s4 native_state;
   struct java_util_Vector* curr;
} gnu_java_awt_peer_gtk_GdkPixbufDecoder;

/*
 * Class:     gnu/java/awt/peer/gtk/GdkPixbufDecoder
 * Method:    initStaticState
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkPixbufDecoder_initStaticState (JNIEnv *env , jclass clazz );
/*
 * Class:     gnu/java/awt/peer/gtk/GdkPixbufDecoder
 * Method:    initState
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkPixbufDecoder_initState (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkPixbufDecoder* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GdkPixbufDecoder
 * Method:    pumpBytes
 * Signature: ([BI)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkPixbufDecoder_pumpBytes (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkPixbufDecoder* this , java_bytearray* par1, s4 par2);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkPixbufDecoder
 * Method:    finish
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkPixbufDecoder_finish (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkPixbufDecoder* this );
