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
   struct java_io_FilenameFilter* filter;
} gnu_java_awt_peer_gtk_GtkFileDialogPeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFileDialogPeer
 * Method:    create
 * Signature: (Lgnu/java/awt/peer/gtk/GtkContainerPeer;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFileDialogPeer_create(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFileDialogPeer* this, struct gnu_java_awt_peer_gtk_GtkContainerPeer* par1);


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


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFileDialogPeer
 * Method:    nativeGetDirectory
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_gnu_java_awt_peer_gtk_GtkFileDialogPeer_nativeGetDirectory(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFileDialogPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFileDialogPeer
 * Method:    nativeSetDirectory
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFileDialogPeer_nativeSetDirectory(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFileDialogPeer* this, struct java_lang_String* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkFileDialogPeer
 * Method:    nativeSetFilenameFilter
 * Signature: (Ljava/io/FilenameFilter;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkFileDialogPeer_nativeSetFilenameFilter(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkFileDialogPeer* this, struct java_io_FilenameFilter* par1);

#endif

