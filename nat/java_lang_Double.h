/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/Double */

typedef struct java_lang_Double {
   java_objectheader header;
   double value;
} java_lang_Double;

/*
 * Class:     java/lang/Double
 * Method:    parseDouble
 * Signature: (Ljava/lang/String;)D
 */
JNIEXPORT double JNICALL Java_java_lang_Double_parseDouble (JNIEnv *env , jclass clazz , struct java_lang_String* par1);
/*
 * Class:     java/lang/Double
 * Method:    toString
 * Signature: (DZ)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_Double_toString (JNIEnv *env , jclass clazz , double par1, s4 par2);
/*
 * Class:     java/lang/Double
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Double_initIDs (JNIEnv *env , jclass clazz );
