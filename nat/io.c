/****************************** nat/io.c ***************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the native functions for class java.io.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Mark Probst         EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/10/22

*******************************************************************************/

#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#ifdef _OSF_SOURCE 
#include <sys/mode.h>
#endif
#include <sys/stat.h>

#include "../threads/threadio.h"                    /* schani */

/*
 * io is performed through the threaded... functions, since io operations may
 * block and the corresponding thread must, in this case, be suspended.
 *
 * schani
 */

/********************* java.io.FileDescriptor ********************************/

s4 java_io_FileDescriptor_valid (struct java_io_FileDescriptor* this)
{
	if (this->fd >= 0) return 1;
	return 0;
}

struct java_io_FileDescriptor* java_io_FileDescriptor_initSystemFD 
        (struct java_io_FileDescriptor* fd, s4 handle)
{
	switch (handle) {
	case 0:  fd -> fd = fileno(stdin); 
	         break;
	case 1:  fd -> fd = fileno(stdout);
	         break;
	case 2:  fd -> fd = fileno(stderr);
	         break;
	default: panic ("Invalid file descriptor number");
	}

	threadedFileDescriptor(fd->fd);

	return fd;
}


/************************* java.io.FileInputStream ***************************/


void java_io_FileInputStream_open 
       (struct java_io_FileInputStream* this, struct java_lang_String* name)
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
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
}

s4 java_io_FileInputStream_read (struct java_io_FileInputStream* this)
{
	s4 r;
	u1 buffer[1];
	r = threadedRead (this->fd->fd, (char *) buffer, 1);	

	if (r>0) return buffer[0];
	if (r==0) return -1;
	
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return 0;
}


s4 java_io_FileInputStream_readBytes (struct java_io_FileInputStream* this, 
     java_bytearray* buffer, s4 start, s4 len)
{
	s4 ret = threadedRead (this->fd->fd, (char *) buffer->data+start, len);
	if (ret>0) return ret;
	if (ret==0) return -1;
	
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return 0;
}


s8 java_io_FileInputStream_skip 
       (struct java_io_FileInputStream* this, s8 numbytes)
{
	s4 ret = lseek (this->fd->fd, builtin_l2i(numbytes), SEEK_CUR);
	if (ret>0) return builtin_i2l(ret);
	if (ret==0) return builtin_i2l(-1);
	
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return builtin_i2l(0);
}

s4 java_io_FileInputStream_available (struct java_io_FileInputStream* this)
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

void java_io_FileInputStream_close (struct java_io_FileInputStream* this)
{
	if (this->fd->fd >= 0) {
		s4 r = close (this->fd->fd);
		this->fd->fd = -1;
		if (r < 0) 
			exceptionptr = native_new_and_init (class_java_io_IOException);
	}
}





/*************************** java.io.FileOutputStream ************************/


void java_io_FileOutputStream_open (struct java_io_FileOutputStream* this, 
                                  struct java_lang_String* name)
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

                                
void java_io_FileOutputStream_write
          (struct java_io_FileOutputStream* this, s4 byte)
{
	u1 buffer[1];
	s4 l;

	buffer[0] = byte;
	l = threadedWrite (this->fd->fd, (char *) buffer, 1);

	if (l<1) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		}
}
	
void java_io_FileOutputStream_writeBytes (struct java_io_FileOutputStream* this,
                            java_bytearray* buffer, s4 start, s4 len)
{
	s4 o;

	if (len == 0)
		return;
	o = threadedWrite (this->fd->fd, (char *) buffer->data+start, len);
	if (o!=len) 
		exceptionptr = native_new_and_init (class_java_io_IOException);
}
                            
void java_io_FileOutputStream_close (struct java_io_FileOutputStream* this)
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



/************************** java.io.File **************************************/

s4 java_io_File_exists0 (struct java_io_File* this)
{
	struct stat buffer;
	char *path;
	int err;
	
	path = javastring_tochar( (java_objectheader*) (this->path));
	
	err = stat (path, &buffer);
	if (err==0) return 1;
	return 0;
}	

s4 java_io_File_canWrite0 (struct java_io_File* this) 
{
	int err;
	err = access (javastring_tochar( (java_objectheader*) (this->path)), W_OK);
	if (err==0) return 1;
	return 0;
}

s4 java_io_File_canRead0 (struct java_io_File* this)
{
	int err;
	err = access (javastring_tochar( (java_objectheader*) (this->path)), R_OK);
	if (err==0) return 1;
	return 0;
}

s4 java_io_File_isFile0 (struct java_io_File* this)
{
	struct stat buffer;
	char *path;
	int err;
	
	path = javastring_tochar( (java_objectheader*) (this->path));
	
	err = stat (path, &buffer);
	if (err!=0) return 0;
	if (S_ISREG(buffer.st_mode)) return 1;
	return 0;
}

s4 java_io_File_isDirectory0 (struct java_io_File* this)
{
	struct stat buffer;
	char *path;
	int err;
	
	path = javastring_tochar( (java_objectheader*) (this->path));
	
	err = stat (path, &buffer);
	if (err!=0) return 0;
	if (S_ISDIR(buffer.st_mode)) return 1;
	return 0;
}
 
s8 java_io_File_lastModified0 (struct java_io_File* this) 
{
	struct stat buffer;
	int err;
	err = stat (javastring_tochar( (java_objectheader*) (this->path)),  &buffer);
	if (err!=0) return builtin_i2l(0);
	return builtin_lmul (builtin_i2l(buffer.st_mtime), builtin_i2l(1000) );
}

