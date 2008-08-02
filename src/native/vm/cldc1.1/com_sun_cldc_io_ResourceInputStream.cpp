/* src/native/vm/cldc1.1/com_sun_cldc_io_ResourceInputStream.cpp

   Copyright (C) 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#include "config.h"

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <zlib.h>

#include "mm/memory.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_String.h"
#include "native/include/com_sun_cldchi_jvm_FileDescriptor.h"

// FIXME
extern "C" {
#include "native/include/com_sun_cldc_io_ResourceInputStream.h"
}

#include "vm/types.h"
#include "vm/builtin.h"
#include "vm/vm.hpp" /* REMOVE ME: temporarily */
#include "vm/exceptions.hpp"
#include "vm/string.hpp"

#include "vmcore/zip.h"

#include "threads/lock-common.h"


/* native methods implemented by this file ************************************/
 
static JNINativeMethod methods[] = {
	{ (char*) "open",        (char*) "(Ljava/lang/String;)Ljava/lang/Object;", (void*) (uintptr_t) &Java_com_sun_cldc_io_ResourceInputStream_open        },
	{ (char*) "bytesRemain", (char*) "(Ljava/lang/Object;)I",                  (void*) (uintptr_t) &Java_com_sun_cldc_io_ResourceInputStream_bytesRemain },
	{ (char*) "readByte",    (char*) "(Ljava/lang/Object;)I",                  (void*) (uintptr_t) &Java_com_sun_cldc_io_ResourceInputStream_readByte    },
	{ (char*) "readBytes",   (char*) "(Ljava/lang/Object;[BII)I",              (void*) (uintptr_t) &Java_com_sun_cldc_io_ResourceInputStream_readBytes   },
	{ (char*) "clone",       (char*) "(Ljava/lang/Object;)Ljava/lang/Object;", (void*) (uintptr_t) &Java_com_sun_cldc_io_ResourceInputStream_clone       },
};
 

/* _Jv_com_sun_cldc_io_ResourceInputStream_init ********************************
 
   Register native functions.
 
*******************************************************************************/
 
// FIXME
extern "C" {
void _Jv_com_sun_cldc_io_ResourceInputStream_init(void)
{
	utf *u;
 
	u = utf_new_char("com/sun/cldc/io/ResourceInputStream");
 
	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}
}

static struct com_sun_cldchi_jvm_FileDescriptor* zip_read_resource(list_classpath_entry *lce, utf *name)
{
	hashtable_zipfile_entry *htzfe;
	lfh                      lfh;
	u1                      *indata;
	u1                      *outdata;
	z_stream                 zs;
	int                      err;
	
	classinfo *ci;
	com_sun_cldchi_jvm_FileDescriptor *fileDescriptor = NULL;

	/* try to find the class in the current archive */

	htzfe = zip_find(lce, name);

	if (htzfe == NULL)
		return NULL;

	/* read stuff from local file header */

	lfh.filenamelength   = SUCK_LE_U2(htzfe->data + LFH_FILE_NAME_LENGTH);
	lfh.extrafieldlength = SUCK_LE_U2(htzfe->data + LFH_EXTRA_FIELD_LENGTH);

	indata = htzfe->data +
		LFH_HEADER_SIZE +
		lfh.filenamelength +
		lfh.extrafieldlength;

	/* allocate buffer for uncompressed data */

	outdata = MNEW(u1, htzfe->uncompressedsize);

	/* how is the file stored? */

	switch (htzfe->compressionmethod) {
	case Z_DEFLATED:
		/* fill z_stream structure */

		zs.next_in   = indata;
		zs.avail_in  = htzfe->compressedsize;
		zs.next_out  = outdata;
		zs.avail_out = htzfe->uncompressedsize;

		zs.zalloc = Z_NULL;
		zs.zfree  = Z_NULL;
		zs.opaque = Z_NULL;

		/* initialize this inflate run */

		if (inflateInit2(&zs, -MAX_WBITS) != Z_OK)
			vm_abort("zip_get: inflateInit2 failed: %s", strerror(errno));

		/* decompress the file into buffer */

		err = inflate(&zs, Z_SYNC_FLUSH);

		if ((err != Z_STREAM_END) && (err != Z_OK))
			vm_abort("zip_get: inflate failed: %s", strerror(errno));

		/* finish this inflate run */

		if (inflateEnd(&zs) != Z_OK)
			vm_abort("zip_get: inflateEnd failed: %s", strerror(errno));
		break;

	case 0:
		/* uncompressed file, just copy the data */
		MCOPY(outdata, indata, u1, htzfe->compressedsize);
		break;

	default:
		vm_abort("zip_get: unknown compression method %d",
				 htzfe->compressionmethod);
	}
		
