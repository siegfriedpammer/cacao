/* src/native/vm/gnu/gnu_classpath_VMStackWalker.c

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

   $Id: gnu_classpath_VMStackWalker.c 8299 2007-08-13 08:41:18Z michi $

*/


#include "config.h"

#include "native/jni.h"
#include "native/native.h"

#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_ClassLoader.h"

#include "native/include/gnu_classpath_VMStackWalker.h"

#include "vm/builtin.h"
#include "vm/global.h"

#include "vm/jit/stacktrace.h"

#include "vmcore/class.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "getClassContext",         "()[Ljava/lang/Class;",      (void *) (ptrint) &Java_gnu_classpath_VMStackWalker_getClassContext         },
	{ "getCallingClass",         "()Ljava/lang/Class;",       (void *) (ptrint) &Java_gnu_classpath_VMStackWalker_getCallingClass         },
	{ "getCallingClassLoader",   "()Ljava/lang/ClassLoader;", (void *) (ptrint) &Java_gnu_classpath_VMStackWalker_getCallingClassLoader   },
	{ "firstNonNullClassLoader", "()Ljava/lang/ClassLoader;", (void *) (ptrint) &Java_gnu_classpath_VMStackWalker_firstNonNullClassLoader },
};


/* _Jv_gnu_classpath_VMStackWalker_init ****************************************

   Register native functions.

*******************************************************************************/

void _Jv_gnu_classpath_VMStackWalker_init(void)
{
	utf *u;

	u = utf_new_char("gnu/classpath/VMStackWalker");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     gnu/classpath/VMStackWalker
 * Method:    getClassContext
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_gnu_classpath_VMStackWalker_getClassContext(JNIEnv *env, jclass clazz)
{
	java_objectarray *oa;

	oa = stacktrace_getClassContext();

	return oa;
}


/*
 * Class:     gnu/classpath/VMStackWalker
 * Method:    getCallingClass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_gnu_classpath_VMStackWalker_getCallingClass(JNIEnv *env, jclass clazz)
{
	java_objectarray *oa;

	oa = stacktrace_getClassContext();

	if (oa == NULL)
		return NULL;

	if (oa->header.size < 2)
		return NULL;

	return (java_lang_Class *) oa->data[1];
}


/*
 * Class:     gnu/classpath/VMStackWalker
 * Method:    getCallingClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT java_lang_ClassLoader* JNICALL Java_gnu_classpath_VMStackWalker_getCallingClassLoader(JNIEnv *env, jclass clazz)
{
	java_objectarray *oa;
	classinfo        *c;
	classloader      *cl;

	oa = stacktrace_getClassContext();

	if (oa == NULL)
		return NULL;

	if (oa->header.size < 2)
		return NULL;
  	 
	c   = (classinfo *) oa->data[1];
	cl  = c->classloader;

	if (cl == NULL)
		return NULL;

	return (java_lang_ClassLoader *) cl->object;
}


/*
 * Class:     gnu/classpath/VMStackWalker
 * Method:    firstNonNullClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT java_lang_ClassLoader* JNICALL Java_gnu_classpath_VMStackWalker_firstNonNullClassLoader(JNIEnv *env, jclass clazz)
{
	java_objectarray *oa;
	classinfo        *c;
	classloader      *cl;
	s4                i;

	oa = stacktrace_getClassContext();

	if (oa == NULL)
		return NULL;

	for (i = 0; i < oa->header.size; i++) {
		c  = (classinfo *) oa->data[i];
		cl = c->classloader;

		if (cl != NULL)
			return (java_lang_ClassLoader *) cl->object;
	}

	return NULL;
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
