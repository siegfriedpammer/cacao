/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKCHECKBOXPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKCHECKBOXPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkCheckboxPeer */

typedef struct gnu_java_awt_peer_gtk_GtkCheckboxPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
   struct gnu_java_awt_peer_gtk_GtkCheckboxGroupPeer* old_group;
} gnu_java_awt_peer_gtk_GtkCheckboxPeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkCheckboxPeer
 * Method:    nativeCreate
 * Signature: (Lgnu/java/awt/peer/gtk/GtkCheckboxGroupPeer;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkCheckboxPeer_nativeCreate(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkCheckboxPeer* this, struct gnu_java_awt_peer_gtk_GtkCheckboxGroupPeer* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkCheckboxPeer
 * Method:    nativeSetCheckboxGroup
 * Signature: (Lgnu/java/awt/peer/gtk/GtkCheckboxGroupPeer;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkCheckboxPeer_nativeSetCheckboxGroup(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkCheckboxPeer* this, struct gnu_java_awt_peer_gtk_GtkCheckboxGroupPeer* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkCheckboxPeer
 * Method:    connectHooks
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkCheckboxPeer_connectHooks(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkCheckboxPeer* this);

#endif

