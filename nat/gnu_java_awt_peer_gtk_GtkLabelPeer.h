/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKLABELPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKLABELPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkLabelPeer */

typedef struct gnu_java_awt_peer_gtk_GtkLabelPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
} gnu_java_awt_peer_gtk_GtkLabelPeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkLabelPeer
 * Method:    create
 * Signature: (Ljava/lang/String;F)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkLabelPeer_create(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkLabelPeer* this, struct java_lang_String* par1, float par2);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkLabelPeer
 * Method:    setText
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkLabelPeer_setText(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkLabelPeer* this, struct java_lang_String* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkLabelPeer
 * Method:    nativeSetAlignment
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkLabelPeer_nativeSetAlignment(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkLabelPeer* this, float par1);

#endif

