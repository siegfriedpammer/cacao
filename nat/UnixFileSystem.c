/* Structure information for class: java/io/UnixFileSystem */

/*
 * Class:     java/io/UnixFileSystem
 * Method:    canonicalize
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_io_UnixFileSystem_canonicalize (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_lang_String* par1) 
{
    return par1;
}

JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_isAbsolute (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* file)
{
  java_lang_String* s = file->path;
  java_chararray *a = s->value;

  /* absolute filenames start with '/' */
  if (a->data[s->offset]=='/') return 1;
  return 0;
}


/*
 * Class:     java/io/UnixFileSystem
 * Method:    getBooleanAttributes0
 * Signature: (Ljava/io/File;)I
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_getBooleanAttributes0 (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* file)
{
	struct stat buffer;
	char *path;
	int err;
	s4 attrib = 0;
	
	path = javastring_tochar( (java_objectheader*) (file->path));
	err  = stat (path, &buffer);

	if (err==0) {

	    attrib = 0x01;  /* file exists */

	    if (S_ISREG(buffer.st_mode)) attrib |= 0x02;
	    if (S_ISDIR(buffer.st_mode)) attrib |= 0x04;
	}

	
	/* printf("getBooleanAttributes called for file: %s (%d)\n",path,attrib); */

	return attrib;
}

/*
 * Class:     java/io/UnixFileSystem
 * Method:    checkAccess
 * Signature: (Ljava/io/File;Z)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_checkAccess (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* file, s4 write)
{
	struct stat buffer;
	char *path;
	int err;
	
	/* get file-attributes */
	path = javastring_tochar( (java_objectheader*) (file->path));
	err  = stat (path, &buffer);

	if (err==0) {

	    /* check access rights */
	    if (((write)  && (buffer.st_mode & S_IWUSR)) ||
	        ((!write) && (buffer.st_mode & S_IRUSR))    )
	      return 1;		
	}

	/* no access rights */
	return 0;
}

/*
 * Class:     java/io/UnixFileSystem
 * Method:    getLastModifiedTime
 * Signature: (Ljava/io/File;)J
 */
JNIEXPORT s8 JNICALL Java_java_io_UnixFileSystem_getLastModifiedTime (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* file)
{
	struct stat buffer;
	int err;
	
	/* check argument */
	if (!file) {
	    log_text("Warning: invalid call of native function getLastModifiedTime");
	    return 0;
	}

	/* get file-attributes */
	err = stat (javastring_tochar( (java_objectheader*) (file->path)),  &buffer);
	if (err!=0) return builtin_i2l(0);
	return builtin_lmul (builtin_i2l(buffer.st_mtime), builtin_i2l(1000) );
}

/*
 * Class:     java/io/UnixFileSystem
 * Method:    getLength
 * Signature: (Ljava/io/File;)J
 */
JNIEXPORT s8 JNICALL Java_java_io_UnixFileSystem_getLength (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* file)
{
	struct stat buffer;
	int err;
	
	if (!file) {
	    log_text("Warning: invalid call of native function getLength");
	    return 0;
	}

	/* get file-attributes */
	err = stat (javastring_tochar( (java_objectheader*) (file->path)),  &buffer);
	if (err!=0) return builtin_i2l(0);
	return builtin_i2l(buffer.st_size);
}

/*
 * Class:     java/io/UnixFileSystem
 * Method:    createFileExclusively
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_createFileExclusively (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_lang_String* name)
{
	s4 fd;
	char *fname = javastring_tochar ((java_objectheader*)name);

	if (!fname) return 0;
	
	/* create new file */
	fd = creat (fname, 0666);
	if (fd<0) return 0;
	
	threadedFileDescriptor(fd);
	close (fd);

	return 1;
}

/*
 * Class:     java/io/UnixFileSystem
 * Method:    delete
 * Signature: (Ljava/io/File;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_delete (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* file)
{
	int err;
	/* delete the file */
	err = remove (javastring_tochar ( (java_objectheader*) (file->path) ) );
	if (err==0) return 1;
	
	/* not successful */
	return 0; 
}

/*
 * Class:     java/io/UnixFileSystem
 * Method:    deleteOnExit
 * Signature: (Ljava/io/File;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_deleteOnExit (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* file)
{
    log_text("Java_java_io_UnixFileSystem_deleteOnExit called");

    return 1;
}

/*
 * Class:     java/io/UnixFileSystem
 * Method:    list
 * Signature: (Ljava/io/File;)[Ljava/lang/String;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_io_UnixFileSystem_list (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* file) 
{
	char *name;
	DIR *d;
	int i,len, namlen;
	java_objectarray *a;
	struct dirent *ent;
	struct java_lang_String *n;
	char entbuffer[257];
	
	name = javastring_tochar ( (java_objectheader*) (file->path) );
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

/*
 * Class:     java/io/UnixFileSystem
 * Method:    createDirectory
 * Signature: (Ljava/io/File;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_createDirectory (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* file)
{
	char *name;
	int err;

	if (file) {

	    name = javastring_tochar ( (java_objectheader*) (file->path) );
	    err = mkdir (name, 0777);

	    if (err==0) return 1;
	}

	return 0;
}

/*
 * Class:     java/io/UnixFileSystem
 * Method:    rename
 * Signature: (Ljava/io/File;Ljava/io/File;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_rename (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* file, struct java_io_File* new)
{

#define MAXJAVAPATHLEN 200

	char newname[MAXJAVAPATHLEN];
	char *n; 
	int err;

	if (file && new) {
	    n = javastring_tochar ( (java_objectheader*) (new->path) );
	    if (strlen(n)>=MAXJAVAPATHLEN) return 0;
	    strcpy (newname, n);
	    n = javastring_tochar ( (java_objectheader*) (file->path) );
	    err = rename (n, newname);
	    if (err==0) return 1;
	}

	return 0;
}

/*
 * Class:     java/io/UnixFileSystem
 * Method:    setLastModifiedTime
 * Signature: (Ljava/io/File;J)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_setLastModifiedTime (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* file, s8 time)
{
        log_text("Java_java_io_FileSystemImpl_setLastModifiedTime called");

	if (!utime(javastring_tochar( (java_objectheader*) (file->path)), time / 1000))
	    return 1;
	else
	    return 0;
}

/*
 * Class:     java/io/UnixFileSystem
 * Method:    setReadOnly
 * Signature: (Ljava/io/File;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_UnixFileSystem_setReadOnly (JNIEnv *env ,  struct java_io_UnixFileSystem* this , struct java_io_File* file)
{
	struct stat buffer;
	char *path;
	int err;
	s4 attrib = 0;
	
        log_text("Java_java_io_FileSystemImpl_setReadOnly called");

	path = javastring_tochar( (java_objectheader*) (file->path));
	/* get file-attributes */
	err  = stat (path, &buffer);

	if (!err) {

	    buffer.st_mode &=  ~( 0x00002 |      /* write by others */
				  0x00200 );     /* write by owner  */

	    /* set file-attributes */
	    chmod(path,buffer.st_mode);

	    if (!err) return 1;
	}

	return 0;
}

/*
 * Class:     java/io/UnixFileSystem
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_io_UnixFileSystem_initIDs (JNIEnv *env )
{
    /* empty */
}


