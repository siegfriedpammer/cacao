/* class: java/net/SocketOutputStream */

/*
 * Class:     java/net/SocketOutputStream
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_SocketOutputStream_init (JNIEnv *env )
{
    if (runverbose)
	log_text("Java_java_net_SocketOutputStream_init called");
}

/*
 * Class:     java/net/SocketOutputStream
 * Method:    socketWrite
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL
Java_java_net_SocketOutputStream_socketWrite (JNIEnv *env,  struct java_net_SocketOutputStream* this, java_bytearray* buf, s4 offset, s4 len)
{
	int r;

	if (runverbose)
	    log_text("Java_java_net_SocketOutputStream_socketWrite called");

#ifdef USE_THREADS
	assert(blockInts == 0);
#endif

	if (this->impl->fd->fd < 0) {
		/* exceptionptr = native_new_and_init (class_java_io_IOException); */
		return;
	}
	r = threadedWrite(this->impl->fd->fd, &buf->data[offset], len);
	if (r < 0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
	}
	assert(r == len);

#ifdef USE_THREADS
	assert(blockInts == 0);
#endif
}
