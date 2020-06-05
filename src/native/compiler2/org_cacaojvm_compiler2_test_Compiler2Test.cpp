/* src/native/compiler2/org_cacaojvm_compiler2_test_Compiler2Test.cpp

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


#include <iostream>
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

// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     org/cacaojvm/compiler2/test/Compiler2Test
 * Method:    compileMethod
 * Signature: (ZLjava/lang/Class;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_cacaojvm_compiler2_test_Compiler2Test_compileMethod(JNIEnv *env, jclass clazz, jboolean baseline, jclass compile_class, jstring name, jstring desc) {
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

	// throw exception if method is not found
	if (m == NULL) {
		exceptions_clear_exception();
		exceptions_throw_nosuchmethoderror(ci, u, d);
		exceptions_print_stacktrace();
		os::abort();
	}

	m->code = NULL;

	if (baseline) {
		// baseline compiler
		if (!jit_compile(m)) {
			os::abort();
		}
	}
	else {
		// compiler2 compiler
		if (!cacao::jit::compiler2::compile(m)) {
			os::abort();
		}
	}

	return;
}

/*
 * Class:     org/cacaojvm/compiler2/test/Compiler2Test
 * Method:    executeMethod
 * Signature: (ZLjava/lang/Class;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_cacaojvm_compiler2_test_Compiler2Test_executeMethod(JNIEnv *env, jclass clazz, jclass compile_class, jstring name, jstring desc, jobjectArray args) {
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

	// throw exception if method is not found
	if (m == NULL) {
		exceptions_clear_exception();
		exceptions_throw_nosuchmethoderror(ci, u, d);
		exceptions_print_stacktrace();
		os::abort();
	}

	java_handle_t *result = vm_call_method_objectarray(m, NULL, args);

	// restore code
	// TODO free code!
	m->code = NULL;

	return (jobject) result;
}

/*
 * Class:     org/cacaojvm/compiler2/test/Compiler2Test
 * Method:    getStatistics
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_cacaojvm_compiler2_test_Compiler2Test_getStatistics(JNIEnv *env, jclass clazz, jstring statistics_entry) {
	// without enabled statistics we throw an exception
	#if !defined(ENABLE_STATISTICS)
	exceptions_throw_unsatisfiedlinkerror(Utf8String::from_utf8("to use the getStatistics(...) method you have to set ENABLE_STATISTICS first"));
	return 0;
	#else

	if (statistics_entry == NULL) {
		exceptions_throw_illegalargumentexception();
		os::abort();
	}

	// search for an statistics entry with the specified name
	cacao::StatGroup *root = &cacao::StatGroup::root();
	const char *v = env->GetStringUTFChars(statistics_entry, 0);
	const cacao::StatEntry *entry = root->find(v);

	// no entry with that name found!
	if (!entry) {
		exceptions_throw_illegalargumentexception();
		return 0;
	}

	// return value as long
	unsigned long return_value = entry->get_value();
	return (jlong) return_value;
	#endif
}

} // extern "C"

/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ (char*) "compileMethod", (char*) "(ZLjava/lang/Class;Ljava/lang/String;Ljava/lang/String;)V",(void*) (uintptr_t) &Java_org_cacaojvm_compiler2_test_Compiler2Test_compileMethod },
	{ (char*) "executeMethod", (char*) "(Ljava/lang/Class;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;",(void*) (uintptr_t) &Java_org_cacaojvm_compiler2_test_Compiler2Test_executeMethod },
	{ (char*) "getStatistics", (char*) "(Ljava/lang/String;)J",(void*) (uintptr_t) &Java_org_cacaojvm_compiler2_test_Compiler2Test_getStatistics },
};


/* _Jv_org_cacaojvm_compiler2_test_Compiler2Test_init *****************************************

   Register native functions.

*******************************************************************************/

void _Jv_org_cacaojvm_compiler2_test_Compiler2Test_init(void) {
	Utf8String u = Utf8String::from_utf8("org/cacaojvm/compiler2/test/Compiler2Test");

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
