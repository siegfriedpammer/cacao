/* This file is machine generated, don't edit it !*/

/* Structure information for class: gnu/java/awt/peer/gtk/GtkToolkit */

typedef struct gnu_java_awt_peer_gtk_GtkToolkit {
   java_objectheader header;
   struct java_util_Map* desktopProperties;
   struct java_beans_PropertyChangeSupport* desktopPropsSupport;
   struct gnu_java_awt_peer_gtk_GtkMainThread* main;
   struct java_util_Hashtable* containers;
} gnu_java_awt_peer_gtk_GtkToolkit;

/*
 * Class:     gnu/java/awt/peer/gtk/GtkToolkit
 * Method:    beep
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkToolkit_beep (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkToolkit* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkToolkit
 * Method:    getScreenSizeDimensions
 * Signature: ([I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkToolkit_getScreenSizeDimensions (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkToolkit* this , java_intarray* par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GtkToolkit
 * Method:    getScreenResolution
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_java_awt_peer_gtk_GtkToolkit_getScreenResolution (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkToolkit* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GtkToolkit
 * Method:    sync
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkToolkit_sync (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GtkToolkit* this );
