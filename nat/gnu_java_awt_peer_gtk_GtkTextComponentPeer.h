/* This file is machine generated, don't edit it !*/

/* Structure information for class: gnu/java/awt/peer/gtk/GtkTextComponentPeer */

typedef struct gnu_java_awt_peer_gtk_GtkTextComponentPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
} gnu_java_awt_peer_gtk_GtkTextComponentPeer;

/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextComponentPeer
 * Method:    connectHooks
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextComponentPeer_connectHooks (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkTextComponentPeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextComponentPeer
 * Method:    getCaretPosition
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkTextComponentPeer_getCaretPosition (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkTextComponentPeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextComponentPeer
 * Method:    setCaretPosition
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextComponentPeer_setCaretPosition (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkTextComponentPeer* this , s4 par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextComponentPeer
 * Method:    getSelectionStart
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkTextComponentPeer_getSelectionStart (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkTextComponentPeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextComponentPeer
 * Method:    getSelectionEnd
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkTextComponentPeer_getSelectionEnd (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkTextComponentPeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextComponentPeer
 * Method:    getText
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_gnu_java_awt_peer_gtk_GtkTextComponentPeer_getText (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkTextComponentPeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextComponentPeer
 * Method:    select
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextComponentPeer_select (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkTextComponentPeer* this , s4 par1, s4 par2);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextComponentPeer
 * Method:    setEditable
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextComponentPeer_setEditable (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkTextComponentPeer* this , s4 par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkTextComponentPeer
 * Method:    setText
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkTextComponentPeer_setText (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkTextComponentPeer* this , struct java_lang_String* par1);
