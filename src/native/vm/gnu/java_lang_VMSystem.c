/* src/native/vm/gnu/java_lang_VMSystem.c - java/lang/VMSystem

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

   $Id: java_lang_VMSystem.c 8363 2007-08-20 19:10:46Z michi $

*/


#include "config.h"

#include <string.h>

#include "vm/types.h"

#include "mm/gc-common.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_io_InputStream.h"        /* required by j.l.VMS */
#include "native/include/java_io_PrintStream.h"        /* required by j.l.VMS */

#include "native/include/java_lang_VMSystem.h"

#include "vm/builtin.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "arraycopy",        "(Ljava/lang/Object;ILjava/lang/Object;II)V", (void *) (ptrint) &Java_java_lang_VMSystem_arraycopy },
	{ "identityHashCode", "(Ljava/lang/Object;)I",                      (void *) (ptrint) &Java_java_lang_VMSystem_identityHashCode },
};


/* _Jv_java_lang_VMSystem_init *************************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_VMSystem_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/VMSystem");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/VMSystem
 * Method:    arraycopy
 * Signature: (Ljava/lang/Object;ILjava/lang/Object;II)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMSystem_arraycopy(JNIEnv *env, jclass clazz, java_lang_Object *src, s4 srcStart, java_lang_Object *dest, s4 destStart, s4 len)
{
	(void) builtin_arraycopy((java_handle_t *) src, srcStart,
							 (java_handle_t *) dest, destStart, len);
}


/*
 * Class:     java/lang/VMSystem
 * Method:    identityHashCode
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMSystem_identityHashCode(JNIEnv *env, jclass clazz, java_lang_Object *o)
{
	s4 hashcode;

	LLNI_CRITICAL_START;

#if defined(ENABLE_GC_CACAO)
	hashcode = heap_get_hashcode(LLNI_UNWRAP((java_handle_t *) o));
#else
	hashcode = (s4) ((ptrint) o);
#endif

	LLNI_CRITICAL_END;

	return hashcode;
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
