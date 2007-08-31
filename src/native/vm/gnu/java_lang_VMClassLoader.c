/* src/native/vm/gnu/VMClassLoader.c

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

*/


#include "config.h"

#include <assert.h>
#include <sys/stat.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_security_ProtectionDomain.h"  /* required by... */
#include "native/include/java_lang_ClassLoader.h"
#include "native/include/java_util_Vector.h"

#include "native/include/java_lang_VMClassLoader.h"

#include "native/vm/java_lang_ClassLoader.h"

#include "toolbox/logging.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/initialize.h"
#include "vm/primitive.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vm/jit/asmpart.h"

#include "vmcore/class.h"
#include "vmcore/classcache.h"
#include "vmcore/linker.h"
#include "vmcore/loader.h"
#include "vmcore/options.h"
#include "vmcore/statistics.h"
#include "vmcore/suck.h"
#include "vmcore/zip.h"

#if defined(ENABLE_JVMTI)
#include "native/jvmti/cacaodbg.h"
#endif


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "defineClass",            "(Ljava/lang/ClassLoader;Ljava/lang/String;[BIILjava/security/ProtectionDomain;)Ljava/lang/Class;", (void *) (ptrint) &Java_java_lang_VMClassLoader_defineClass            },
	{ "getPrimitiveClass",      "(C)Ljava/lang/Class;",                                                                             (void *) (ptrint) &Java_java_lang_VMClassLoader_getPrimitiveClass      },
	{ "resolveClass",           "(Ljava/lang/Class;)V",                                                                             (void *) (ptrint) &Java_java_lang_VMClassLoader_resolveClass           },
	{ "loadClass",              "(Ljava/lang/String;Z)Ljava/lang/Class;",                                                           (void *) (ptrint) &Java_java_lang_VMClassLoader_loadClass              },
	{ "nativeGetResources",     "(Ljava/lang/String;)Ljava/util/Vector;",                                                           (void *) (ptrint) &Java_java_lang_VMClassLoader_nativeGetResources     },
	{ "defaultAssertionStatus", "()Z",                                                                                              (void *) (ptrint) &Java_java_lang_VMClassLoader_defaultAssertionStatus },
	{ "findLoadedClass",        "(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;",                                     (void *) (ptrint) &Java_java_lang_VMClassLoader_findLoadedClass        },
};


/* _Jv_java_lang_VMClassLoader_init ********************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_VMClassLoader_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/VMClassLoader");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    defineClass
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/String;[BIILjava/security/ProtectionDomain;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_defineClass(JNIEnv *env, jclass clazz, java_lang_ClassLoader *cl, java_lang_String *name, java_handle_bytearray_t *data, s4 offset, s4 len, java_security_ProtectionDomain *pd)
{
	return _Jv_java_lang_ClassLoader_defineClass(cl, name, data, offset, len, pd);
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    getPrimitiveClass
 * Signature: (C)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_getPrimitiveClass(JNIEnv *env, jclass clazz, s4 type)
{
	classinfo *c;

	c = primitive_class_get_by_char(type);

	if (c == NULL) {
		exceptions_throw_classnotfoundexception(utf_null);
		return NULL;
	}

	return LLNI_classinfo_wrap(c);
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    resolveClass
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClassLoader_resolveClass(JNIEnv *env, jclass clazz, java_lang_Class *c)
{
	classinfo *ci;

	ci = LLNI_classinfo_unwrap(c);

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
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_loadClass(JNIEnv *env, jclass clazz, java_lang_String *name, s4 resolve)
{
	classinfo *c;
	utf       *u;

	if (name == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* create utf string in which '.' is replaced by '/' */

	u = javastring_toutf((java_handle_t *) name, true);

	/* load class */

	c = load_class_bootstrap(u);

	if (c == NULL)
		return NULL;

	/* resolve class -- if requested */

/*  	if (resolve) */
		if (!link_class(c))
			return NULL;

	return LLNI_classinfo_wrap(c);
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    nativeGetResources
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT java_util_Vector* JNICALL Java_java_lang_VMClassLoader_nativeGetResources(JNIEnv *env, jclass clazz, java_lang_String *name)
{
	java_handle_t        *o;         /* vector being created     */
	methodinfo           *m;         /* "add" method of vector   */
	java_handle_t        *path;      /* path to be added         */
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

	utfname = javastring_toutf((java_handle_t *) name, false);

	if (utfname == NULL)
		return NULL;

	/* copy it to a char buffer */

	namelen   = utf_bytes(utfname);
	searchlen = namelen;
	bufsize   = namelen + strlen("0");
	buffer    = MNEW(char, bufsize);

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

	if (o == NULL)
		goto return_NULL;

	/* get Vector.add() method */

	m = class_resolveclassmethod(class_java_util_Vector,
								 utf_add,
								 utf_new_char("(Ljava/lang/Object;)Z"),
								 NULL,
								 true);

	if (m == NULL)
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

		if (path != NULL) {
			ret = vm_call_method_int(m, o, path);

			if (exceptions_get_exception() != NULL)
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
 * Method:    defaultAssertionStatus
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClassLoader_defaultAssertionStatus(JNIEnv *env, jclass clazz)
{
	return _Jv_jvm->Java_java_lang_VMClassLoader_defaultAssertionStatus;
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    findLoadedClass
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_findLoadedClass(JNIEnv *env, jclass clazz, java_lang_ClassLoader *loader, java_lang_String *name)
{
	classloader *cl;
	classinfo   *c;
	utf         *u;

	/* XXX is it correct to add the classloader to the hashtable here? */

	cl = loader_hashtable_classloader_add((java_handle_t *) loader);

	/* replace `.' by `/', this is required by the classcache */

	u = javastring_toutf((java_handle_t *) name, true);

	/* lookup for defining classloader */

	c = classcache_lookup_defined(cl, u);

	/* if not found, lookup for initiating classloader */

	if (c == NULL)
		c = classcache_lookup(cl, u);

	return LLNI_classinfo_wrap(c);
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
