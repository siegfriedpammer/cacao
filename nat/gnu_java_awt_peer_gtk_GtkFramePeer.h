/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKFRAMEPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKFRAMEPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkFramePeer */

typedef struct gnu_java_awt_peer_gtk_GtkFramePeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
   struct java_awt_Container* c;
   s4 hasBeenShown;
   s4 oldState;
   s4 menuBarHeight;
   struct java_awt_peer_MenuBarPeer* menuBar;
} gnu_java_awt_peer_gtk_GtkFramePeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFramePeer
 * Method:    getMenuBarHeight
 * Signature: (Ljava/awt/peer/MenuBarPeer;)I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkFramePeer_getMenuBarHeight(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFramePeer* this, struct java_awt_peer_MenuBarPeer* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFramePeer
 * Method:    setMenuBarPeer
 * Signature: (Ljava/awt/peer/MenuBarPeer;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFramePeer_setMenuBarPeer(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFramePeer* this, struct java_awt_peer_MenuBarPeer* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFramePeer
 * Method:    removeMenuBarPeer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFramePeer_removeMenuBarPeer(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFramePeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFramePeer
 * Method:    moveLayout
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFramePeer_moveLayout(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFramePeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFramePeer
 * Method:    gtkLayoutSetVisible
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFramePeer_gtkLayoutSetVisible(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFramePeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFramePeer
 * Method:    nativeSetIconImageFromDecoder
 * Signature: (Lgnu/java/awt/peer/gtk/GdkPixbufDecoder;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFramePeer_nativeSetIconImageFromDecoder(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFramePeer* this, struct gnu_java_awt_peer_gtk_GdkPixbufDecoder* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFramePeer
 * Method:    nativeSetIconImageFromData
 * Signature: ([III)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFramePeer_nativeSetIconImageFromData(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFramePeer* this, java_intarray* par1, s4 par2, s4 par3);

#endif