	/* Create a file descriptor object */
	ci = load_class_bootstrap(utf_new_char("com/sun/cldchi/jvm/FileDescriptor"));
	fileDescriptor = (com_sun_cldchi_jvm_FileDescriptor *) native_new_and_init(ci);
	LLNI_field_set_val(fileDescriptor, pointer, (int)outdata);
	LLNI_field_set_val(fileDescriptor, length, htzfe->uncompressedsize);
	LLNI_field_set_val(fileDescriptor, position, 0);
	return fileDescriptor;
	
}

static struct com_sun_cldchi_jvm_FileDescriptor* file_read_resource(char *path) 
{
	int len;
	struct stat statBuffer;
	u1 *filep;
	com_sun_cldchi_jvm_FileDescriptor *fileDescriptor = NULL; 
	classinfo *ci;
	int fd;
	
	fd = open(path, O_RDONLY);
	
	if (fd > 0) {
		
		if (fstat(fd, &statBuffer) != -1) {
			len = statBuffer.st_size;
		} else {  
			return NULL;
		}
		
		/* Map file into the memory */
		filep = (u1*) mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
		
		/* Create a file descriptor object */
		ci = load_class_bootstrap(utf_new_char("com/sun/cldchi/jvm/FileDescriptor"));
		fileDescriptor = (com_sun_cldchi_jvm_FileDescriptor *) native_new_and_init(ci);
		LLNI_field_set_val(fileDescriptor, pointer, (int)filep);
		LLNI_field_set_val(fileDescriptor, length, len);
		LLNI_field_set_val(fileDescriptor, position, 0);
		
		return fileDescriptor;	
		
	} else {
		return NULL;
	}
	
}


// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     com/sun/cldc/io/ResourceInputStream
 * Method:    open
 * Signature: (Ljava/lang/String;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_com_sun_cldc_io_ResourceInputStream_open(JNIEnv *env, jclass clazz, java_lang_String *name)
{
	
	list_classpath_entry *lce;
	char *filename;
	s4 filenamelen;
	char *path;
	utf *uname;
	com_sun_cldchi_jvm_FileDescriptor* descriptor;
	
	/* get the classname as char string (do it here for the warning at
       the end of the function) */

	uname = javastring_toutf((java_handle_t *)name, false);
	filenamelen = utf_bytes(uname) + strlen("0");
	filename = MNEW(char, filenamelen);
	utf_copy(filename, uname);
	
	/* walk through all classpath entries */

	for (lce = (list_classpath_entry*) list_first(list_classpath_entries); lce != NULL;
		 lce = (list_classpath_entry*) list_next(list_classpath_entries, lce)) {
		 	
#if defined(ENABLE_ZLIB)
		if (lce->type == CLASSPATH_ARCHIVE) {

			/* enter a monitor on zip/jar archives */
			LOCK_MONITOR_ENTER(lce);

			/* try to get the file in current archive */
			descriptor = zip_read_resource(lce, uname);

			/* leave the monitor */
			LOCK_MONITOR_EXIT(lce);
			
			if (descriptor != NULL) { /* file exists */
				break;
			}

		} else {
#endif
			
			path = MNEW(char, lce->pathlen + filenamelen);
			strcpy(path, lce->path);
			strcat(path, filename);

			descriptor = file_read_resource(path);
			
			MFREE(path, char, lce->pathlen + filenamelen);

			if (descriptor != NULL) { /* file exists */
				break;
			}
			
#if defined(ENABLE_ZLIB)
		}
#endif	
			
	}

	MFREE(filename, char, filenamelen);

	return (java_lang_Object*) descriptor;
	
}


/*
 * Class:     com_sun_cldc_io_ResourceInputStream
 * Method:    bytesRemain
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_com_sun_cldc_io_ResourceInputStream_bytesRemain(JNIEnv *env, jclass clazz, struct java_lang_Object* jobj) {
	
	com_sun_cldchi_jvm_FileDescriptor *fileDescriptor;
	int32_t length;
	int32_t position;

	fileDescriptor = (com_sun_cldchi_jvm_FileDescriptor *) jobj;
	LLNI_field_get_val(fileDescriptor, position, position);
	LLNI_field_get_val(fileDescriptor, length, length);
	
	return length - position;

}

/*
 * Class:     com_sun_cldc_io_ResourceInputStream
 * Method:    readByte
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_com_sun_cldc_io_ResourceInputStream_readByte(JNIEnv *env, jclass clazz, struct java_lang_Object* jobj) {
	
	com_sun_cldchi_jvm_FileDescriptor *fileDescriptor;
	u1 byte;
	int32_t length;
	int32_t position;
	int64_t filep;
	
	fileDescriptor = (com_sun_cldchi_jvm_FileDescriptor *) jobj;
	LLNI_field_get_val(fileDescriptor, position, position);
	LLNI_field_get_val(fileDescriptor, length, length);
	LLNI_field_get_val(fileDescriptor, pointer, filep);
	
	if (position < length) {
		byte = ((u1*)(int)filep)[position];
		position++;
	} else {
		return -1; /* EOF */
	}

	/* Update access position */
	LLNI_field_set_val(fileDescriptor, position, position);
	
	return (byte & 0xFF);

}

