/* src/native/vm/gnuclasspath/java_lang_VMClassLoader.cpp

   Copyright (C) 1996-2005, 2006, 2007, 2008
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

#include <assert.h>
#include <stdint.h>
#include <sys/stat.h>

#include "mm/memory.h"

#include "native/jni.hpp"
#include "native/llni.h"
#include "native/native.h"

#if defined(ENABLE_JNI_HEADERS)
# include "native/vm/include/java_lang_VMClassLoader.h"
#endif

#include "toolbox/logging.h"
#include "toolbox/list.h"

#if defined(ENABLE_ASSERTION)
#include "vm/assertion.h"
#endif

#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/classcache.h"
#include "vm/exceptions.hpp"
#include "vm/globals.hpp"
#include "vm/initialize.h"
#include "vm/javaobjects.hpp"
#include "vm/linker.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/primitive.hpp"
#include "vm/statistics.h"
#include "vm/string.hpp"
#include "vm/suck.h"
#include "vm/vm.hpp"
#include "vm/zip.h"

#include "vm/jit/asmpart.h"

#if defined(ENABLE_JVMTI)
#include "native/jvmti/cacaodbg.h"
#endif


// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     java/lang/VMClassLoader
 * Method:    defineClass
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/String;[BIILjava/security/ProtectionDomain;)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_defineClass(JNIEnv *env, jclass clazz, jobject cl, jstring name, jbyteArray data, jint offset, jint len, jobject pd)
{
	utf             *utfname;
	classinfo       *c;
	classloader_t   *loader;
	java_handle_bytearray_t* ba;
	uint8_t*                 stream;

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

	if ((offset < 0) || (len < 0) || ((offset + len) > LLNI_array_size(data))) {
		exceptions_throw_arrayindexoutofboundsexception();
		return NULL;
	}

	/* add classloader to classloader hashtable */

	loader = loader_hashtable_classloader_add((java_handle_t *) cl);

	if (name != NULL) {
		/* convert '.' to '/' in java string */

		utfname = javastring_toutf((java_handle_t *) name, true);
	} 
	else {
		utfname = NULL;
	}

#if defined(ENABLE_JVMTI)
	/* XXX again this will not work because of the indirection cell for classloaders */
	assert(0);
	/* fire Class File Load Hook JVMTI event */

	if (jvmti)
		jvmti_ClassFileLoadHook(utfname, len, (unsigned char *) data->data, 
								loader, (java_handle_t *) pd, 
								&new_class_data_len, &new_class_data);
#endif

	/* define the class */

#if defined(ENABLE_JVMTI)
	/* check if the JVMTI wants to modify the class */

	if (new_class_data == NULL)
		c = class_define(utfname, loader, new_class_data_len, new_class_data, pd); 
	else
