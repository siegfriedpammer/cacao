/* class: java/io/FileInputStream */

/*
 * Class:     java/io/FileInputStream
 * Method:    available
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_io_FileInputStream_available ( JNIEnv *env ,  struct java_io_FileInputStream* this)
{
	struct stat buffer;
	s4 r1,r2;
	
	r1 = fstat (this->fd->fd, &buffer);
	r2 = lseek(this->fd->fd, 0, SEEK_CUR);
	
	if ( (r1 >= 0) && (r2 >= 0) )  
		return buffer.st_size - r2; 

	exceptionptr = native_new_and_init (class_java_io_IOException);
	return 0;
}
/*
 * Class:     java/io/FileInputStream
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_FileInputStream_close ( JNIEnv *env ,  struct java_io_FileInputStream* this)
{
	if (this->fd->fd >= 0) {
		s4 r = close (this->fd->fd);
		this->fd->fd = -1;
		if (r < 0) 
			exceptionptr = native_new_and_init (class_java_io_IOException);
	}
}
/*
 * Class:     java/io/FileInputStream
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_FileInputStream_initIDs ( JNIEnv *env  )
{
    /* no IDs required */
}

/*
 * Class:     java/io/FileInputStream
 * Method:    open
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_java_io_FileInputStream_open ( JNIEnv *env ,  struct java_io_FileInputStream* this, struct java_lang_String* name)
{
	s4 fd;
	char *fname = javastring_tochar ((java_objectheader*)name);

	if (!fname) goto fail;

	fd = open (fname, O_RDONLY, 0);
	if (fd<0) goto fail;
	
	threadedFileDescriptor(fd);

	this->fd->fd = fd;
	return;

	fail:
	        printf("failed to open: %s\n",fname);	       

		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
}

/*
 * Class:     java/io/FileInputStream
 * Method:    read
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_io_FileInputStream_read ( JNIEnv *env ,  struct java_io_FileInputStream* this)
{
	s4 r;
	u1 buffer[1];
	r = threadedRead (this->fd->fd, (char *) buffer, 1);	

	if (r>0) return buffer[0];
	if (r==0) return -1;
	
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return 0;
}
/*
 * Class:     java/io/FileInputStream
 * Method:    readBytes
 * Signature: ([BII)I
 */
JNIEXPORT s4 JNICALL Java_java_io_FileInputStream_readBytes ( JNIEnv *env ,  struct java_io_FileInputStream* this, java_bytearray* buffer, s4 start, s4 len)
{
	s4 ret = threadedRead (this->fd->fd, (char *) buffer->data+start, len);
	if (ret>0) return ret;
	if (ret==0) return -1;
	
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return 0;
}
/*
 * Class:     java/io/FileInputStream
 * Method:    skip
 * Signature: (J)J
 */
JNIEXPORT s8 JNICALL Java_java_io_FileInputStream_skip ( JNIEnv *env ,  struct java_io_FileInputStream* this, s8 numbytes)
{
	s4 ret = lseek (this->fd->fd, builtin_l2i(numbytes), SEEK_CUR);
	if (ret>0) return builtin_i2l(ret);
	if (ret==0) return builtin_i2l(-1);
	
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return builtin_i2l(0);
}




