/* src/native/vm/cldc1.1/java_lang_Throwable.c - java/lang/Throwable

   Copyright (C) 2006, 2007 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Throwable.h"

#include "vm/exceptions.h"
#include "vm/jit/stacktrace.h"


/* native methods implemented by this file ************************************/
 
static JNINativeMethod methods[] = {
	{ "printStackTrace",  "()V", (void *) (ptrint) &Java_java_lang_Throwable_printStackTrace  },
	{ "fillInStackTrace", "()V", (void *) (ptrint) &Java_java_lang_Throwable_fillInStackTrace },
};


/* _Jv_java_lang_Throwable_init ************************************************
 
   Register native functions.
 
*******************************************************************************/
 
void _Jv_java_lang_Throwable_init(void)
{
	utf *u;
 
	u = utf_new_char("java/lang/Throwable");
 
	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     java/lang/Throwable
 * Method:    printStackTrace
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Throwable_printStackTrace(JNIEnv *env, java_lang_Throwable *this)
{
	java_handle_t *o;

	o = (java_handle_t *) this;

	exceptions_print_exception(o);
	stacktrace_print_trace(o);
}


/*
 * Class:     java/lang/Throwable
 * Method:    fillInStackTrace
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Throwable_fillInStackTrace(JNIEnv *env, java_lang_Throwable *this)
{
	java_handle_bytearray_t *ba;

	ba = stacktrace_fillInStackTrace();

	if (ba == NULL)
		return;

	this->backtrace = (java_lang_Object *) ba;
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
