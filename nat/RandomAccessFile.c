/* class: java/io/RandomAccessFile */

/*
 * Class:     java/io/RandomAccessFile
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_close ( JNIEnv *env ,  struct java_io_RandomAccessFile* this)
{
  if (this->fd->fd >= 0) {
    s4 r = close (this->fd->fd);
		this->fd->fd = -1;
		if (r<0) 
			exceptionptr = native_new_and_init (class_java_io_IOException);
		}
}

/*
 * Class:     java/io/RandomAccessFile
 * Method:    getFilePointer
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_io_RandomAccessFile_getFilePointer ( JNIEnv *env ,  struct java_io_RandomAccessFile* this)
{
	s4 p = lseek (this->fd->fd, 0, SEEK_CUR);
	if (p>=0) return builtin_i2l(p);
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return builtin_i2l(0);
}

/*
 * Class:     java/io/RandomAccessFile
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_initIDs ( JNIEnv *env  )
{
    /* no IDs required */
}

/*
 * Class:     java/io/RandomAccessFile
 * Method:    length
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_io_RandomAccessFile_length ( JNIEnv *env ,  struct java_io_RandomAccessFile* this)
{
	struct stat buffer;
	s4 r = fstat(this->fd->fd, &buffer);
	if (r>=0) return builtin_i2l(buffer.st_size);
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return builtin_i2l(0);
}

/*
 * Class:     java/io/RandomAccessFile
 * Method:    open
 * Signature: (Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_open ( JNIEnv *env ,  struct java_io_RandomAccessFile* this, struct java_lang_String* name, s4 writeable)
{
	s4 fd;
	char *fname = javastring_tochar ((java_objectheader*)name);
	
	if (writeable) fd = open (fname, O_RDWR|O_CREAT|O_TRUNC, 0666);
	else           fd = open (fname, O_RDONLY, 0);
	if (fd==-1) goto fail;

	threadedFileDescriptor(fd);
	
	this->fd->fd = fd;
	return;

	fail:
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
}

/*
 * Class:     java/io/RandomAccessFile
 * Method:    read
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_io_RandomAccessFile_read ( JNIEnv *env ,  struct java_io_RandomAccessFile* this)
{
	s4 r;
	u1 buffer[1];
	r = threadedRead (this->fd->fd, (char *) buffer, 1);	
	if (r>0) return buffer[1];
	if (r==0) return -1;
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return 0;
}

/*
 * Class:     java/io/RandomAccessFile
 * Method:    readBytes
 * Signature: ([BII)I
 */
JNIEXPORT s4 JNICALL Java_java_io_RandomAccessFile_readBytes ( JNIEnv *env ,  struct java_io_RandomAccessFile* this, java_bytearray* buffer, s4 start, s4 len)
{
	s4 r = threadedRead (this->fd->fd, (char *) buffer->data+start, len);
	if (r>0) return r;
	if (r==0) return -1;
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return 0;
}

/*
 * Class:     java/io/RandomAccessFile
 * Method:    seek
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_seek ( JNIEnv *env ,  struct java_io_RandomAccessFile* this, s8 offset)
{
	s4 p = lseek (this->fd->fd, builtin_l2i(offset), SEEK_SET); 
	if (p<0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		}
}

/*
 * Class:     java/io/RandomAccessFile
 * Method:    setLength
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_setLength ( JNIEnv *env ,  struct java_io_RandomAccessFile* this, s8 length)
{
  if (ftruncate(this->fd->fd, length)<0)
		exceptionptr = native_new_and_init (class_java_io_IOException);
}

/*
 * Class:     java/io/RandomAccessFile
 * Method:    write
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_write ( JNIEnv *env ,  struct java_io_RandomAccessFile* this, s4 byte)
{
	u1 buffer[1];
	int r;
	buffer[1] = byte;
	r = write (this->fd->fd, buffer, 1);
	if (r<0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		}
}

/*
 * Class:     java/io/RandomAccessFile
 * Method:    writeBytes
 * Signature: ([BII)V
 */
JNIEXPORT void JNICALL Java_java_io_RandomAccessFile_writeBytes ( JNIEnv *env ,  struct java_io_RandomAccessFile* this, java_bytearray* buffer, s4 start, s4 len)
{
	s4 o;
	if (len == 0)
		return;
	o = threadedWrite (this->fd->fd, (char *) buffer->data+start, len);
	if (o!=len) exceptionptr = native_new_and_init (class_java_io_IOException);
}









