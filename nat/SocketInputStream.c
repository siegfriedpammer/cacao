/* class: java/net/SocketInputStream */

/*
 * Class:     java/net/SocketInputStream
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_SocketInputStream_init (JNIEnv *env )
{
    if (runverbose)
	log_text("Java_java_net_SocketInputStream_initProto called");
}

/*
 * Class:     java/net/SocketInputStream
 * Method:    socketRead
 * Signature: ([BII)I
 */
JNIEXPORT s4 JNICALL
Java_java_net_SocketInputStream_socketRead (JNIEnv *env,  struct java_net_SocketInputStream* this, java_bytearray* buf, s4 offset, s4 len)
{
	int r;

	if (runverbose)
	    log_text("Java_java_net_SocketInputStream_socketRead called");
    
#ifdef USE_THREADS
	assert(blockInts == 0);
#endif

	r = threadedRead(this->impl->fd->fd, &buf->data[offset], len);
	if (r < 0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return 0;
	}
	else if (r == 0) {
		return (-1);	/* EOF */
	}
	else {
		return (r);
	}

#ifdef USE_THREADS
	assert(blockInts == 0);
#endif

	return 0;
}
