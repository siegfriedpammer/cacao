/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKBUTTONPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKBUTTONPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkButtonPeer */

typedef struct gnu_java_awt_peer_gtk_GtkButtonPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
} gnu_java_awt_peer_gtk_GtkButtonPeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkButtonPeer
 * Method:    create
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkButtonPeer_create(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkButtonPeer* this, struct java_lang_String* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkButtonPeer
 * Method:    connectJObject
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkButtonPeer_connectJObject(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkButtonPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkButtonPeer
 * Method:    connectSignals
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkButtonPeer_connectSignals(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkButtonPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkButtonPeer
 * Method:    gtkSetFont
 * Signature: (Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkButtonPeer_gtkSetFont(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkButtonPeer* this, struct java_lang_String* par1, s4 par2, s4 par3);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkButtonPeer
 * Method:    gtkSetLabel
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkButtonPeer_gtkSetLabel(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkButtonPeer* this, struct java_lang_String* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkButtonPeer
 * Method:    gtkWidgetSetForeground
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkButtonPeer_gtkWidgetSetForeground(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkButtonPeer* this, s4 par1, s4 par2, s4 par3);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkButtonPeer
 * Method:    gtkActivate
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkButtonPeer_gtkActivate(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkButtonPeer* this);

#endif

