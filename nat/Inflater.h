/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/util/zip/Inflater */

typedef struct java_util_zip_Inflater {
   java_objectheader header;
   s8 strm;
   java_bytearray* buf;
   s4 off;
   s4 len;
   s4 finished;
   s4 needDict;
} java_util_zip_Inflater;

/*
 * Class:     java/util/zip/Inflater
 * Method:    end
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_util_zip_Inflater_end (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/Inflater
 * Method:    getAdler
 * Signature: (J)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_Inflater_getAdler (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/Inflater
 * Method:    getTotalIn
 * Signature: (J)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_Inflater_getTotalIn (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/Inflater
 * Method:    getTotalOut
 * Signature: (J)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_Inflater_getTotalOut (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/Inflater
 * Method:    inflateBytes
 * Signature: ([BII)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_Inflater_inflateBytes (JNIEnv *env ,  struct java_util_zip_Inflater* this , java_bytearray* par1, s4 par2, s4 par3);
/*
 * Class:     java/util/zip/Inflater
 * Method:    init
 * Signature: (Z)J
 */
JNIEXPORT s8 JNICALL Java_java_util_zip_Inflater_init (JNIEnv *env , s4 par1);
/*
 * Class:     java/util/zip/Inflater
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_util_zip_Inflater_initIDs (JNIEnv *env );
/*
 * Class:     java/util/zip/Inflater
 * Method:    reset
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_util_zip_Inflater_reset (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/Inflater
 * Method:    setDictionary
 * Signature: (J[BII)V
 */
JNIEXPORT void JNICALL Java_java_util_zip_Inflater_setDictionary (JNIEnv *env , s8 par1, java_bytearray* par2, s4 par3, s4 par4);
