/* class: java/util/zip/ZipEntry */

/*
 * Class:     java/util/zip/ZipEntry
 * Method:    initFields
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_util_zip_ZipEntry_initFields ( JNIEnv *env ,  struct java_util_zip_ZipEntry* this, s8 par1)
{
  log_text("Java_java_util_zip_ZipEntry_initFields  called");
}
/*
 * Class:     java/util/zip/ZipEntry
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_util_zip_ZipEntry_initIDs ( JNIEnv *env  )
{
  if (verbose)
    log_text("Java_java_util_zip_ZipEntry_initIDs  called");
}

