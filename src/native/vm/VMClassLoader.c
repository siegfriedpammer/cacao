/* native/vm/VMClassLoader.c - java/lang/VMClassLoader

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Roman Obermaiser

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: VMClassLoader.c 2188 2005-04-02 01:15:33Z edwin $

*/


#include <sys/stat.h>

#include "config.h"
#include "types.h"
#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_ClassLoader.h"
#include "native/include/java_security_ProtectionDomain.h"
#include "native/include/java_util_Vector.h"
#include "toolbox/logging.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/builtin.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"


/*
 * Class:     java/lang/VMClassLoader
 * Method:    defineClass
 * Signature: (Ljava/lang/ClassLoader;Ljava/lang/String;[BIILjava/security/ProtectionDomain;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_defineClass(JNIEnv *env, jclass clazz, java_lang_ClassLoader *this, java_lang_String *name, java_bytearray *buf, s4 off, s4 len, java_security_ProtectionDomain *pd)
{
	classinfo   *c;
	classinfo   *r;
	classbuffer *cb;

	if ((off < 0) || (len < 0) || ((off + len) > buf->header.size)) {
		*exceptionptr =
			new_exception(string_java_lang_IndexOutOfBoundsException);
		return NULL;
	}

	if (!name) {
		/* XXX we have to support this case (name is read from classbuffer) */
		log_text("defineClass(name == NULL,...) is not implemented, yet");
		*exceptionptr = new_nullpointerexception();
		return NULL;
	}

	/* convert '.' to '/' in java string */

	c = class_new(javastring_toutf(name, true));

#if defined(USE_THREADS)
	/* enter a monitor on the class */

	builtin_monitorenter((java_objectheader *) c);
#endif

#if defined(STATISTICS)
	/* measure time */

	if (getloadingtime)
		loadingtime_start();
#endif

	/* build a classbuffer with the given data */

	cb = NEW(classbuffer);
	cb->class = c;
	cb->size = len;
	cb->data = (u1 *) &buf->data[off];
	cb->pos = cb->data - 1;

	/* preset the defining classloader */

	c->classloader = (java_objectheader *) this;

	/* load the class from this buffer */

	r = load_class_from_classbuffer(cb);

	/* free memory */

	FREE(cb, classbuffer);

#if defined(STATISTICS)
	/* measure time */

	if (getloadingtime)
		loadingtime_stop();
#endif

#if defined(USE_THREADS)
	/* leave the monitor */

	builtin_monitorexit((java_objectheader *) c);
#endif

	/* exception? return! */

	if (!r) {
		/* If return value is NULL, we had a problem and the class is not     */
		/* loaded. */

		c->loaded = false;

		/* now free the allocated memory, otherwise we could ran into a DOS */

		class_remove(c);

		return NULL;
	}

	/* set ProtectionDomain */

	c->pd = pd;

	use_class_as_object(c);

	return (java_lang_Class *) c;
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    getPrimitiveClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_getPrimitiveClass(JNIEnv *env, jclass clazz, java_lang_String *type)
{
	classinfo *c;
	utf       *u;

	u = javastring_toutf(type, false);

	/* illegal primitive classname specified */

	if (!u) {
		*exceptionptr = new_exception(string_java_lang_ClassNotFoundException);
		return NULL;
	}

	/* get primitive class */

	c = class_new(u);

	if (!load_class_bootstrap(c) || !class_init(c))
		return NULL;

	use_class_as_object(c);

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
		*exceptionptr = new_nullpointerexception();
		return;
	}

	/* link the class */

	if (!ci->linked)
		link_class(ci);

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

	if (!name) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return NULL;
	}

	/* create utf string in which '.' is replaced by '/' */

	u = javastring_toutf(name, true);

	/* create class */

	c = class_new(u);

	/* load class */

	if (!load_class_bootstrap(c))
		goto exception;

	/* resolve class -- if requested */
	/* XXX TWISTI: we do not support REAL (at runtime) lazy linking */
/*  	if (resolve) { */
		if (!link_class(c))
			goto exception;

		use_class_as_object(c);
/*  	} */

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
	jobject           o;
	methodinfo       *m;
	java_lang_String *path;
	classpath_info   *cpi;
	utf              *utfname;
	char             *charname;
	char             *tmppath;
	s4                namelen;
	s4                pathlen;
	struct stat       buf;
	jboolean          ret;

	/* get the resource name as utf string */

	utfname = javastring_toutf(name, false);

	namelen = utf_strlen(utfname) + strlen("0");
	charname = MNEW(char, namelen);

	utf_sprint(charname, utfname);

	/* new Vector() */

	o = native_new_and_init(class_java_util_Vector);

	if (!o)
		return NULL;

	/* get v.add() method */

	m = class_resolveclassmethod(class_java_util_Vector,
								 utf_new_char("add"),
								 utf_new_char("(Ljava/lang/Object;)Z"),
								 NULL,
								 true);

	if (!m)
		return NULL;

	for (cpi = classpath_entries; cpi != NULL; cpi = cpi->next) {
		/* clear path pointer */
  		path = NULL;

#if defined(USE_ZLIB)
		if (cpi->type == CLASSPATH_ARCHIVE) {

#if defined(USE_THREADS)
			/* enter a monitor on zip/jar archives */

			builtin_monitorenter((java_objectheader *) cpi);
#endif

			if (cacao_locate(cpi->uf, utfname) == UNZ_OK) {
				pathlen = strlen("jar:file://") + cpi->pathlen + strlen("!/") +
					namelen + strlen("0");

				tmppath = MNEW(char, pathlen);

				sprintf(tmppath, "jar:file://%s!/%s", cpi->path, charname);
				path = javastring_new_char(tmppath),

				MFREE(tmppath, char, pathlen);
			}

#if defined(USE_THREADS)
			/* leave the monitor */

			builtin_monitorexit((java_objectheader *) cpi);
#endif

		} else {
#endif /* defined(USE_ZLIB) */
			pathlen = strlen("file://") + cpi->pathlen + namelen + strlen("0");

			tmppath = MNEW(char, pathlen);

			sprintf(tmppath, "file://%s%s", cpi->path, charname);

			if (stat(tmppath + strlen("file://") - 1, &buf) == 0)
				path = javastring_new_char(tmppath),

			MFREE(tmppath, char, pathlen);
#if defined(USE_ZLIB)
		}
#endif

		/* if a resource was found, add it to the vector */

		if (path) {
			ret = (jboolean) asm_calljavafunction_int(m, o, path, NULL, NULL);

			if (!ret)
				return NULL;
		}
	}

	return (java_util_Vector *) o;
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
