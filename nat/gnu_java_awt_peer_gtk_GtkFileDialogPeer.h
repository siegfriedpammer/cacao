/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKFILEDIALOGPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKFILEDIALOGPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkFileDialogPeer */

typedef struct gnu_java_awt_peer_gtk_GtkFileDialogPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
   struct java_awt_Container* c;
   s4 hasBeenShown;
   s4 oldState;
   struct java_lang_String* currentFile;
   struct java_lang_String* currentDirectory;
} gnu_java_awt_peer_gtk_GtkFileDialogPeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFileDialogPeer
 * Method:    create
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFileDialogPeer_create(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFileDialogPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFileDialogPeer
 * Method:    connectJObject
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFileDialogPeer_connectJObject(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFileDialogPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFileDialogPeer
 * Method:    connectSignals
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFileDialogPeer_connectSignals(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFileDialogPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFileDialogPeer
 * Method:    nativeSetFile
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFileDialogPeer_nativeSetFile(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFileDialogPeer* this, struct java_lang_String* par1);

#endif