s8 java_io_File_length0 (struct java_io_File* this)
{
	struct stat buffer;
	int err;
	err = stat (javastring_tochar( (java_objectheader*) (this->path)),  &buffer);
	if (err!=0) return builtin_i2l(0);
	return builtin_i2l(buffer.st_size);
}

s4 java_io_File_mkdir0 (struct java_io_File* this) 
{ 
	char *name = javastring_tochar ( (java_objectheader*) (this->path) );
	int err = mkdir (name, 0777);
	if (err==0) return 1;
	return 0;
}

s4 java_io_File_renameTo0 (struct java_io_File* this, struct java_io_File* new) 
{ 
#define MAXPATHLEN 200
	char newname[MAXPATHLEN];
	char *n = javastring_tochar ( (java_objectheader*) (new->path) );
	int err;
	
	if (strlen(n)>=MAXPATHLEN) return 0;
	strcpy (newname, n);
	n = javastring_tochar ( (java_objectheader*) (this->path) );
	err = rename (n, newname);
	if (err==0) return 1;
	return 0;
}

s4 java_io_File_delete0 (struct java_io_File* this) 
{ 
	int err;
	err = remove (javastring_tochar ( (java_objectheader*) (this->path) ) );
	if (err==0) return 1;
	return 0; 
}

java_objectarray* java_io_File_list0 (struct java_io_File* this) 
{ 
	char *name;
	DIR *d;
	int i,len, namlen;
	java_objectarray *a;
	struct dirent *ent;
	struct java_lang_String *n;
	char entbuffer[257];
	
	name = javastring_tochar ( (java_objectheader*) (this->path) );
	d = opendir(name);
	if (!d) return NULL;
	
	len=0;
	while (readdir(d) != NULL) len++;
	rewinddir (d);
	
	a = builtin_anewarray (len, class_java_lang_String);
	if (!a) {
		closedir(d);
		return NULL;
		}
		
	for (i=0; i<len; i++) {
		if ( (ent = readdir(d)) != NULL) {
			namlen = strlen(ent->d_name);
			memcpy (entbuffer, ent->d_name, namlen);
			entbuffer[namlen] = '\0';
			
			n = (struct java_lang_String*) 
				javastring_new_char (entbuffer);
			
			a -> data[i] = (java_objectheader*) n;
			}
		}


	closedir(d);
	return a;
}

s4 java_io_File_isAbsolute (struct java_io_File* this) 
{ 
	char *name = javastring_tochar ( (java_objectheader*) (this->path) );
	if (name[0] == '/') return 1;
	return 0;
}




/********************** java.io.RandomAccessFile *****************************/

void java_io_RandomAccessFile_open (struct java_io_RandomAccessFile* this, 
               struct java_lang_String* name, s4 writeable)
{
	s4 fd;
	char *fname = javastring_tochar ((java_objectheader*)name);
	
	if (writeable) fd = open (fname, O_RDWR, 0);
	else           fd = open (fname, O_RDONLY, 0);
	if (fd==-1) goto fail;

	threadedFileDescriptor(fd);
	
	this->fd->fd = fd;
	return;

	fail:
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
}

s4 java_io_RandomAccessFile_read (struct java_io_RandomAccessFile* this)
{ 
	s4 r;
	u1 buffer[1];
	r = threadedRead (this->fd->fd, (char *) buffer, 1);	
	if (r>0) return buffer[1];
	if (r==0) return -1;
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return 0;
}

s4 java_io_RandomAccessFile_readBytes (struct java_io_RandomAccessFile* this, 
     java_bytearray* buffer, s4 start, s4 len)
{
	s4 r = threadedRead (this->fd->fd, (char *) buffer->data+start, len);
	if (r>0) return r;
	if (r==0) return -1;
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return 0;
}

void java_io_RandomAccessFile_write 
       (struct java_io_RandomAccessFile* this, s4 byte)
{ 
	u1 buffer[1];
	int r;
	buffer[1] = byte;
	r = write (this->fd->fd, buffer, 1);
	if (r<0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		}
}

void java_io_RandomAccessFile_writeBytes (struct java_io_RandomAccessFile* this, 
                            java_bytearray* buffer, s4 start, s4 len)
{
	s4 o;
	if (len == 0)
		return;
	o = threadedWrite (this->fd->fd, (char *) buffer->data+start, len);
	if (o!=len) exceptionptr = native_new_and_init (class_java_io_IOException);
}
               
s8 java_io_RandomAccessFile_getFilePointer 
        (struct java_io_RandomAccessFile* this)
{
	s4 p = lseek (this->fd->fd, 0, SEEK_CUR);
	if (p>=0) return builtin_i2l(p);
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return builtin_i2l(0);
}

void java_io_RandomAccessFile_seek 
           (struct java_io_RandomAccessFile* this, s8 offset)
{
	s4 p = lseek (this->fd->fd, builtin_l2i(offset), SEEK_SET); 
	if (p<0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		}
}

s8 java_io_RandomAccessFile_length (struct java_io_RandomAccessFile* this)
{
	struct stat buffer;
	s4 r = fstat(this->fd->fd, &buffer);
	if (r>=0) return builtin_i2l(buffer.st_size);
	exceptionptr = native_new_and_init (class_java_io_IOException);
	return builtin_i2l(0);
}

void java_io_RandomAccessFile_close (struct java_io_RandomAccessFile* this)
{	
  if (this->fd->fd >= 0) {
    s4 r = close (this->fd->fd);
		this->fd->fd = -1;
		if (r<0) 
			exceptionptr = native_new_and_init (class_java_io_IOException);
		}
}




/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */

