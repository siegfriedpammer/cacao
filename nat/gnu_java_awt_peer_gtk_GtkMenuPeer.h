/* This file is machine generated, don't edit it !*/

/* Structure information for class: gnu/java/awt/peer/gtk/GtkMenuPeer */

typedef struct gnu_java_awt_peer_gtk_GtkMenuPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
} gnu_java_awt_peer_gtk_GtkMenuPeer;

/*
 * Class:     gnu/java/awt/peer/gtk/GtkMenuPeer
 * Method:    create
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkMenuPeer_create (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkMenuPeer* this , struct java_lang_String* par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkMenuPeer
 * Method:    addItem
 * Signature: (Ljava/awt/peer/MenuItemPeer;IZ)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkMenuPeer_addItem (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkMenuPeer* this , struct java_awt_peer_MenuItemPeer* par1, s4 par2, s4 par3);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkMenuPeer
 * Method:    setupAccelGroup
 * Signature: (Lgnu/java/awt/peer/gtk/GtkGenericPeer;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkMenuPeer_setupAccelGroup (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkMenuPeer* this , struct gnu_java_awt_peer_gtk_GtkGenericPeer* par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkMenuPeer
 * Method:    delItem
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkMenuPeer_delItem (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkMenuPeer* this , s4 par1);
