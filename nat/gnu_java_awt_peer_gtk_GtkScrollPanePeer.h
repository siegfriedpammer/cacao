/* This file is machine generated, don't edit it !*/

/* Structure information for class: gnu/java/awt/peer/gtk/GtkScrollPanePeer */

typedef struct gnu_java_awt_peer_gtk_GtkScrollPanePeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
   struct java_awt_Container* c;
} gnu_java_awt_peer_gtk_GtkScrollPanePeer;

/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    create
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_create (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    gtkScrolledWindowNew
 * Signature: (Ljava/awt/peer/ComponentPeer;III[I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_gtkScrolledWindowNew (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this , struct java_awt_peer_ComponentPeer* par1, s4 par2, s4 par3, s4 par4, java_intarray* par5);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    gtkScrolledWindowSetScrollPosition
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_gtkScrolledWindowSetScrollPosition (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this , s4 par1, s4 par2);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    gtkScrolledWindowSetHScrollIncrement
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_gtkScrolledWindowSetHScrollIncrement (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this , s4 par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    gtkScrolledWindowSetVScrollIncrement
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_gtkScrolledWindowSetVScrollIncrement (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this , s4 par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    gtkScrolledWindowSetSize
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_gtkScrolledWindowSetSize (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this , s4 par1, s4 par2);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    setPolicy
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_setPolicy (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this , s4 par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    childResized
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_childResized (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this , s4 par1, s4 par2);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    getHScrollbarHeight
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_getHScrollbarHeight (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    getVScrollbarWidth
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_getVScrollbarWidth (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    setScrollPosition
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_setScrollPosition (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this , s4 par1, s4 par2);
