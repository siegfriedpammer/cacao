/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKTEXTAREAPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKTEXTAREAPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkTextAreaPeer */

typedef struct gnu_java_awt_peer_gtk_GtkTextAreaPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
} gnu_java_awt_peer_gtk_GtkTextAreaPeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextAreaPeer
 * Method:    create
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextAreaPeer_create(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkTextAreaPeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextAreaPeer
 * Method:    gtkSetFont
 * Signature: (Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextAreaPeer_gtkSetFont(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkTextAreaPeer* this, struct java_lang_String* par1, s4 par2, s4 par3);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextAreaPeer
 * Method:    gtkTextGetSize
 * Signature: ([I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextAreaPeer_gtkTextGetSize(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkTextAreaPeer* this, java_intarray* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextAreaPeer
 * Method:    insert
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextAreaPeer_insert(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkTextAreaPeer* this, struct java_lang_String* par1, s4 par2);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextAreaPeer
 * Method:    replaceRange
 * Signature: (Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextAreaPeer_replaceRange(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkTextAreaPeer* this, struct java_lang_String* par1, s4 par2, s4 par3);

#endif

