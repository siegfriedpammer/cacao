/* src/native/vm/java_lang_VMRuntime.c

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

#if defined(ENABLE_LTDL) && defined(HAVE_LTDL_H)
# include <ltdl.h>
#endif

#include "vm/types.h"

#include "mm/gc-common.h"

#include "native/jni.h"
#include "native/native.h"

#include "native/include/java_lang_String.h"

#include "toolbox/logging.h"

#include "vm/exceptions.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vmcore/options.h"


/* should we run all finalizers on exit? */
static bool finalizeOnExit = false;


/*
 * Class:     java/lang/Runtime
 * Method:    exitInternal
 * Signature: (I)V
 */
void _Jv_java_lang_Runtime_exit(s4 status)
{
	if (finalizeOnExit)
		gc_finalize_all();

	vm_shutdown(status);
}


/*
 * Class:     java/lang/Runtime
 * Method:    freeMemory
 * Signature: ()J
 */
s8 _Jv_java_lang_Runtime_freeMemory(void)
{
	return gc_get_free_bytes();
}


/*
 * Class:     java/lang/Runtime
 * Method:    totalMemory
 * Signature: ()J
 */
s8 _Jv_java_lang_Runtime_totalMemory(void)
{
	return gc_get_heap_size();
}


/*
 * Class:     java/lang/Runtime
 * Method:    gc
 * Signature: ()V
 */
void _Jv_java_lang_Runtime_gc(void)
{
	gc_call();
}


/*
 * Class:     java/lang/Runtime
 * Method:    loadLibrary
 * Signature: (Ljava/lang/String;Ljava/lang/ClassLoader;)I
 */
#if defined(ENABLE_JNI)
s4 _Jv_java_lang_Runtime_loadLibrary(JNIEnv *env, java_lang_String *libname, classloader *cl)
#else
s4 _Jv_java_lang_Runtime_loadLibrary(java_lang_String *libname, classloader *cl)
#endif
{
#if defined(ENABLE_LTDL)
	utf               *name;
	lt_dlhandle        handle;
# if defined(ENABLE_JNI)
	lt_ptr             onload;
	s4                 version;
# endif

	if (libname == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}

	name = javastring_toutf((java_handle_t *) libname, false);

	/* is the library already loaded? */

	if (native_library_find(name, cl) != NULL)
		return 1;

	/* open the library */

	handle = native_library_open(name);

	if (handle == NULL)
		return 0;

# if defined(ENABLE_JNI)
	/* resolve JNI_OnLoad function */

	onload = lt_dlsym(handle, "JNI_OnLoad");

	if (onload != NULL) {
		JNIEXPORT s4 (JNICALL *JNI_OnLoad) (JavaVM *, void *);
		JavaVM *vm;

		JNI_OnLoad = (JNIEXPORT s4 (JNICALL *)(JavaVM *, void *)) (ptrint) onload;

		(*env)->GetJavaVM(env, &vm);

		version = JNI_OnLoad(vm, NULL);

		/* if the version is not 1.2 and not 1.4 the library cannot be loaded */

		if ((version != JNI_VERSION_1_2) && (version != JNI_VERSION_1_4)) {
			lt_dlclose(handle);

			return 0;
		}
	}
# endif

	/* insert the library name into the library hash */

	native_library_add(name, cl, handle);

	return 1;
#else
	vm_abort("_Jv_java_lang_Runtime_loadLibrary: not available");

	/* keep compiler happy */

	return 0;
#endif
}


#if defined(ENABLE_JAVASE)

/*
 * Class:     java/lang/Runtime
 * Method:    runFinalizersOnExit
 * Signature: (Z)V
 */
void _Jv_java_lang_Runtime_runFinalizersOnExit(s4 value)
{
	/* XXX threading */

	finalizeOnExit = value;
}

#endif


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
