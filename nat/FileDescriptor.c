/* class: java/io/FileDescriptor */

/*
 * Class:     java/io/FileDescriptor
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_FileDescriptor_initIDs ( JNIEnv *env  )
{
    /* empty since no JNI field IDs required */
}

/*
 * Class:     java/io/FileDescriptor
 * Method:    sync
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_FileDescriptor_sync ( JNIEnv *env ,  struct java_io_FileDescriptor* this)
{
    if (fsync(this->fd)==-1)
	exceptionptr = native_new_and_init (class_java_io_SyncFailedException);
}