/*
 * Class:     com_sun_cldc_io_ResourceInputStream
 * Method:    readBytes
 * Signature: (Ljava/lang/Object;[BII)I
 */
JNIEXPORT s4 JNICALL Java_com_sun_cldc_io_ResourceInputStream_readBytes(JNIEnv *env, jclass clazz, struct java_lang_Object* jobj, java_handle_bytearray_t* byteArray, s4 off, s4 len) {
	
	com_sun_cldchi_jvm_FileDescriptor *fileDescriptor;
	s4 readBytes = -1;
	int32_t fileLength;
	int32_t position;
	s4 available;
	int64_t filep;
	void *buf;

	/* get pointer to the buffer */
	buf = &(LLNI_array_direct(byteArray, off));
	
	fileDescriptor = (com_sun_cldchi_jvm_FileDescriptor *) jobj;
	LLNI_field_get_val(fileDescriptor, position, position);
	LLNI_field_get_val(fileDescriptor, length, fileLength);
	LLNI_field_get_val(fileDescriptor, pointer, filep);
	
	if (position < fileLength) {
		available = fileLength - position;
		if (available < len) {
			readBytes = available;
		} else {
			readBytes = len;
		}
		memcpy(buf, ((u1*)(int)filep) + position, readBytes * sizeof(u1));
		position += readBytes;
	} else {
		return -1; /* EOF */
	}

	/* Update access position */
	LLNI_field_set_val(fileDescriptor, position, position);
	
	return readBytes;
}

/*
 * Class:     com_sun_cldc_io_ResourceInputStream
 * Method:    clone
 * Signature: (Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_com_sun_cldc_io_ResourceInputStream_clone(JNIEnv *env, jclass clazz, struct java_lang_Object* jobj) {
	
	classinfo *ci;
	com_sun_cldchi_jvm_FileDescriptor *srcFileDescriptor;
	com_sun_cldchi_jvm_FileDescriptor *dstFileDescriptor;
	int32_t srcLength;
	int32_t srcPosition;
	int64_t srcFilePointer;
	
	srcFileDescriptor = (com_sun_cldchi_jvm_FileDescriptor *) jobj;
	LLNI_field_get_val(srcFileDescriptor, position, srcPosition);
	LLNI_field_get_val(srcFileDescriptor, length, srcLength);
	LLNI_field_get_val(srcFileDescriptor, pointer, srcFilePointer);
	
	ci = load_class_bootstrap(utf_new_char("com/sun/cldchi/jvm/FileDescriptor"));
	dstFileDescriptor = (com_sun_cldchi_jvm_FileDescriptor *) native_new_and_init(ci);
	LLNI_field_set_val(dstFileDescriptor, position, srcPosition);
	LLNI_field_set_val(dstFileDescriptor, length, srcLength);
	LLNI_field_set_val(dstFileDescriptor, pointer, srcFilePointer);
	
	return (java_lang_Object*) dstFileDescriptor;

}

} // extern "C"


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
