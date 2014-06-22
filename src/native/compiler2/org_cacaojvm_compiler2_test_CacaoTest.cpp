/* src/native/compiler2/org_cacaojvm_compiler2_test_CacaoTest.cpp

   Copyright (C) 1996-2014
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

#include "vm/vm.hpp"
#include "vm/string.hpp"

#include "vm/jit/argument.hpp"
#include "vm/jit/jit.hpp"

#include "threads/thread.hpp"

#include "native/jni.hpp"
#include "native/llni.hpp"
#include "native/native.hpp"

#include "vm/jit/compiler2/Compiler.hpp"

int32_t call_method(methodinfo *m, jobjectArray args) {
	// leave the nativeworld
	THREAD_NATIVEWORLD_EXIT;

	uint64_t *array = argument_vmarray_from_objectarray(m, NULL, args);
	int32_t result = vm_call_int_array(m, array);

	// enter the nativeworld again
	THREAD_NATIVEWORLD_ENTER;

	return result;
}

// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     org/cacaojvm/compiler2/test/CacaoTest
 * Method:    compileMethod
 * Signature: (ZLjava/lang/Class;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_org_cacaojvm_compiler2_test_CacaoTest_compileMethod(JNIEnv *env, jclass clazz, jboolean baseline, jclass compile_class, jstring name, jstring desc, jobjectArray args) {
	classinfo *ci;

	ci = LLNI_classinfo_unwrap(compile_class);

	if (!ci) {
		exceptions_throw_nullpointerexception();
		os::abort();
	}

	if (name == NULL) {
		exceptions_throw_nullpointerexception();
		os::abort();
	}

	// create utf string in which '.' is replaced by '/'
	Utf8String u = JavaString((java_handle_t*) name).to_utf8_dot_to_slash();
	Utf8String d = JavaString((java_handle_t*) desc).to_utf8_dot_to_slash();

	// find the method of the class
	methodinfo *m = class_resolveclassmethod(ci, u, d, NULL, false);

	if (exceptions_get_exception()) {
		exceptions_print_stacktrace();
		os::abort();
	}

	// there is no main method or it isn't static
	if ((m == NULL) || !(m->flags & ACC_STATIC)) {
		exceptions_clear_exception();
		exceptions_throw_nosuchmethoderror(ci, u, d);
		exceptions_print_stacktrace();
		os::abort();
	}

	// back up and reset code
	codeinfo *code = m->code;
	m->code = NULL;
	int32_t result;

	if (baseline) {
		// baseline compiler
		if (!jit_compile(m)) {
			os::abort();
		}
		result = call_method(m, args);
	}
	else {
		// compiler2 compiler
		if (!cacao::jit::compiler2::compile(m)) {
			os::abort();
		}
		result = call_method(m, args);
	}

	// restore code
	// TODO free code!
	m->code = code;

	// exception occurred?
	if (exceptions_get_exception()) {
		exceptions_print_stacktrace();
		os::abort();
	}

	return result;
}

} // extern "C"

/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ (char*) "compileMethod", (char*) "(ZLjava/lang/Class;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/Object;)I",(void*) (uintptr_t) &Java_org_cacaojvm_compiler2_test_CacaoTest_compileMethod },
};


/* _Jv_org_cacaojvm_compiler2_test_CacaoTest_init *****************************************

   Register native functions.

*******************************************************************************/

void _Jv_org_cacaojvm_compiler2_test_CacaoTest_init(void) {
	Utf8String u = Utf8String::from_utf8("org/cacaojvm/compiler2/test/CacaoTest");

	NativeMethods& nm = VM::get_current()->get_nativemethods();
	nm.register_methods(u, methods, NATIVE_METHODS_COUNT);
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
