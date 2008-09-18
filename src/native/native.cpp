/* src/native/native.cpp - native library support

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
#include <ctype.h>

#include <stdint.h>

#include <algorithm>
#include <functional>
#include <map>

#include "mm/memory.h"

#include "native/jni.hpp"
#include "native/native.hpp"

#include "threads/mutex.hpp"

#include "toolbox/logging.h"

#include "vm/jit/builtin.hpp"
#include "vm/exceptions.hpp"
#include "vm/global.h"
#include "vm/globals.hpp"
#include "vm/loader.hpp"
#include "vm/options.h"
#include "vm/os.hpp"
#include "vm/resolve.h"
#include "vm/string.hpp"
#include "vm/vm.hpp"

#include "vm/jit/asmpart.h"
#include "vm/jit/jit.hpp"

#if defined(ENABLE_JVMTI)
#include "native/jvmti/cacaodbg.h"
#endif


/* native_make_overloaded_function *********************************************

   XXX

*******************************************************************************/

static utf *native_make_overloaded_function(utf *name, utf *descriptor)
{
	char *newname;
	s4    namelen;
	char *utf_ptr;
	u2    c;
	s4    i;
	utf  *u;

	utf_ptr = descriptor->text;
	namelen = strlen(name->text) + strlen("__") + strlen("0");

	/* calculate additional length */

	while ((c = utf_nextu2(&utf_ptr)) != ')') {
		switch (c) {
		case 'Z':
		case 'B':
		case 'C':
		case 'S':
		case 'I':
		case 'J':
		case 'F':
		case 'D':
			namelen++;
			break;
		case '[':
			namelen += 2;
			break;
		case 'L':
			namelen++;
			while (utf_nextu2(&utf_ptr) != ';')
				namelen++;
			namelen += 2;
			break;
		case '(':
			break;
		default:
			assert(0);
		}
	}

	/* reallocate memory */

	i = strlen(name->text);

	newname = MNEW(char, namelen);
	MCOPY(newname, name->text, char, i);

	utf_ptr = descriptor->text;

	newname[i++] = '_';
	newname[i++] = '_';

	while ((c = utf_nextu2(&utf_ptr)) != ')') {
		switch (c) {
		case 'Z':
		case 'B':
		case 'C':
		case 'S':
		case 'J':
		case 'I':
		case 'F':
		case 'D':
			newname[i++] = c;
			break;
		case '[':
			newname[i++] = '_';
			newname[i++] = '3';
			break;
		case 'L':
			newname[i++] = 'L';
			while ((c = utf_nextu2(&utf_ptr)) != ';')
				if (((c >= 'a') && (c <= 'z')) ||
					((c >= 'A') && (c <= 'Z')) ||
					((c >= '0') && (c <= '9')))
					newname[i++] = c;
				else
					newname[i++] = '_';
			newname[i++] = '_';
			newname[i++] = '2';
			break;
		case '(':
			break;
		default:
			assert(0);
		}
	}

	/* close string */

	newname[i] = '\0';

	/* make a utf-string */

	u = utf_new_char(newname);

	/* release memory */

	MFREE(newname, char, namelen);

	return u;
}


/* native_insert_char **********************************************************

   Inserts the passed UTF character into the native method name.  If
   necessary it is escaped properly.

*******************************************************************************/

static s4 native_insert_char(char *name, u4 pos, u2 c)
{
	s4 val;
	s4 i;

	switch (c) {
	case '/':
	case '.':
		/* replace '/' or '.' with '_' */
		name[pos] = '_';
		break;

	case '_':
		/* escape sequence for '_' is '_1' */
		name[pos]   = '_';
		name[++pos] = '1';
		break;

	case ';':
		/* escape sequence for ';' is '_2' */
		name[pos]   = '_';
		name[++pos] = '2';
		break;

	case '[':
		/* escape sequence for '[' is '_1' */
		name[pos]   = '_';
		name[++pos] = '3';
		break;

	default:
		if (isalnum(c))
			name[pos] = c;
		else {
			/* unicode character */
			name[pos]   = '_';
			name[++pos] = '0';

			for (i = 0; i < 4; ++i) {
				val = c & 0x0f;
				name[pos + 4 - i] = (val > 10) ? ('a' + val - 10) : ('0' + val);
				c >>= 4;
			}

			pos += 4;
		}
		break;
	}

	/* return the new buffer index */

	return pos;
}


