/* This file is machine generated, don't edit it !*/

#ifndef _GNU_JAVA_AWT_PEER_GTK_GTKIMAGEPAINTER_H
#define _GNU_JAVA_AWT_PEER_GTK_GTKIMAGEPAINTER_H

/* Structure information for class: gnu/java/awt/peer/gtk/GtkImagePainter */

typedef struct gnu_java_awt_peer_gtk_GtkImagePainter {
   java_objectheader header;
   struct gnu_java_awt_peer_gtk_GtkImage* image;
   struct gnu_java_awt_peer_gtk_GdkGraphics* gc;
   s4 startX;
   s4 startY;
   s4 redBG;
   s4 greenBG;
   s4 blueBG;
   java_doublearray* affine;
   s4 width;
   s4 height;
   s4 flipX;
   s4 flipY;
   struct java_awt_Rectangle* clip;
   s4 s_width;
   s4 s_height;
} gnu_java_awt_peer_gtk_GtkImagePainter;


/*
 * Class:     gnu/java/awt/peer/gtk/GtkImagePainter
 * Method:    drawPixels
 * Signature: (Lgnu/java/awt/peer/gtk/GdkGraphics;IIIIIII[III[D)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GtkImagePainter_drawPixels(JNIEnv *env, struct gnu_java_awt_peer_gtk_GtkImagePainter* this, struct gnu_java_awt_peer_gtk_GdkGraphics* par1, s4 par2, s4 par3, s4 par4, s4 par5, s4 par6, s4 par7, s4 par8, java_intarray* par9, s4 par10, s4 par11, java_doublearray* par12);

#endif

