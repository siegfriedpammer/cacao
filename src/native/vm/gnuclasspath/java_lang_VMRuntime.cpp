/* src/native/vm/gnuclasspath/java_lang_VMRuntime.cpp

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
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>

#if defined(__DARWIN__)
# if defined(__POWERPC__)
#  define OS_INLINE    /* required for <libkern/ppc/OSByteOrder.h> */
# endif
# include <mach/mach.h>
#endif

#include "mm/memory.h"
#include "mm/gc.hpp"

#include "native/jni.hpp"
#include "native/native.hpp"

#if defined(ENABLE_JNI_HEADERS)
# include "native/vm/include/java_lang_VMRuntime.h"
#endif

#include "vm/jit/builtin.hpp"
#include "vm/exceptions.hpp"
#include "vm/os.hpp"
#include "vm/string.hpp"
#include "vm/utf8.h"
#include "vm/vm.hpp"


static bool finalizeOnExit = false;


// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     java/lang/VMRuntime
 * Method:    exit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_exit(JNIEnv *env, jclass clazz, jint status)
{
	if (finalizeOnExit)
		gc_finalize_all();

	vm_shutdown(status);
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    freeMemory
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_java_lang_VMRuntime_freeMemory(JNIEnv *env, jclass clazz)
{
	return gc_get_free_bytes();
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    totalMemory
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_java_lang_VMRuntime_totalMemory(JNIEnv *env, jclass clazz)
{
	return gc_get_heap_size();
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    maxMemory
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_java_lang_VMRuntime_maxMemory(JNIEnv *env, jclass clazz)
{
	return gc_get_max_heap_size();
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    gc
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_gc(JNIEnv *env, jclass clazz)
{
	gc_call();
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    runFinalization
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalization(JNIEnv *env, jclass clazz)
{
	gc_invoke_finalizers();
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    runFinalizersOnExit
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalizersOnExit(JNIEnv *env, jclass clazz, jboolean value)
{
	/* XXX threading */

	finalizeOnExit = value;
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    runFinalizationsForExit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalizationForExit(JNIEnv *env, jclass clazz)
{
/*  	if (finalizeOnExit) { */
/*  		gc_call(); */
	/* gc_finalize_all(); */
/*  	} */
/*  	log_text("Java_java_lang_VMRuntime_runFinalizationForExit called"); */
	/*gc_finalize_all();*/
	/*gc_invoke_finalizers();*/
	/*gc_call();*/
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    traceInstructions
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_traceInstructions(JNIEnv *env, jclass clazz, jboolean par1)
{
	/* not supported */
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    traceMethodCalls
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_traceMethodCalls(JNIEnv *env, jclass clazz, jboolean par1)
{
	/* not supported */
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    availableProcessors
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_java_lang_VMRuntime_availableProcessors(JNIEnv *env, jclass clazz)
{
	return os::processors_online();
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    nativeLoad
 * Signature: (Ljava/lang/String;Ljava/lang/ClassLoader;)I
 */
JNIEXPORT jint JNICALL Java_java_lang_VMRuntime_nativeLoad(JNIEnv *env, jclass clazz, jstring libname, jobject loader)
{
	classloader_t *cl;
	utf           *name;

	cl = loader_hashtable_classloader_add((java_handle_t *) loader);

	/* REMOVEME When we use Java-strings internally. */

	if (libname == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}

	name = javastring_toutf((java_handle_t *) libname, false);

	return native_library_load(env, name, cl);
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    mapLibraryName
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_java_lang_VMRuntime_mapLibraryName(JNIEnv *env, jclass clazz, jstring libname)
{
	utf           *u;
	char          *buffer;
	int32_t        buffer_len;
	java_handle_t *o;

	if (libname == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	u = javastring_toutf((java_handle_t *) libname, false);

	/* calculate length of library name */

	buffer_len =
		strlen(NATIVE_LIBRARY_PREFIX) +
		utf_bytes(u) +
		strlen(NATIVE_LIBRARY_SUFFIX) +
		strlen("0");

	buffer = MNEW(char, buffer_len);

	/* generate library name */

	strcpy(buffer, NATIVE_LIBRARY_PREFIX);
	utf_cat(buffer, u);
	strcat(buffer, NATIVE_LIBRARY_SUFFIX);

	o = javastring_new_from_utf_string(buffer);

	/* release memory */

	MFREE(buffer, char, buffer_len);

	return (jstring) o;
}

} // extern "C"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ (char*) "exit",                   (char*) "(I)V",                                         (void*) (uintptr_t) &Java_java_lang_VMRuntime_exit                   },
	{ (char*) "freeMemory",             (char*) "()J",                                          (void*) (uintptr_t) &Java_java_lang_VMRuntime_freeMemory             },
	{ (char*) "totalMemory",            (char*) "()J",                                          (void*) (uintptr_t) &Java_java_lang_VMRuntime_totalMemory            },
	{ (char*) "maxMemory",              (char*) "()J",                                          (void*) (uintptr_t) &Java_java_lang_VMRuntime_maxMemory              },
	{ (char*) "gc",                     (char*) "()V",                                          (void*) (uintptr_t) &Java_java_lang_VMRuntime_gc                     },
	{ (char*) "runFinalization",        (char*) "()V",                                          (void*) (uintptr_t) &Java_java_lang_VMRuntime_runFinalization        },
	{ (char*) "runFinalizersOnExit",    (char*) "(Z)V",                                         (void*) (uintptr_t) &Java_java_lang_VMRuntime_runFinalizersOnExit    },
	{ (char*) "runFinalizationForExit", (char*) "()V",                                          (void*) (uintptr_t) &Java_java_lang_VMRuntime_runFinalizationForExit },
	{ (char*) "traceInstructions",      (char*) "(Z)V",                                         (void*) (uintptr_t) &Java_java_lang_VMRuntime_traceInstructions      },
	{ (char*) "traceMethodCalls",       (char*) "(Z)V",                                         (void*) (uintptr_t) &Java_java_lang_VMRuntime_traceMethodCalls       },
	{ (char*) "availableProcessors",    (char*) "()I",                                          (void*) (uintptr_t) &Java_java_lang_VMRuntime_availableProcessors    },
	{ (char*) "nativeLoad",             (char*) "(Ljava/lang/String;Ljava/lang/ClassLoader;)I", (void*) (uintptr_t) &Java_java_lang_VMRuntime_nativeLoad             },
	{ (char*) "mapLibraryName",         (char*) "(Ljava/lang/String;)Ljava/lang/String;",       (void*) (uintptr_t) &Java_java_lang_VMRuntime_mapLibraryName         },
};


/* _Jv_java_lang_VMRuntime_init ************************************************

   Register native functions.

*******************************************************************************/

// FIXME
extern "C" {
void _Jv_java_lang_VMRuntime_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/VMRuntime");

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