/* native_method_symbol ********************************************************

   Generate a method-symbol string out of the class name and the
   method name.

*******************************************************************************/

static utf *native_method_symbol(utf *classname, utf *methodname)
{
	char *name;
	s4    namelen;
	char *utf_ptr;
	char *utf_endptr;
	u2    c;
	u4    pos;
	utf  *u;

	/* Calculate length of native function name.  We multiply the
	   class and method name length by 6 as this is the maxium
	   escape-sequence that can be generated (unicode). */

	namelen =
		strlen("Java_") +
		utf_get_number_of_u2s(classname) * 6 +
		strlen("_") +
		utf_get_number_of_u2s(methodname) * 6 +
		strlen("0");

	/* allocate memory */

	name = MNEW(char, namelen);

	/* generate name of native functions */

	strcpy(name, "Java_");
	pos = strlen("Java_");

	utf_ptr    = classname->text;
	utf_endptr = UTF_END(classname);

	for (; utf_ptr < utf_endptr; utf_ptr++, pos++) {
		c   = *utf_ptr;
		pos = native_insert_char(name, pos, c);
	}

	/* seperator between class and method */

	name[pos++] = '_';

	utf_ptr    = methodname->text;
	utf_endptr = UTF_END(methodname);

	for (; utf_ptr < utf_endptr; utf_ptr++, pos++) {
		c   = *utf_ptr;
		pos = native_insert_char(name, pos, c);
	}

	/* close string */

	name[pos] = '\0';

	/* check for an buffer overflow */

	assert((int32_t) pos <= namelen);

	/* make a utf-string */

	u = utf_new_char(name);

	/* release memory */

	MFREE(name, char, namelen);

	return u;
}


bool operator< (const NativeMethod& first, const NativeMethod& second)
{
	if (first._classname < second._classname)
		return true;
	else if (first._classname > second._classname)
		return false;
		
	if (first._name < second._name)
		return true;
	else if (first._name > second._name)
		return false;

	if (first._descriptor < second._descriptor)
		return true;
	else if (first._descriptor > second._descriptor)
		return false;

	// All pointers are equal, we have found the entry.
	return false;
}


/**
 * Register native methods with the VM.  This is done by inserting
 * them into the native method table.
 *
 * @param classname
 * @param methods   Native methods array.
 * @param count     Number of methods in the array.
 */
void NativeMethods::register_methods(utf* classname, const JNINativeMethod* methods, size_t count)
{
	// Insert all methods passed */
	for (size_t i = 0; i < count; i++) {
		if (opt_verbosejni) {
			printf("[Registering JNI native method ");
			utf_display_printable_ascii_classname(classname);
			printf(".%s]\n", methods[i].name);
		}

		// Generate the UTF8 names.
		utf* name      = utf_new_char(methods[i].name);
		utf* signature = utf_new_char(methods[i].signature);

		NativeMethod nm(classname, name, signature, methods[i].fnPtr);

		// Insert the method into the table.
		_methods.insert(nm);
	}
}


/**
 * Resolves a native method, maybe from a dynamic library.
 *
 * @param m Method structure of the native Java method to resolve.
 *
 * @return Pointer to the resolved method (symbol).
 */
