/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKMAINTHREAD_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKMAINTHREAD_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkMainThread */

typedef struct gnu_java_awt_peer_gtk_GtkMainThread {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   s4 gtkInitCalled;
} gnu_java_awt_peer_gtk_GtkMainThread;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkMainThread
 * Method:    gtkInit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkMainThread_gtkInit(JNIEnv *env, jclass clazz, s4 par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkMainThread
 * Method:    gtkMain
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkMainThread_gtkMain(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkMainThread* this);

#endif

