/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKCHECKBOXPEER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKCHECKBOXPEER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkCheckboxPeer */

typedef struct gnu_java_awt_peer_gtk_GtkCheckboxPeer {
   java_objectheader header;
   s4 native_state;
   struct java_lang_Object* awtWidget;
   struct java_awt_Component* awtComponent;
   struct java_awt_Insets* insets;
   struct gnu_java_awt_peer_gtk_GtkCheckboxGroupPeer* old_group;
   s4 currentState;
} gnu_java_awt_peer_gtk_GtkCheckboxPeer;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkCheckboxPeer
 * Method:    create
 * Signature: (Lgnu/java/awt/peer/gtk/GtkCheckboxGroupPeer;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkCheckboxPeer_create(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkCheckboxPeer* this, struct gnu_java_awt_peer_gtk_GtkCheckboxGroupPeer* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkCheckboxPeer
 * Method:    nativeSetCheckboxGroup
 * Signature: (Lgnu/java/awt/peer/gtk/GtkCheckboxGroupPeer;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkCheckboxPeer_nativeSetCheckboxGroup(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkCheckboxPeer* this, struct gnu_java_awt_peer_gtk_GtkCheckboxGroupPeer* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkCheckboxPeer
 * Method:    connectSignals
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkCheckboxPeer_connectSignals(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkCheckboxPeer* this);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkCheckboxPeer
 * Method:    gtkSetFont
 * Signature: (Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkCheckboxPeer_gtkSetFont(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkCheckboxPeer* this, struct java_lang_String* par1, s4 par2, s4 par3);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkCheckboxPeer
 * Method:    gtkButtonSetLabel
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkCheckboxPeer_gtkButtonSetLabel(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkCheckboxPeer* this, struct java_lang_String* par1);


/*
 * Class:     gnu/java/awt/peer/gtk/GtkCheckboxPeer
 * Method:    gtkToggleButtonSetActive
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkCheckboxPeer_gtkToggleButtonSetActive(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkCheckboxPeer* this, s4 par1);

#endif

