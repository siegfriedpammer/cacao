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

#include "mm/memory.hpp"

#include "native/jni.hpp"
#include "native/llni.h"
#include "native/native.hpp"

#if defined(ENABLE_JNI_HEADERS)
# include "native/include/com_sun_cldc_io_ResourceInputStream.h"
#endif

#include "threads/mutex.hpp"

#include "vm/jit/builtin.hpp"
#include "vm/exceptions.hpp"
#include "vm/javaobjects.hpp"
#include "vm/string.hpp"
#include "vm/types.h"
#include "vm/vm.hpp" /* REMOVE ME: temporarily */
#include "vm/zip.hpp"


static java_handle_t* zip_read_resource(list_classpath_entry *lce, utf *name)
{
	hashtable_zipfile_entry *htzfe;
	lfh                      lfh;
	u1                      *indata;
	u1                      *outdata;
	z_stream                 zs;
	int                      err;
	
	classinfo *ci;

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
		
	// Create a file descriptor object.
	ci = load_class_bootstrap(utf_new_char("com/sun/cldchi/jvm/FileDescriptor"));
	java_handle_t* h = native_new_and_init(ci);

	if (h == NULL)
		return NULL;

	com_sun_cldchi_jvm_FileDescriptor fd(h, (int64_t) outdata, 0, htzfe->uncompressedsize);

	return fd.get_handle();
}


static java_handle_t* file_read_resource(char *path) 
{
	int len;
	struct stat statBuffer;
	u1 *filep;
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
		java_handle_t* h = native_new_and_init(ci);

		if (h == NULL)
			return NULL;

		com_sun_cldchi_jvm_FileDescriptor fd(h, (int64_t) filep, 0, len); 

		return fd.get_handle();	
	}
	else {
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
JNIEXPORT jobject JNICALL Java_com_sun_cldc_io_ResourceInputStream_open(JNIEnv *env, jclass clazz, jstring name)
{
	char *filename;
	s4 filenamelen;
	char *path;
	utf *uname;
	java_handle_t* descriptor;
	
	/* get the classname as char string (do it here for the warning at
       the end of the function) */

	uname = javastring_toutf((java_handle_t *)name, false);
	filenamelen = utf_bytes(uname) + strlen("0");
	filename = MNEW(char, filenamelen);
	utf_copy(filename, uname);
	
	/* walk through all classpath entries */

	for (List<list_classpath_entry*>::iterator it = list_classpath_entries->begin(); it != list_classpath_entries->end(); it++) {
		list_classpath_entry* lce = *it;

#if defined(ENABLE_ZLIB)
		if (lce->type == CLASSPATH_ARCHIVE) {

			/* enter a monitor on zip/jar archives */
			lce->mutex->lock();

			/* try to get the file in current archive */
			descriptor = zip_read_resource(lce, uname);

			/* leave the monitor */
			lce->mutex->unlock();
			
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

	return (jobject) descriptor;
}


/*
 * Class:     com/sun/cldc/io/ResourceInputStream
 * Method:    bytesRemain
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_com_sun_cldc_io_ResourceInputStream_bytesRemain(JNIEnv *env, jclass clazz, jobject jobj)
{
	com_sun_cldchi_jvm_FileDescriptor fd(jobj);
	int32_t length   = fd.get_position();
	int32_t position = fd.get_length();

	return length - position;
}


/*
 * Class:     com/sun/cldc/io/ResourceInputStream
 * Method:    readByte
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_com_sun_cldc_io_ResourceInputStream_readByte(JNIEnv *env, jclass clazz, jobject jobj)
{
	com_sun_cldchi_jvm_FileDescriptor fd(jobj);

	int64_t filep    = fd.get_pointer();
	int32_t position = fd.get_position();
	int32_t length   = fd.get_length();

	uint8_t byte;

	if (position < length) {
		byte = ((uint8_t*) filep)[position];
		position++;
	}
	else {
		return -1; /* EOF */
	}

	// Update access position.
	fd.set_position(position);
	
	return (byte & 0xFF);
}


/*
 * Class:     com/sun/cldc/io/ResourceInputStream
 * Method:    readBytes
 * Signature: (Ljava/lang/Object;[BII)I
 */
JNIEXPORT jint JNICALL Java_com_sun_cldc_io_ResourceInputStream_readBytes(JNIEnv *env, jclass clazz, jobject jobj, jbyteArray byteArray, jint off, jint len)
{
	/* get pointer to the buffer */
	// XXX Not GC safe.
	void* buf = &(LLNI_array_direct((java_handle_bytearray_t*) byteArray, off));
	
	com_sun_cldchi_jvm_FileDescriptor fd(jobj);

	int64_t filep      = fd.get_pointer();
	int32_t position   = fd.get_position();
	int32_t fileLength = fd.get_length();

	int32_t readBytes = -1;

	if (position < fileLength) {
		int32_t available = fileLength - position;

		if (available < len) {
			readBytes = available;
		} else {
			readBytes = len;
		}

		os::memcpy(buf, ((uint8_t*) filep) + position, readBytes * sizeof(uint8_t));
		position += readBytes;
	}
	else {
		return -1; /* EOF */
	}

	// Update access position.
	fd.set_position(position);
	
	return readBytes;
}


/*
 * Class:     com/sun/cldc/io/ResourceInputStream
 * Method:    clone
 * Signature: (Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_com_sun_cldc_io_ResourceInputStream_clone(JNIEnv *env, jclass clazz, jobject jobj)
{
	com_sun_cldchi_jvm_FileDescriptor fd(jobj);

	classinfo* c = load_class_bootstrap(utf_new_char("com/sun/cldchi/jvm/FileDescriptor"));
	java_handle_t* h = native_new_and_init(c);

	if (h == NULL)
		return NULL;

	com_sun_cldchi_jvm_FileDescriptor clonefd(h, fd);
	
	return (jobject) clonefd.get_handle();
}

} // extern "C"


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
 
void _Jv_com_sun_cldc_io_ResourceInputStream_init(void)
{
	utf* u = utf_new_char("com/sun/cldc/io/ResourceInputStream");
 
	NativeMethods& nm = VM::get_current()->get_nativemethods();
	nm.register_methods(u, methods, NATIVE_METHODS_COUNT);
}


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