#endif
	{
		ba = (java_handle_bytearray_t*) data;
		stream = (uint8_t *) &LLNI_array_direct(ba, offset);
		c = class_define(utfname, loader, len, stream, (java_handle_t *) pd);
	}

	if (c == NULL)
		return NULL;

	// REMOVEME
	java_handle_t* h = LLNI_classinfo_wrap(c);

	// Set ProtectionDomain.
	java_lang_Class jlc(h);
	jlc.set_pd(pd);

	return (jclass) jlc.get_handle();
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    getPrimitiveClass
 * Signature: (C)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_getPrimitiveClass(JNIEnv *env, jclass clazz, jchar type)
{
	classinfo *c;

	c = Primitive::get_class_by_char(type);

	if (c == NULL) {
		exceptions_throw_classnotfoundexception(utf_null);
		return NULL;
	}

	return (jclass) LLNI_classinfo_wrap(c);
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    resolveClass
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClassLoader_resolveClass(JNIEnv *env, jclass clazz, jclass c)
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
JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_loadClass(JNIEnv *env, jclass clazz, jstring name, jboolean resolve)
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

	return (jclass) LLNI_classinfo_wrap(c);
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    nativeGetResources
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMClassLoader_nativeGetResources(JNIEnv *env, jclass clazz, jstring name)
{
	java_handle_t        *o;         /* vector being created     */
	methodinfo           *m;         /* "add" method of vector   */
	java_handle_t        *path;      /* path to be added         */
	list_classpath_entry *lce;       /* classpath entry          */
	utf                  *utfname;   /* utf to look for          */
	char                 *buffer;    /* char buffer              */
	char                 *namestart; /* start of name to use     */
	char                 *tmppath;   /* temporary buffer         */
	int32_t               namelen;   /* length of name to use    */
	int32_t               searchlen; /* length of name to search */
	int32_t               bufsize;   /* size of buffer allocated */
	int32_t               pathlen;   /* name of path to assemble */
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

	for (lce = (list_classpath_entry*) list_first(list_classpath_entries); lce != NULL;
		 lce = (list_classpath_entry*) list_next(list_classpath_entries, lce)) {
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

	return (jobject) o;

return_NULL:
	MFREE(buffer, char, bufsize);

	return NULL;
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    defaultAssertionStatus
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_VMClassLoader_defaultAssertionStatus(JNIEnv *env, jclass clazz)
{
#if defined(ENABLE_ASSERTION)
	return assertion_system_enabled;
#else
	return false;
#endif
}

/*
 * Class:     java/lang/VMClassLoader
 * Method:    userAssertionStatus
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_VMClassLoader_defaultUserAssertionStatus(JNIEnv *env, jclass clazz)
{
#if defined(ENABLE_ASSERTION)
	return assertion_user_enabled;
#else
	return false;
#endif
}

/*
 * Class:     java/lang/VMClassLoader
 * Method:    packageAssertionStatus
 * Signature: (Ljava/lang/Boolean;Ljava/lang/Boolean;)Ljava/util/Map;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMClassLoader_packageAssertionStatus0(JNIEnv *env, jclass clazz, jobject jtrue, jobject jfalse)
{
	java_handle_t     *hm;
#if defined(ENABLE_ASSERTION)
	java_handle_t     *js;
	methodinfo        *m;
	assertion_name_t  *item;
#endif

	/* new HashMap() */

	hm = native_new_and_init(class_java_util_HashMap);
	if (hm == NULL) {
		return NULL;
	}

#if defined(ENABLE_ASSERTION)
	/* if nothing todo, return now */

	if (assertion_package_count == 0) {
		return (jobject) hm;
	}

	/* get HashMap.put method */

	m = class_resolveclassmethod(class_java_util_HashMap,
                                 utf_put,
                                 utf_new_char("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"),
                                 NULL,
                                 true);

	if (m == NULL) {
		return NULL;
	}

	item = (assertion_name_t *)list_first(list_assertion_names);

	while (item != NULL) {
		if (item->package == false) {
			item = (assertion_name_t *)list_next(list_assertion_names, item);
			continue;
		}
		
		if (strcmp(item->name, "") == 0) {
			/* unnamed package wanted */
			js = NULL;
		}
		else {
			js = javastring_new_from_ascii(item->name);
			if (js == NULL) {
				return NULL;
			}
		}

		if (item->enabled == true) {
			vm_call_method(m, hm, js, jtrue);
		}
		else {
			vm_call_method(m, hm, js, jfalse);
		}

		item = (assertion_name_t *)list_next(list_assertion_names, item);
	}
#endif

	return (jobject) hm;
}

/*
 * Class:     java/lang/VMClassLoader
 * Method:    classAssertionStatus
 * Signature: (Ljava/lang/Boolean;Ljava/lang/Boolean;)Ljava/util/Map;
 */
JNIEXPORT jobject JNICALL Java_java_lang_VMClassLoader_classAssertionStatus0(JNIEnv *env, jclass clazz, jobject jtrue, jobject jfalse)
{
	java_handle_t     *hm;
#if defined(ENABLE_ASSERTION)
	java_handle_t     *js;
	methodinfo        *m;
	assertion_name_t  *item;
#endif

	/* new HashMap() */

	hm = native_new_and_init(class_java_util_HashMap);
	if (hm == NULL) {
		return NULL;
	}

#if defined(ENABLE_ASSERTION)
	/* if nothing todo, return now */

	if (assertion_class_count == 0) {
		return (jobject) hm;
	}

	/* get HashMap.put method */

	m = class_resolveclassmethod(class_java_util_HashMap,
                                 utf_put,
                                 utf_new_char("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"),
                                 NULL,
                                 true);

	if (m == NULL) {
		return NULL;
	}

	item = (assertion_name_t *)list_first(list_assertion_names);

	while (item != NULL) {
		if (item->package == true) {
			item = (assertion_name_t *)list_next(list_assertion_names, item);
			continue;
		}

		js = javastring_new_from_ascii(item->name);
		if (js == NULL) {
			return NULL;
		}

		if (item->enabled == true) {
			vm_call_method(m, hm, js, jtrue);
		}
		else {
			vm_call_method(m, hm, js, jfalse);
		}

		item = (assertion_name_t *)list_next(list_assertion_names, item);
	}
#endif

	return (jobject) hm;
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    findLoadedClass
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_java_lang_VMClassLoader_findLoadedClass(JNIEnv *env, jclass clazz, jobject loader, jstring name)
{
	classloader_t *cl;
	classinfo     *c;
	utf           *u;

	/* XXX is it correct to add the classloader to the hashtable here? */

	cl = loader_hashtable_classloader_add((java_handle_t *) loader);

	/* replace `.' by `/', this is required by the classcache */

	u = javastring_toutf((java_handle_t *) name, true);

	/* lookup for defining classloader */

	c = classcache_lookup_defined(cl, u);

	/* if not found, lookup for initiating classloader */

	if (c == NULL)
		c = classcache_lookup(cl, u);

	return (jclass) LLNI_classinfo_wrap(c);
}

} // extern "C"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ (char*) "defineClass",                (char*) "(Ljava/lang/ClassLoader;Ljava/lang/String;[BIILjava/security/ProtectionDomain;)Ljava/lang/Class;", (void*) (uintptr_t) &Java_java_lang_VMClassLoader_defineClass                },
	{ (char*) "getPrimitiveClass",          (char*) "(C)Ljava/lang/Class;",                                                                             (void*) (uintptr_t) &Java_java_lang_VMClassLoader_getPrimitiveClass          },
	{ (char*) "resolveClass",               (char*) "(Ljava/lang/Class;)V",                                                                             (void*) (uintptr_t) &Java_java_lang_VMClassLoader_resolveClass               },
	{ (char*) "loadClass",                  (char*) "(Ljava/lang/String;Z)Ljava/lang/Class;",                                                           (void*) (uintptr_t) &Java_java_lang_VMClassLoader_loadClass                  },
	{ (char*) "nativeGetResources",         (char*) "(Ljava/lang/String;)Ljava/util/Vector;",                                                           (void*) (uintptr_t) &Java_java_lang_VMClassLoader_nativeGetResources         },
	{ (char*) "defaultAssertionStatus",     (char*) "()Z",                                                                                              (void*) (uintptr_t) &Java_java_lang_VMClassLoader_defaultAssertionStatus     },
	{ (char*) "defaultUserAssertionStatus", (char*) "()Z",                                                                                              (void*) (uintptr_t) &Java_java_lang_VMClassLoader_defaultUserAssertionStatus },
	{ (char*) "packageAssertionStatus0",    (char*) "(Ljava/lang/Boolean;Ljava/lang/Boolean;)Ljava/util/Map;",                                          (void*) (uintptr_t) &Java_java_lang_VMClassLoader_packageAssertionStatus0    },
	{ (char*) "classAssertionStatus0",      (char*) "(Ljava/lang/Boolean;Ljava/lang/Boolean;)Ljava/util/Map;",                                          (void*) (uintptr_t) &Java_java_lang_VMClassLoader_classAssertionStatus0      },
	{ (char*) "findLoadedClass",            (char*) "(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;",                                     (void*) (uintptr_t) &Java_java_lang_VMClassLoader_findLoadedClass            },
};


/* _Jv_java_lang_VMClassLoader_init ********************************************

   Register native functions.

*******************************************************************************/

// FIXME
extern "C" {
void _Jv_java_lang_VMClassLoader_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/VMClassLoader");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}
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
