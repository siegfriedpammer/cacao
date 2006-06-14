/* src/native/vm/VMClassLoader.c - java/lang/VMClassLoader

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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

   Contact: cacao@cacaojvm.org

   Authors: Roman Obermaiser

   Changes: Joseph Wenninger
            Christian Thalinger
            Edwin Steiner

   $Id: VMClassLoader.c 5031 2006-06-14 18:36:22Z motse $

*/


#include "config.h"

#include <sys/stat.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_ClassLoader.h"
#include "native/include/java_security_ProtectionDomain.h"
#include "native/include/java_util_Vector.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/classcache.h"
#include "vm/exceptions.h"
#include "vm/initialize.h"
#include "vm/linker.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/suck.h"
#include "vm/vm.h"
#include "vm/zip.h"
#include "vm/jit/asmpart.h"

#if defined(ENABLE_JVMTI)
#include "native/jvmti/cacaodbg.h"
#endif


/*
 * Class:     java/lang/VMClassLoader
 * Method:    defineClass
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/String;[BIILjava/security/ProtectionDomain;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_defineClass(JNIEnv *env, jclass clazz, java_lang_ClassLoader *cl, java_lang_String *name, java_bytearray *data, s4 offset, s4 len, java_security_ProtectionDomain *pd)
{
	classinfo   *c;
	classinfo   *r;
	classbuffer *cb;
	utf         *utfname;
#if defined(ENABLE_JVMTI)
	jint new_class_data_len = 0;
	unsigned char* new_class_data = NULL;
#endif

	/* check if data was passed */

	if (data == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* check the indexes passed */

	if ((offset < 0) || (len < 0) || ((offset + len) > data->header.size)) {
		exceptions_throw_arrayindexoutofboundsexception();
		return NULL;
	}

	if (name) {
		/* convert '.' to '/' in java string */

		utfname = javastring_toutf(name, true);
		
		/* check if this class has already been defined */

		c = classcache_lookup_defined_or_initiated((java_objectheader *) cl, utfname);
		if (c) {
			*exceptionptr =
				exceptions_new_linkageerror("duplicate class definition: ",c);
			return NULL;
		}
	} 
	else {
		utfname = NULL;
	}


#if defined(ENABLE_JVMTI)
	/* fire Class File Load Hook JVMTI event */
	if (jvmti) jvmti_ClassFileLoadHook(utfname, len, (unsigned char*)data->data, 
							(java_objectheader *)cl, (java_objectheader *)pd, 
							&new_class_data_len, &new_class_data);
#endif


	/* create a new classinfo struct */

	c = class_create_classinfo(utfname);

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getloadingtime)
		loadingtime_start();
#endif

	/* build a classbuffer with the given data */

	cb = NEW(classbuffer);
	cb->class = c;
#if defined(ENABLE_JVMTI)
	/* check if the JVMTI wants to modify the class */
	if (new_class_data == NULL) {
#endif
	cb->size  = len;
	cb->data  = (u1 *) &data->data[offset];
#if defined(ENABLE_JVMTI)
	} else {
		cb->size  = new_class_data_len;
		cb->data  = (u1 *) new_class_data;
	}
#endif
	cb->pos   = cb->data;

	/* preset the defining classloader */

	c->classloader = (java_objectheader *) cl;

	/* load the class from this buffer */

	r = load_class_from_classbuffer(cb);

	/* free memory */

	FREE(cb, classbuffer);

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getloadingtime)
		loadingtime_stop();
