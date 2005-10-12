/* src/native/vm/VMStackWalker.c - gnu/classpath/VMStackWalker

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

   Authors: Christian Thalinger

   Changes: 

   $Id: VMStackWalker.c 3407 2005-10-12 08:22:50Z twisti $

*/


#include "config.h"

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_ClassLoader.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/global.h"
#include "vm/tables.h"


/*
 * Class:     gnu/classpath/VMStackWalker
 * Method:    getClassContext
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_gnu_classpath_VMStackWalker_getClassContext(JNIEnv *env, jclass clazz)
{
/*  	if (cacao_initializing) */
/*  		return NULL; */

#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__MIPS__) || defined(__POWERPC__) || defined(__X86_64__)
	/* these JITs support stacktraces, and so does the interpreter */

	return cacao_createClassContextArray();

#else
# if defined(ENABLE_INTRP)
	/* the interpreter supports stacktraces, even if the JIT does not */

	if (opt_intrp) {
		return cacao_createClassContextArray();

	} else
# endif
		{
			return builtin_anewarray(0, class_java_lang_Class);
		}
#endif
}


/*
 * Class:     gnu/classpath/VMStackWalker
 * Method:    getCallingClass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_gnu_classpath_VMStackWalker_getCallingClass(JNIEnv *env, jclass clazz)
{
#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__MIPS__) || defined(__POWERPC__) || defined(__X86_64__)
	/* these JITs support stacktraces, and so does the interpreter */

	java_objectarray *oa;

	oa = cacao_createClassContextArray();

	if (oa->header.size < 2)
		return NULL;

	return (java_lang_Class *) oa->data[1];

#else
# if defined(ENABLE_INTRP)
	/* the interpreter supports stacktraces, even if the JIT does not */

	if (opt_intrp) {
		java_objectarray *oa;

		oa = cacao_createClassContextArray();

		if (oa->header.size < 2)
			return NULL;

		return (java_lang_Class *) oa->data[1];

	} else
# endif
		{
			return NULL;
		}
#endif
}


/*
 * Class:     gnu/classpath/VMStackWalker
 * Method:    getCallingClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT java_lang_ClassLoader* JNICALL Java_gnu_classpath_VMStackWalker_getCallingClassLoader(JNIEnv *env, jclass clazz)
{
#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__MIPS__) || defined(__POWERPC__) || defined(__X86_64__)
	/* these JITs support stacktraces, and so does the interpreter */

	java_objectarray *oa;
	classinfo        *c;

	oa = cacao_createClassContextArray();

	if (oa->header.size < 2)
		return NULL;

	c = (classinfo *) oa->data[1];

	return (java_lang_ClassLoader *) c->classloader;

#else
# if defined(ENABLE_INTRP)
	/* the interpreter supports stacktraces, even if the JIT does not */

	if (opt_intrp) {
		java_objectarray *oa;
		classinfo        *c;

		oa = cacao_createClassContextArray();

		if (oa->header.size < 2)
			return NULL;

		c = (classinfo *) oa->data[1];

		return (java_lang_ClassLoader *) c->classloader;

	} else
# endif
		{
			return NULL;
		}
#endif
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
