/* This file is machine generated, don't edit it !*/

/* Structure information for class: gnu/java/awt/peer/gtk/GtkComponentPeer */

typedef struct gnu_java_awt_peer_gtk_GtkComponentPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
} gnu_java_awt_peer_gtk_GtkComponentPeer;

/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    isEnabled
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_isEnabled (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    modalHasGrab
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_modalHasGrab (JNIEnv *env , jclass clazz );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    gtkWidgetGetForeground
 * Signature: ()[I
 */
JNIEXPORT java_intarray* JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_gtkWidgetGetForeground (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    gtkWidgetGetBackground
 * Signature: ()[I
 */
JNIEXPORT java_intarray* JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_gtkWidgetGetBackground (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    gtkWidgetSetVisible
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_gtkWidgetSetVisible (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this , s4 par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    gtkWidgetGetDimensions
 * Signature: ([I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_gtkWidgetGetDimensions (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this , java_intarray* par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    gtkWidgetGetLocationOnScreen
 * Signature: ([I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_gtkWidgetGetLocationOnScreen (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this , java_intarray* par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    gtkWidgetSetCursor
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_gtkWidgetSetCursor (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this , s4 par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    gtkWidgetSetBackground
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_gtkWidgetSetBackground (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this , s4 par1, s4 par2, s4 par3);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    gtkWidgetSetForeground
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_gtkWidgetSetForeground (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this , s4 par1, s4 par2, s4 par3);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    connectHooks
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_connectHooks (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    requestFocus
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_requestFocus (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    setNativeBounds
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_setNativeBounds (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this , s4 par1, s4 par2, s4 par3, s4 par4);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    set
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer_set (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this , struct java_lang_String* par1, struct java_lang_String* par2);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    set
 * Signature: (Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer0_set (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this , struct java_lang_String* par1, s4 par2);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    set
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer1_set (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this , struct java_lang_String* par1, s4 par2);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    set
 * Signature: (Ljava/lang/String;F)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer2_set (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this , struct java_lang_String* par1, float par2);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkComponentPeer
 * Method:    set
 * Signature: (Ljava/lang/String;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkComponentPeer3_set (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkComponentPeer* this , struct java_lang_String* par1, struct java_lang_Object* par2);