void* NativeMethods::resolve_method(methodinfo* m)
{
	// Verbose output.
	if (opt_verbosejni) {
		printf("[Dynamic-linking native method ");
		utf_display_printable_ascii_classname(m->clazz->name);
		printf(".");
		utf_display_printable_ascii(m->name);
		printf(" ... ");
	}

	/* generate method symbol string */

	utf* name = native_method_symbol(m->clazz->name, m->name);

	/* generate overloaded function (having the types in it's name)           */

	utf* newname = native_make_overloaded_function(name, m->descriptor);

	// Try to find the symbol.
	void* symbol = NULL;

#if defined(ENABLE_DL)
	// Get the classloader.
	classloader_t* classloader = class_get_classloader(m->clazz);

	// Resolve the native method name from the native libraries.
	NativeLibraries& libraries = VM::get_current()->get_nativelibraries();
	symbol = libraries.resolve_symbol(name, classloader);

	if (symbol == NULL)
		symbol = libraries.resolve_symbol(newname, classloader);

# if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
	if (symbol == NULL) {
		/* We can resolve the function directly from
		   java.lang.ClassLoader as it's a static function. */
		/* XXX should be done in native_init */

		methodinfo* method_findNative =
			class_resolveclassmethod(class_java_lang_ClassLoader,
									 utf_findNative,
									 utf_java_lang_ClassLoader_java_lang_String__J,
									 class_java_lang_ClassLoader,
									 true);

		if (method_findNative == NULL)
			return NULL;

		// Try the normal name.
		java_handle_t* s = javastring_new(name);
		symbol = (void*) vm_call_method_long(method_findNative, NULL, classloader, s);

		// If not found, try the mangled name.
		if (symbol == NULL) {
			s = javastring_new(newname);
			symbol = (void*) vm_call_method_long(method_findNative, NULL, classloader, s);
		}
	}
# endif

	if (symbol != NULL)
		if (opt_verbosejni)
			printf("JNI ]\n");
#endif

	// If not found already, try to find the native method symbol in
	// the native methods registered with the VM.
	if (symbol == NULL) {
		symbol = find_registered_method(m);

		if (symbol != NULL)
			if (opt_verbosejni)
				printf("internal ]\n");
	}

#if defined(ENABLE_JVMTI)
	/* fire Native Method Bind event */
	if (jvmti) jvmti_NativeMethodBind(m, f, &f);
#endif

	// Symbol not found?  Throw an exception.
	if (symbol == NULL) {
		if (opt_verbosejni)
			printf("failed ]\n");

		exceptions_throw_unsatisfiedlinkerror(m->name);
	}

	return symbol;
}


/**
 * Try to find the given method in the native methods registered with
 * the VM.
 *
 * @param m Method structure.
 *
 * @return Pointer to function if found, NULL otherwise.
 */
void* NativeMethods::find_registered_method(methodinfo* m)
{
	NativeMethod nm(m);
	std::set<NativeMethod>::iterator it = _methods.find(nm);

	if (it == _methods.end())
		return NULL;

	return (*it).get_function();
}


/**
 * Open this native library.
 *
 * @return File handle on success, NULL otherwise.
 */
#if defined(ENABLE_DL)
void* NativeLibrary::open()
{
	if (opt_verbosejni) {
		printf("[Loading native library ");
		utf_display_printable_ascii(_filename);
		printf(" ... ");
	}

	// Sanity check.
	assert(_filename != NULL);

	// Try to open the library.
	_handle = os::dlopen(_filename->text, RTLD_LAZY);

	if (_handle == NULL) {
		if (opt_verbosejni)
			printf("failed ]\n");

		if (opt_verbose) {
			log_start();
			log_print("NativeLibrary::open: os::dlopen failed: ");
			log_print(os::dlerror());
			log_finish();
		}

		return NULL;
	}

	if (opt_verbosejni)
		printf("OK ]\n");

	return _handle;
}
#endif


/**
 * Close this native library.
 */
#if defined(ENABLE_DL)
void NativeLibrary::close()
{
	if (opt_verbosejni) {
		printf("[Unloading native library ");
/* 		utf_display_printable_ascii(filename); */
		printf(" ... ");
	}

	// Sanity check.
	assert(_handle != NULL);

	// Close the library.
	int result = os::dlclose(_handle);

	if (result != 0) {
		if (opt_verbosejni)
			printf("failed ]\n");

		if (opt_verbose) {
			log_start();
			log_print("NativeLibrary::close: os::dlclose failed: ");
			log_print(os::dlerror());
			log_finish();
		}
	}

	if (opt_verbosejni)
		printf("OK ]\n");
}
#endif


/**
 * Load this native library and initialize it, if possible.
 *
 * @param env JNI environment.
 *
 * @return true if library loaded successfully, false otherwise.
 */
bool NativeLibrary::load(JNIEnv* env)
{
#if defined(ENABLE_DL)
	if (_filename == NULL) {
		exceptions_throw_nullpointerexception();
		return false;
	}

	// Is the library already loaded?
	if (is_loaded())
		return true;

	// Open the library.
	open();

	if (_handle == NULL)
		return false;

# if defined(ENABLE_JNI)
	// Resolve JNI_OnLoad function.
	void* onload = os::dlsym(_handle, "JNI_OnLoad");

	if (onload != NULL) {
		JNIEXPORT jint (JNICALL *JNI_OnLoad) (JavaVM*, void*);
		JavaVM *vm;

		JNI_OnLoad = (JNIEXPORT jint (JNICALL *)(JavaVM*, void*)) (uintptr_t) onload;

		env->GetJavaVM(&vm);

		jint version = JNI_OnLoad(vm, NULL);

		// If the version is not 1.2 and not 1.4 the library cannot be
		// loaded.
		if ((version != JNI_VERSION_1_2) && (version != JNI_VERSION_1_4)) {
			os::dlclose(_handle);
			return false;
		}
	}
# endif

	// Insert the library name into the native library table.
	NativeLibraries& nativelibraries = VM::get_current()->get_nativelibraries();
	nativelibraries.add(*this);

	return true;
#else
	os::abort("NativeLibrary::load: Not available in this configuration.");

	// Keep the compiler happy.
	return false;
#endif
}


