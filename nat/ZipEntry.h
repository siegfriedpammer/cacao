/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/util/zip/ZipEntry */

typedef struct java_util_zip_ZipEntry {
   java_objectheader header;
   struct java_lang_String* name;
   s8 time;
   s8 crc;
   s8 size;
   s8 csize;
   s4 method;
   java_bytearray* extra;
   struct java_lang_String* comment;
   s4 flag;
   s4 version;
   s8 offset;
} java_util_zip_ZipEntry;

/*
 * Class:     java/util/zip/ZipEntry
 * Method:    initFields
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_util_zip_ZipEntry_initFields (JNIEnv *env ,  struct java_util_zip_ZipEntry* this , s8 par1);
/*
 * Class:     java/util/zip/ZipEntry
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_util_zip_ZipEntry_initIDs (JNIEnv *env );
