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
} gnu_java_awt_peer_gtk_GtkFramePeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFramePeer
 * Method:    getMenuBarHeight
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkFramePeer_getMenuBarHeight(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFramePeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFramePeer
 * Method:    setMenuBarPeer
 * Signature: (Ljava/awt/peer/MenuBarPeer;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFramePeer_setMenuBarPeer(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFramePeer* this, struct java_awt_peer_MenuBarPeer* par1);

#endif