#endif

	if (!r) {
		/* If return value is NULL, we had a problem and the class is not   */
		/* loaded. */
		/* now free the allocated memory, otherwise we could run into a DOS */

		class_free(c);

		return NULL;
	}

	/* set ProtectionDomain */

	c->object.pd = pd;

	/* Store the newly defined class in the class cache. This call also       */
	/* checks whether a class of the same name has already been defined by    */
	/* the same defining loader, and if so, replaces the newly created class  */
	/* by the one defined earlier.                                            */
	/* Important: The classinfo given to classcache_store must be             */
	/*            fully prepared because another thread may return this       */
	/*            pointer after the lookup at to top of this function         */
	/*            directly after the class cache lock has been released.      */

	c = classcache_store((java_objectheader *)cl,c,true);

	return (java_lang_Class *) c;
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    getPrimitiveClass
 * Signature: (C)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_getPrimitiveClass(JNIEnv *env, jclass clazz, s4 type)
{
	classinfo *c;

	/* get primitive class */

	switch (type) {
	case 'I':
		c = primitivetype_table[PRIMITIVETYPE_INT].class_primitive;
		break;
	case 'J':
		c = primitivetype_table[PRIMITIVETYPE_LONG].class_primitive;
		break;
	case 'F':
		c = primitivetype_table[PRIMITIVETYPE_FLOAT].class_primitive;
		break;
	case 'D':
		c = primitivetype_table[PRIMITIVETYPE_DOUBLE].class_primitive;
		break;
	case 'B':
		c = primitivetype_table[PRIMITIVETYPE_BYTE].class_primitive;
		break;
	case 'C':
		c = primitivetype_table[PRIMITIVETYPE_CHAR].class_primitive;
		break;
	case 'S':
		c = primitivetype_table[PRIMITIVETYPE_SHORT].class_primitive;
		break;
	case 'Z':
		c = primitivetype_table[PRIMITIVETYPE_BOOLEAN].class_primitive;
		break;
	case 'V':
		c = primitivetype_table[PRIMITIVETYPE_VOID].class_primitive;
		break;
	default:
		*exceptionptr = new_exception(string_java_lang_ClassNotFoundException);
		c = NULL;
	}

	return (java_lang_Class *) c;
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    resolveClass
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClassLoader_resolveClass(JNIEnv *env, jclass clazz, java_lang_Class *c)
{
	classinfo *ci;

	ci = (classinfo *) c;

	if (!ci) {
		exceptions_throw_nullpointerexception();
		return;
	}

	/* link the class */

	if (!(ci->state & CLASS_LINKED))
		(void) link_class(ci);

	return;
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    loadClass
 * Signature: (Ljava/lang/String;Z)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_loadClass(JNIEnv *env, jclass clazz, java_lang_String *name, jboolean resolve)
{
	classinfo *c;
	utf *u;

	if (name == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* create utf string in which '.' is replaced by '/' */

	u = javastring_toutf(name, true);

	/* load class */

	if (!(c = load_class_bootstrap(u)))
		goto exception;

	/* resolve class -- if requested */

/*  	if (resolve) */
		if (!link_class(c))
			goto exception;

	return (java_lang_Class *) c;

 exception:
	c = (*exceptionptr)->vftbl->class;
	
	/* if the exception is a NoClassDefFoundError, we replace it with a
	   ClassNotFoundException, otherwise return the exception */

	if (c == class_java_lang_NoClassDefFoundError) {
		/* clear exceptionptr, because builtin_new checks for 
		   ExceptionInInitializerError */
		*exceptionptr = NULL;

		*exceptionptr =
			new_exception_javastring(string_java_lang_ClassNotFoundException, name);
	}

	return NULL;
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    nativeGetResources
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT java_util_Vector* JNICALL Java_java_lang_VMClassLoader_nativeGetResources(JNIEnv *env, jclass clazz, java_lang_String *name)
{
	jobject               o;         /* vector being created     */
	methodinfo           *m;         /* "add" method of vector   */
	java_lang_String     *path;      /* path to be added         */
	list_classpath_entry *lce;       /* classpath entry          */
	utf                  *utfname;   /* utf to look for          */
	char                 *buffer;    /* char buffer              */
	char                 *namestart; /* start of name to use     */
	char                 *tmppath;   /* temporary buffer         */
	s4                    namelen;   /* length of name to use    */
	s4                    searchlen; /* length of name to search */
	s4                    bufsize;   /* size of buffer allocated */
	s4                    pathlen;   /* name of path to assemble */
	struct stat           buf;       /* buffer for stat          */
	jboolean              ret;       /* return value of "add"    */

	/* get the resource name as utf string */

	utfname = javastring_toutf(name, false);
	if (!utfname)
		return NULL;

	/* copy it to a char buffer */

	namelen = utf_bytes(utfname);
	searchlen = namelen;
	bufsize = namelen + strlen("0");
	buffer = MNEW(char, bufsize);

	utf_copy(buffer, utfname);
	namestart = buffer;

	/* skip leading '/' */

	if (namestart[0] == '/') {
		namestart++;
		namelen--;
		searchlen--;
	}

	/* remove trailing `.class' */

	if (namelen >= 6 && strcmp(namestart + (namelen - 6), ".class") == 0) {
		searchlen -= 6;
	}

	/* create a new needle to look for, if necessary */

	if (searchlen != bufsize-1) {
		utfname = utf_new(namestart, searchlen);
		if (utfname == NULL)
			goto return_NULL;
	}
			
	/* new Vector() */

	o = native_new_and_init(class_java_util_Vector);

	if (!o)
		goto return_NULL;

	/* get Vector.add() method */

	m = class_resolveclassmethod(class_java_util_Vector,
								 utf_add,
								 utf_new_char("(Ljava/lang/Object;)Z"),
								 NULL,
								 true);

	if (!m)
		goto return_NULL;

	/* iterate over all classpath entries */

	for (lce = list_first(list_classpath_entries); lce != NULL;
		 lce = list_next(list_classpath_entries, lce)) {
		/* clear path pointer */
  		path = NULL;

#if defined(ENABLE_ZLIB)
		if (lce->type == CLASSPATH_ARCHIVE) {

			if (zip_find(lce, utfname)) {
				pathlen = strlen("jar:file://") + lce->pathlen + strlen("!/") +
					namelen + strlen("0");

				tmppath = MNEW(char, pathlen);

				sprintf(tmppath, "jar:file://%s!/%s", lce->path, namestart);
				path = javastring_new_from_utf_string(tmppath),

				MFREE(tmppath, char, pathlen);
			}

		} else {
#endif /* defined(ENABLE_ZLIB) */
			pathlen = strlen("file://") + lce->pathlen + namelen + strlen("0");

			tmppath = MNEW(char, pathlen);

			sprintf(tmppath, "file://%s%s", lce->path, namestart);

			/* Does this file exist? */

			if (stat(tmppath + strlen("file://") - 1, &buf) == 0)
				if (!S_ISDIR(buf.st_mode))
					path = javastring_new_from_utf_string(tmppath);

			MFREE(tmppath, char, pathlen);
#if defined(ENABLE_ZLIB)
		}
#endif

		/* if a resource was found, add it to the vector */

		if (path) {
			ret = vm_call_method_int(m, o, path);

			if (*exceptionptr)
				goto return_NULL;

			if (ret == 0) 
				goto return_NULL;
		}
	}

	MFREE(buffer, char, bufsize);

	return (java_util_Vector *) o;

return_NULL:
	MFREE(buffer, char, bufsize);

	return NULL;
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    findLoadedClass
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_findLoadedClass(JNIEnv *env, jclass clazz, java_lang_ClassLoader *cl, java_lang_String *name)
{
	classinfo *c;
	utf       *u;

	/* replace `.' by `/', this is required by the classcache */

	u = javastring_toutf(name, true);

	/* lookup for defining classloader */

	c = classcache_lookup_defined((classloader *) cl, u);

	/* if not found, lookup for initiating classloader */

	if (c == NULL)
		c = classcache_lookup((classloader *) cl, u);

	return (java_lang_Class *) c;
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
 * vim:noexpandtab:sw=4:ts=4:
 */
