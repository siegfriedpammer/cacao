/* src/native/vm/gnu/gnu_classpath_VMStackWalker.c

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

#include "native/jni.h"
#include "native/native.h"

#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_ClassLoader.h"

#include "native/include/gnu_classpath_VMStackWalker.h"

#include "vm/global.h"

#include "vm/jit/stacktrace.h"

#include "vmcore/class.h"
#include "vmcore/utf8.h"


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
JNIEXPORT java_handle_objectarray_t* JNICALL Java_gnu_classpath_VMStackWalker_getClassContext(JNIEnv *env, jclass clazz)
{
	java_handle_objectarray_t *oa;

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
	classinfo *c;

	c = stacktrace_get_caller_class(2);

	return (java_lang_Class *) c;
}


/*
 * Class:     gnu/classpath/VMStackWalker
 * Method:    getCallingClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT java_lang_ClassLoader* JNICALL Java_gnu_classpath_VMStackWalker_getCallingClassLoader(JNIEnv *env, jclass clazz)
{
	classinfo     *c;
	classloader_t *cl;

	c  = stacktrace_get_caller_class(2);
	cl = class_get_classloader(c);

	return (java_lang_ClassLoader *) cl;
}


/*
 * Class:     gnu/classpath/VMStackWalker
 * Method:    firstNonNullClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT java_lang_ClassLoader* JNICALL Java_gnu_classpath_VMStackWalker_firstNonNullClassLoader(JNIEnv *env, jclass clazz)
{
	classloader_t *cl;

	cl = stacktrace_first_nonnull_classloader();

	return (java_lang_ClassLoader *) cl;
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
