/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GDKFONTMETRICS_H
#define _GNU_JAVA_AWT_PEER_GTK_GDKFONTMETRICS_H

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
 * Signature: (Ljava/lang/String;II)[I
 */
JNIEXPORT java_intarray* JNICALL Java_gnu_java_awt_peer_gtk_GdkFontMetrics_initState(JNIEnv *env, struct gnu_java_awt_peer_gtk_GdkFontMetrics* this, struct java_lang_String* par1, s4 par2, s4 par3);


/*
 * Class:     gnu/java/awt/peer/gtk/GdkFontMetrics
 * Method:    stringWidth
 * Signature: (Ljava/lang/String;IILjava/lang/String;)I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GdkFontMetrics_stringWidth(JNIEnv *env, struct gnu_java_awt_peer_gtk_GdkFontMetrics* this, struct java_lang_String* par1, s4 par2, s4 par3, struct java_lang_String* par4);

#endif

