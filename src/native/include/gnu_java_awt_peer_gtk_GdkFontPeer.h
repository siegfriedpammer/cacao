/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GDKFONTPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GDKFONTPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GdkFontPeer */

typedef struct gnu_java_awt_peer_gtk_GdkFontPeer {
   java_objectheader header;
   struct java_lang_String* logicalName;
   struct java_lang_String* familyName;
   struct java_lang_String* faceName;
   s4 style;
   float size;
   struct java_awt_geom_AffineTransform* transform;
   s4 native_state;
} gnu_java_awt_peer_gtk_GdkFontPeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GdkFontPeer
 * Method:    initStaticState
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkFontPeer_initStaticState(JNIEnv *env, jclass clazz);


/*
 * Class:     gnu/java/awt/peer/gtk/GdkFontPeer
 * Method:    initState
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkFontPeer_initState(JNIEnv *env, struct gnu_java_awt_peer_gtk_GdkFontPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GdkFontPeer
 * Method:    dispose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkFontPeer_dispose(JNIEnv *env, struct gnu_java_awt_peer_gtk_GdkFontPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GdkFontPeer
 * Method:    setFont
 * Signature: (Ljava/lang/String;IIZ)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkFontPeer_setFont(JNIEnv *env, struct gnu_java_awt_peer_gtk_GdkFontPeer* this, struct java_lang_String* par1, s4 par2, s4 par3, s4 par4);

#endif

