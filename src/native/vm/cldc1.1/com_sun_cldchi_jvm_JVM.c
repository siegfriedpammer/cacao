/* src/native/vm/cldc1.1/com_sun_cldchi_jvm_JVM.c

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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

   $Id: java_lang_VMRuntime.c 5900 2006-11-04 17:30:44Z michi $

*/


#include "config.h"
#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"

#include "native/include/java_lang_String.h"

#include "native/include/com_sun_cldchi_jvm_JVM.h"

#include "native/vm/java_lang_Runtime.h"

#include "vm/exceptions.h"
#include "vm/stringlocal.h"


/* native methods implemented by this file ************************************/
 
static JNINativeMethod methods[] = {
	{ "loadLibrary", "(Ljava/lang/String;)V", (void *) (ptrint) &Java_com_sun_cldchi_jvm_JVM_loadLibrary },
};


/* _Jv_com_sun_cldchi_jvm_JVM_init *********************************************
 
   Register native functions.
 
*******************************************************************************/
 
void _Jv_com_sun_cldchi_jvm_JVM_init(void)
{
	utf *u;
 
	u = utf_new_char("com/sun/cldchi/jvm/JVM");
 
	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     com/sun/cldchi/jvm/JVM
 * Method:    loadLibrary
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_sun_cldchi_jvm_JVM_loadLibrary(JNIEnv *env, jclass clazz, java_lang_String *libName)
{
	s4   result;
	utf *name;

#if defined(ENABLE_JNI)
	result = _Jv_java_lang_Runtime_loadLibrary(env, libName, NULL);
#else
	result = _Jv_java_lang_Runtime_loadLibrary(libName, NULL);
#endif

	/* check for error and throw one in case */

	if (result == 0) {
		name = javastring_toutf((java_handle_t *) libName, false);
		exceptions_throw_unsatisfiedlinkerror(name);
	}
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
