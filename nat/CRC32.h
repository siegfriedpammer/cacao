/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/util/zip/CRC32 */

typedef struct java_util_zip_CRC32 {
   java_objectheader header;
   s4 crc;
} java_util_zip_CRC32;

/*
 * Class:     java/util/zip/CRC32
 * Method:    update
 * Signature: (II)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_CRC32_update (JNIEnv *env , s4 par1, s4 par2);
/*
 * Class:     java/util/zip/CRC32
 * Method:    updateBytes
 * Signature: (I[BII)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_CRC32_updateBytes (JNIEnv *env , s4 par1, java_bytearray* par2, s4 par3, s4 par4);
