/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKSCROLLBARPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKSCROLLBARPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkScrollbarPeer */

typedef struct gnu_java_awt_peer_gtk_GtkScrollbarPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
} gnu_java_awt_peer_gtk_GtkScrollbarPeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollbarPeer
 * Method:    create
 * Signature: (IIIIIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollbarPeer_create(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollbarPeer* this, s4 par1, s4 par2, s4 par3, s4 par4, s4 par5, s4 par6, s4 par7);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollbarPeer
 * Method:    connectJObject
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollbarPeer_connectJObject(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollbarPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollbarPeer
 * Method:    connectSignals
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollbarPeer_connectSignals(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollbarPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollbarPeer
 * Method:    setLineIncrement
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollbarPeer_setLineIncrement(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollbarPeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollbarPeer
 * Method:    setPageIncrement
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollbarPeer_setPageIncrement(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollbarPeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollbarPeer
 * Method:    setValues
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollbarPeer_setValues(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollbarPeer* this, s4 par1, s4 par2, s4 par3, s4 par4);

#endif