/**
 * Checks if this native library is loaded.
 *
 * @return true if loaded, false otherwise.
 */
#if defined(ENABLE_DL)
bool NativeLibrary::is_loaded()
{
	NativeLibraries& libraries = VM::get_current()->get_nativelibraries();
	return libraries.is_loaded(*this);
}
#endif


/**
 * Resolve the given symbol in this native library.
 *
 * @param symbolname Symbol name.
 *
 * @return Pointer to symbol if found, NULL otherwise.
 */
void* NativeLibrary::resolve_symbol(utf* symbolname) const
{
	return os::dlsym(_handle, symbolname->text);
}


/**
 * Add the given native library to the native libraries table.
 *
 * @param library Native library to insert.
 */
#if defined(ENABLE_DL)
void NativeLibraries::add(NativeLibrary& library)
{
	// Make the container thread-safe.
	_mutex.lock();

	// XXX Check for double entries.
	// Insert the native library.
	_libraries.insert(std::make_pair(library.get_classloader(), library));

	_mutex.unlock();
}
#endif


/**
 * Checks if the given native library is loaded.
 *
 * @param library Native library.
 *
 * @return true if loaded, false otherwise.
 */
bool NativeLibraries::is_loaded(NativeLibrary& library)
{
	std::pair<MAP::const_iterator, MAP::const_iterator> its = _libraries.equal_range(library.get_classloader());

	// No entry for the classloader was found (the range has length
	// zero).
	if (its.first == its.second)
		return false;
	
	MAP::const_iterator it = find_if(its.first, its.second, std::bind2nd(comparator(), library.get_filename()));

	// No matching entry in the range found.
	if (it == its.second)
		return false;

	return true;
}


/**
 * Try to find a symbol with the given name in all loaded native
 * libraries defined by classloader.
 *
 * @param symbolname  Name of the symbol to find.
 * @param classloader Defining classloader.
 *
 * @return Pointer to symbol if found, NULL otherwise.
 */
void* NativeLibraries::resolve_symbol(utf* symbolname, classloader_t* classloader)
{
	std::pair<MAP::const_iterator, MAP::const_iterator> its = _libraries.equal_range(classloader);

	// No entry for the classloader was found (the range has length
	// zero).
	if (its.first == its.second)
		return NULL;

	for (MAP::const_iterator it = its.first; it != its.second; it++) {
		const NativeLibrary& library = (*it).second;
		void* symbol = library.resolve_symbol(symbolname);

		if (symbol != NULL)
			return symbol;
	}

	return NULL;
}


/* native_new_and_init *********************************************************

   Creates a new object on the heap and calls the initializer.
   Returns the object pointer or NULL if memory is exhausted.
			
*******************************************************************************/

java_handle_t *native_new_and_init(classinfo *c)
{
	methodinfo    *m;
	java_handle_t *o;

	if (c == NULL)
		vm_abort("native_new_and_init: c == NULL");

	/* create object */

	o = builtin_new(c);
	
	if (o == NULL)
		return NULL;

	/* try to find the initializer */

	m = class_findmethod(c, utf_init, utf_void__void);
	                      	                      
	/* ATTENTION: returning the object here is ok, since the class may
       not have an initializer */

	if (m == NULL)
		return o;

	/* call initializer */

	(void) vm_call_method(m, o);

	return o;
}


java_handle_t *native_new_and_init_string(classinfo *c, java_handle_t *s)
{
	methodinfo    *m;
	java_handle_t *o;

	if (c == NULL)
		vm_abort("native_new_and_init_string: c == NULL");

	/* create object */

	o = builtin_new(c);

	if (o == NULL)
		return NULL;

	/* find initializer */

	m = class_findmethod(c, utf_init, utf_java_lang_String__void);

	/* initializer not found */

	if (m == NULL)
		return NULL;

	/* call initializer */

	(void) vm_call_method(m, o, s);

	return o;
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
