/* This file is machine generated, don't edit it !*/

/* Structure information for class: gnu/java/awt/peer/gtk/GtkClipboard */

typedef struct gnu_java_awt_peer_gtk_GtkClipboard {
   java_objectheader header;
   struct java_awt_datatransfer_Transferable* contents;
   struct java_awt_datatransfer_ClipboardOwner* owner;
   struct java_lang_String* name;
} gnu_java_awt_peer_gtk_GtkClipboard;

/*
 * Class:     gnu/java/awt/peer/gtk/GtkClipboard
 * Method:    initNativeState
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkClipboard_initNativeState (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkClipboard* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkClipboard
 * Method:    requestStringConversion
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkClipboard_requestStringConversion (JNIEnv *env , jclass clazz );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkClipboard
 * Method:    selectionGet
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkClipboard_selectionGet (JNIEnv *env , jclass clazz );
