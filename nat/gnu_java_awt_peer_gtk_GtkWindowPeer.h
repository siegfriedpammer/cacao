/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKWINDOWPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKWINDOWPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkWindowPeer */

typedef struct gnu_java_awt_peer_gtk_GtkWindowPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
   struct java_awt_Container* c;
   s4 hasBeenShown;
   s4 oldState;
} gnu_java_awt_peer_gtk_GtkWindowPeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkWindowPeer
 * Method:    create
 * Signature: (IZIILgnu/java/awt/peer/gtk/GtkWindowPeer;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkWindowPeer_create(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkWindowPeer* this, s4 par1, s4 par2, s4 par3, s4 par4, struct gnu_java_awt_peer_gtk_GtkWindowPeer* par5);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkWindowPeer
 * Method:    connectHooks
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkWindowPeer_connectHooks(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkWindowPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkWindowPeer
 * Method:    toBack
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkWindowPeer_toBack(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkWindowPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkWindowPeer
 * Method:    toFront
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkWindowPeer_toFront(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkWindowPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkWindowPeer
 * Method:    nativeSetBounds
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkWindowPeer_nativeSetBounds(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkWindowPeer* this, s4 par1, s4 par2, s4 par3, s4 par4);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkWindowPeer
 * Method:    setSize
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkWindowPeer_setSize(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkWindowPeer* this, s4 par1, s4 par2);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkWindowPeer
 * Method:    setBoundsCallback
 * Signature: (Ljava/awt/Window;IIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkWindowPeer_setBoundsCallback(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkWindowPeer* this, struct java_awt_Window* par1, s4 par2, s4 par3, s4 par4, s4 par5);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkWindowPeer
 * Method:    nativeSetVisible
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkWindowPeer_nativeSetVisible(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkWindowPeer* this, s4 par1);

#endif

