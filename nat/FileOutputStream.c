/* class: java/io/FileOutputStream */

/*
 * Class:     java/io/FileOutputStream
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_FileOutputStream_close ( JNIEnv *env ,  struct java_io_FileOutputStream* this)
{
	if (this->fd->fd == fileno(stderr))  /* don't close stderr!!! -- phil. */
		return;

	if (this->fd->fd == fileno(stdout))
		return;

	if (this->fd->fd >= 0) {
		s4 r = close (this->fd->fd);
		this->fd->fd = -1;
		if (r<0) 
			exceptionptr = native_new_and_init (class_java_io_IOException);
		}
}

/*
 * Class:     java/io/FileOutputStream
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_FileOutputStream_initIDs ( JNIEnv *env  )
{
    /* no IDs required */
}

/*
 * Class:     java/io/FileOutputStream
 * Method:    open
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_java_io_FileOutputStream_open ( JNIEnv *env ,  struct java_io_FileOutputStream* this, struct java_lang_String* name)
{
	s4 fd;
	char *fname = javastring_tochar ((java_objectheader*)name);
	if (!fname) goto fail;
	
	fd = creat (fname, 0666);
	if (fd<0) goto fail;
	
	threadedFileDescriptor(fd);

	this->fd->fd = fd;
	return;

	fail:
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
}

/*
 * Class:     java/io/FileOutputStream
 * Method:    openAppend
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_java_io_FileOutputStream_openAppend ( JNIEnv *env ,  struct java_io_FileOutputStream* this, struct java_lang_String* name)
{
	s4 fd;
	char *fname = javastring_tochar ((java_objectheader*)name);
	if (!fname) goto fail;
	
	fd = open (fname, O_APPEND, 0666);
	if (fd<0) goto fail;
	
	threadedFileDescriptor(fd);

	this->fd->fd = fd;
	return;

	fail:
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
}

/*
 * Class:     java/io/FileOutputStream
 * Method:    write
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_io_FileOutputStream_write ( JNIEnv *env ,  struct java_io_FileOutputStream* this, s4 byte)
{
	u1 buffer[1];
	s4 l;

	buffer[0] = byte;
	l = threadedWrite (this->fd->fd, (char *) buffer, 1);

	if (l<1) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		}
}

/*
 * Class:     java/io/FileOutputStream
 * Method:    writeBytes
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL Java_java_io_FileOutputStream_writeBytes ( JNIEnv *env ,  struct java_io_FileOutputStream* this, java_bytearray* buffer, s4 start, s4 len)
{
	s4 o;

	if (len == 0)
		return;
	o = threadedWrite (this->fd->fd, (char *) buffer->data+start, len);
	if (o!=len) 
		exceptionptr = native_new_and_init (class_java_io_IOException);
}



