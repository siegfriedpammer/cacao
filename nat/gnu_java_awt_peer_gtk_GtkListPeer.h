/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKLISTPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKLISTPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkListPeer */

typedef struct gnu_java_awt_peer_gtk_GtkListPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
} gnu_java_awt_peer_gtk_GtkListPeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    create
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_create(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    connectJObject
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_connectJObject(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    connectSignals
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_connectSignals(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    gtkSetFont
 * Signature: (Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_gtkSetFont(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this, struct java_lang_String* par1, s4 par2, s4 par3);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    gtkWidgetRequestFocus
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_gtkWidgetRequestFocus(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    getSize
 * Signature: (II[I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_getSize(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this, s4 par1, s4 par2, java_intarray* par3);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    append
 * Signature: ([Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_append(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this, java_objectarray* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    add
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_add(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this, struct java_lang_String* par1, s4 par2);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    delItems
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_delItems(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this, s4 par1, s4 par2);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    deselect
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_deselect(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    getSelectedIndexes
 * Signature: ()[I
 */
JNIEXPORT java_intarray* JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_getSelectedIndexes(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    makeVisible
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_makeVisible(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    select
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_select(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkListPeer
 * Method:    setMultipleMode
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkListPeer_setMultipleMode(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkListPeer* this, s4 par1);

#endif

