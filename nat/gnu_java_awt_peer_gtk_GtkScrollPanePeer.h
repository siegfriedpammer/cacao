/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKSCROLLPANEPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKSCROLLPANEPEER_H

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
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_create(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this, s4 par1, s4 par2);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    gtkScrolledWindowSetScrollPosition
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_gtkScrolledWindowSetScrollPosition(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this, s4 par1, s4 par2);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    gtkScrolledWindowSetHScrollIncrement
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_gtkScrolledWindowSetHScrollIncrement(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    gtkScrolledWindowSetVScrollIncrement
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_gtkScrolledWindowSetVScrollIncrement(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    setPolicy
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_setPolicy(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    getHScrollbarHeight
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_getHScrollbarHeight(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    getVScrollbarWidth
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_getVScrollbarWidth(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkScrollPanePeer
 * Method:    setScrollPosition
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkScrollPanePeer_setScrollPosition(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkScrollPanePeer* this, s4 par1, s4 par2);

#endif

