/* class: java/util/zip/Adler32 */

/*
 * Class:     java/util/zip/Adler32
 * Method:    update
 * Signature: (II)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_Adler32_update ( JNIEnv *env ,  s4 par1, s4 par2)
{
  /* Adler-32 checksum not implemented */
  log_text("Java_java_util_zip_Adler32_update called");
  return 0;
}

/*
 * Class:     java/util/zip/Adler32
 * Method:    updateBytes
 * Signature: (I[BII)I
 */
JNIEXPORT s4 JNICALL Java_java_util_zip_Adler32_updateBytes ( JNIEnv *env ,  s4 par1, java_bytearray* par2, s4 par3, s4 par4)
{
  /* Adler-32 checksum not implemented */
  log_text("Java_java_util_zip_Adler32_updateBytes  called");
  return 0;
}
