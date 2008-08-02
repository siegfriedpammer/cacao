/* src/native/vm/cldc1.1/java_lang_Throwable.cpp

   Copyright (C) 2006, 2007, 2008
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

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Object.h"

// FIXME
extern "C" {
#include "native/include/java_lang_Throwable.h"
}

#include "vm/exceptions.hpp"
#include "vm/jit/stacktrace.hpp"


/* native methods implemented by this file ************************************/
 
static JNINativeMethod methods[] = {
	{ (char*) "printStackTrace",  (char*) "()V", (void*) (uintptr_t) &Java_java_lang_Throwable_printStackTrace  },
	{ (char*) "fillInStackTrace", (char*) "()V", (void*) (uintptr_t) &Java_java_lang_Throwable_fillInStackTrace },
};


/* _Jv_java_lang_Throwable_init ************************************************
 
   Register native functions.
 
*******************************************************************************/
 
// FIXME
extern "C" {
void _Jv_java_lang_Throwable_init(void)
{
	utf *u;
 
	u = utf_new_char("java/lang/Throwable");
 
	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}
}


// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     java/lang/Throwable
 * Method:    printStackTrace
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Throwable_printStackTrace(JNIEnv *env, java_lang_Throwable *_this)
{
	java_handle_t *o;

	o = (java_handle_t *) _this;

	exceptions_print_exception(o);
	stacktrace_print_exception(o);
}


/*
 * Class:     java/lang/Throwable
 * Method:    fillInStackTrace
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Throwable_fillInStackTrace(JNIEnv *env, java_lang_Throwable *_this)
{
	java_handle_bytearray_t *ba;

	ba = stacktrace_get_current();

	if (ba == NULL)
		return;

	LLNI_field_set_ref(_this, backtrace, (java_lang_Object *) ba);
}

} // extern "C"


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
 */
