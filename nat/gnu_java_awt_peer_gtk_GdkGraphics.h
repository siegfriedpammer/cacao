/* This file is machine generated, don't edit it !*/

/* Structure information for class: gnu/java/awt/peer/gtk/GdkGraphics */

typedef struct gnu_java_awt_peer_gtk_GdkGraphics {
   java_objectheader header;
   s4 native_state;
   struct java_awt_Color* color;
   struct java_awt_Color* xorColor;
   struct gnu_java_awt_peer_gtk_GtkComponentPeer* component;
   struct java_awt_Font* font;
   struct java_awt_Rectangle* clip;
   s4 xOffset;
   s4 yOffset;
} gnu_java_awt_peer_gtk_GdkGraphics;

/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    initState
 * Signature: (Lgnu/java/awt/peer/gtk/GtkComponentPeer;)[I
 */
JNIEXPORT java_intarray* JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_initState (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , struct gnu_java_awt_peer_gtk_GtkComponentPeer* par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    initState
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics0_initState (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    copyState
 * Signature: (Lgnu/java/awt/peer/gtk/GdkGraphics;)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_copyState (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , struct gnu_java_awt_peer_gtk_GdkGraphics* par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    clearRect
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_clearRect (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2, s4 par3, s4 par4);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    copyArea
 * Signature: (IIIIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_copyArea (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2, s4 par3, s4 par4, s4 par5, s4 par6);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    dispose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_dispose (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this );
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    copyPixmap
 * Signature: (Ljava/awt/Graphics;IIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_copyPixmap (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , struct java_awt_Graphics* par1, s4 par2, s4 par3, s4 par4, s4 par5);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    drawLine
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_drawLine (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2, s4 par3, s4 par4);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    drawArc
 * Signature: (IIIIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_drawArc (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2, s4 par3, s4 par4, s4 par5, s4 par6);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    fillArc
 * Signature: (IIIIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_fillArc (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2, s4 par3, s4 par4, s4 par5, s4 par6);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    drawOval
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_drawOval (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2, s4 par3, s4 par4);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    fillOval
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_fillOval (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2, s4 par3, s4 par4);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    drawPolygon
 * Signature: ([I[II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_drawPolygon (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , java_intarray* par1, java_intarray* par2, s4 par3);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    fillPolygon
 * Signature: ([I[II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_fillPolygon (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , java_intarray* par1, java_intarray* par2, s4 par3);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    drawPolyline
 * Signature: ([I[II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_drawPolyline (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , java_intarray* par1, java_intarray* par2, s4 par3);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    drawRect
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_drawRect (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2, s4 par3, s4 par4);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    fillRect
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_fillRect (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2, s4 par3, s4 par4);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    drawString
 * Signature: (Ljava/lang/String;IILjava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_drawString (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , struct java_lang_String* par1, s4 par2, s4 par3, struct java_lang_String* par4, s4 par5);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    setClipRectangle
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_setClipRectangle (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2, s4 par3, s4 par4);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    setFGColor
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_setFGColor (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2, s4 par3);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    setFunction
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_setFunction (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1);
/*
 * Class:     gnu/java/awt/peer/gtk/GdkGraphics
 * Method:    translateNative
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_gnu_java_awt_peer_gtk_GdkGraphics_translateNative (JNIEnv *env ,  struct gnu_java_awt_peer_gtk_GdkGraphics* this , s4 par1, s4 par2);
