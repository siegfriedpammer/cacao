/* class: java/io/FileSystem */

/*
 * Class:     java/io/FileSystem
 * Method:    getFileSystem
 * Signature: ()Ljava/io/FileSystem;
 */
JNIEXPORT struct java_io_FileSystem* JNICALL Java_java_io_FileSystem_getFileSystem ( JNIEnv *env  )
{
   java_io_FileSystem*  o = (java_io_FileSystem*) native_new_and_init(class_java_io_UnixFileSystem);
   return o;
}
