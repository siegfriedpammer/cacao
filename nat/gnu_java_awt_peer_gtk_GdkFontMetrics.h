/* This file is machine generated, don't edit it !*/

/* Structure information for class: gnu/java/awt/peer/gtk/GdkFontMetrics */

typedef struct gnu_java_awt_peer_gtk_GdkFontMetrics {
   java_objectheader header;
   struct java_awt_Font* font;
   s4 native_state;
   java_intarray* metrics;
} gnu_java_awt_peer_gtk_GdkFontMetrics;

/*
 * Class:     gnu/java/awt/peer/gtk/GdkFontMetrics
 * Method:    initState
 * Signature: (Ljava/lang/String;I)[I
 */
JNIEXPORT java_intarray* JNICALL Java_gnu_java_awt_peer_gtk_GdkFontMetrics_initState (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkFontMetrics* this , struct java_lang_String* par1, s4 par2);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkFontMetrics
 * Method:    stringWidth
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GdkFontMetrics_stringWidth (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkFontMetrics* this , struct java_lang_String* par1);
