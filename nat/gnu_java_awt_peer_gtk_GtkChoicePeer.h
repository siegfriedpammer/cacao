/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKCHOICEPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKCHOICEPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkChoicePeer */

typedef struct gnu_java_awt_peer_gtk_GtkChoicePeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
} gnu_java_awt_peer_gtk_GtkChoicePeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkChoicePeer
 * Method:    create
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkChoicePeer_create(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkChoicePeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkChoicePeer
 * Method:    append
 * Signature: ([Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkChoicePeer_append(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkChoicePeer* this, java_objectarray* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkChoicePeer
 * Method:    getHistory
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkChoicePeer_getHistory(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkChoicePeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkChoicePeer
 * Method:    nativeAdd
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkChoicePeer_nativeAdd(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkChoicePeer* this, struct java_lang_String* par1, s4 par2);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkChoicePeer
 * Method:    nativeRemove
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkChoicePeer_nativeRemove(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkChoicePeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkChoicePeer
 * Method:    select
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkChoicePeer_select(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkChoicePeer* this, s4 par1);

#endif

