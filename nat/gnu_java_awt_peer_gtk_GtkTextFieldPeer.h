/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKTEXTFIELDPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKTEXTFIELDPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkTextFieldPeer */

typedef struct gnu_java_awt_peer_gtk_GtkTextFieldPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
} gnu_java_awt_peer_gtk_GtkTextFieldPeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextFieldPeer
 * Method:    create
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextFieldPeer_create(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkTextFieldPeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextFieldPeer
 * Method:    gtkEntryGetBorderWidth
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkTextFieldPeer_gtkEntryGetBorderWidth(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkTextFieldPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextFieldPeer
 * Method:    gtkSetFont
 * Signature: (Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextFieldPeer_gtkSetFont(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkTextFieldPeer* this, struct java_lang_String* par1, s4 par2, s4 par3);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextFieldPeer
 * Method:    setEchoChar
 * Signature: (C)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextFieldPeer_setEchoChar(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkTextFieldPeer* this, s4 par1);

#endif

