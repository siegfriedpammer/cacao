/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/util/zip/Deflater */

typedef struct java_util_zip_Deflater {
   java_objectheader header;
   s8 strm;
   java_bytearray* buf;
   s4 off;
   s4 len;
   s4 level;
   s4 strategy;
   s4 setParams;
   s4 finish;
   s4 finished;
} java_util_zip_Deflater;

/*
 * Class:     java/util/zip/Deflater
 * Method:    deflateBytes
 * Signature: ([BII)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_Deflater_deflateBytes (JNIEnv *env ,  struct java_util_zip_Deflater* this , java_bytearray* par1, s4 par2, s4 par3);
/*
 * Class:     java/util/zip/Deflater
 * Method:    end
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_util_zip_Deflater_end (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/Deflater
 * Method:    getAdler
 * Signature: (J)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_Deflater_getAdler (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/Deflater
 * Method:    getTotalIn
 * Signature: (J)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_Deflater_getTotalIn (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/Deflater
 * Method:    getTotalOut
 * Signature: (J)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_Deflater_getTotalOut (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/Deflater
 * Method:    init
 * Signature: (IIZ)J
 */
JNIEXPORT s8 JNICALL Java_java_util_zip_Deflater_init (JNIEnv *env , s4 par1, s4 par2, s4 par3);
/*
 * Class:     java/util/zip/Deflater
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_util_zip_Deflater_initIDs (JNIEnv *env );
/*
 * Class:     java/util/zip/Deflater
 * Method:    reset
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_util_zip_Deflater_reset (JNIEnv *env , s8 par1);
/*
 * Class:     java/util/zip/Deflater
 * Method:    setDictionary
 * Signature: (J[BII)V
 */
JNIEXPORT void JNICALL Java_java_util_zip_Deflater_setDictionary (JNIEnv *env , s8 par1, java_bytearray* par2, s4 par3, s4 par4);
