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

#include "vm/types.h"

#include "mm/memory.h"

#include "native/jni.hpp"
#include "native/native.hpp"

#include "native/vm/nativevm.h"

#include "threads/mutex.hpp"

#include "toolbox/avl.h"
#include "toolbox/hashtable.h"
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


/* global variables ***********************************************************/

static avl_tree_t *tree_native_methods;

#if defined(ENABLE_DL)
static hashtable *hashtable_library;
#endif


/* prototypes *****************************************************************/

static s4 native_tree_native_methods_comparator(const void *treenode, const void *node);


/* native_init *****************************************************************

   Initializes the native subsystem.

*******************************************************************************/

bool native_init(void)
{
	TRACESUBSYSTEMINITIALIZATION("native_init");

#if defined(ENABLE_DL)
	/* initialize library hashtable, 10 entries should be enough */

	hashtable_library = NEW(hashtable);

	hashtable_create(hashtable_library, 10);
#endif

	/* initialize the native methods table */

	tree_native_methods = avl_create(&native_tree_native_methods_comparator);

	/* everything's ok */

	return true;
}


/* native_tree_native_methods_comparator ***************************************

   Comparison function for AVL tree of native methods.

   IN:
       treenode....node in the tree
	   node........node to compare with tree-node

   RETURN VALUE:
       -1, 0, +1

*******************************************************************************/

static s4 native_tree_native_methods_comparator(const void *treenode, const void *node)
{
	const native_methods_node_t *treenmn;
	const native_methods_node_t *nmn;

	treenmn = (native_methods_node_t*) treenode;
	nmn     = (native_methods_node_t*) node;

	/* these are for walking the tree */

	if (treenmn->classname < nmn->classname)
		return -1;
	else if (treenmn->classname > nmn->classname)
		return 1;

	if (treenmn->name < nmn->name)
		return -1;
	else if (treenmn->name > nmn->name)
		return 1;

	if (treenmn->descriptor < nmn->descriptor)
		return -1;
	else if (treenmn->descriptor > nmn->descriptor)
		return 1;

	/* all pointers are equal, we have found the entry */

	return 0;
}


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


/* native_method_register ******************************************************

   Register a native method in the native method table.

*******************************************************************************/

void native_method_register(utf *classname, const JNINativeMethod *methods,
							int32_t count)
{
	native_methods_node_t *nmn;
	utf                   *name;
	utf                   *descriptor;
	int32_t                i;

	/* insert all methods passed */

	for (i = 0; i < count; i++) {
		if (opt_verbosejni) {
			printf("[Registering JNI native method ");
			utf_display_printable_ascii_classname(classname);
			printf(".%s]\n", methods[i].name);
		}

		/* generate the utf8 names */

		name       = utf_new_char(methods[i].name);
		descriptor = utf_new_char(methods[i].signature);

		/* allocate a new tree node */

		nmn = NEW(native_methods_node_t);

		nmn->classname  = classname;
		nmn->name       = name;
		nmn->descriptor = descriptor;
		nmn->function   = (functionptr) (ptrint) methods[i].fnPtr;

		/* insert the method into the tree */

		avl_insert(tree_native_methods, nmn);
	}
}


/* native_method_find **********************************************************

   Find a native method in the native method table.

*******************************************************************************/

static functionptr native_method_find(methodinfo *m)
{
	native_methods_node_t  tmpnmn;
	native_methods_node_t *nmn;

	/* fill the temporary structure used for searching the tree */

	tmpnmn.classname  = m->clazz->name;
	tmpnmn.name       = m->name;
	tmpnmn.descriptor = m->descriptor;

	/* find the entry in the AVL-tree */

	nmn = (native_methods_node_t*) avl_find(tree_native_methods, &tmpnmn);

	if (nmn == NULL)
		return NULL;

	return nmn->function;
}


/* native_method_resolve *******************************************************

   Resolves a native method, maybe from a dynamic library.

   IN:
       m ... methodinfo of the native Java method to resolve

   RESULT:
       pointer to the resolved method (symbol)

*******************************************************************************/

functionptr native_method_resolve(methodinfo *m)
{
	utf                            *name;
	utf                            *newname;
	functionptr                     f;
#if defined(ENABLE_DL)
	classloader_t                  *cl;
	hashtable_library_loader_entry *le;
	hashtable_library_name_entry   *ne;
	u4                              key;    /* hashkey                        */
	u4                              slot;   /* slot in hashtable              */
#endif
#if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
	methodinfo                     *method_findNative;
	java_handle_t                  *s;
#endif

	/* verbose output */

	if (opt_verbosejni) {
		printf("[Dynamic-linking native method ");
		utf_display_printable_ascii_classname(m->clazz->name);
		printf(".");
		utf_display_printable_ascii(m->name);
		printf(" ... ");
	}

	/* generate method symbol string */

	name = native_method_symbol(m->clazz->name, m->name);

	/* generate overloaded function (having the types in it's name)           */

	newname = native_make_overloaded_function(name, m->descriptor);

	/* check the library hash entries of the classloader of the
	   methods's class  */

	f = NULL;

#if defined(ENABLE_DL)
	/* Get the classloader. */

	cl = class_get_classloader(m->clazz);

	/* normally addresses are aligned to 4, 8 or 16 bytes */

	key  = ((u4) (ptrint) cl) >> 4;                       /* align to 16-byte */
	slot = key & (hashtable_library->size - 1);
	le   = (hashtable_library_loader_entry*) hashtable_library->ptr[slot];

	/* iterate through loaders in this hash slot */

	while ((le != NULL) && (f == NULL)) {
		/* iterate through names in this loader */

		ne = le->namelink;
			
		while ((ne != NULL) && (f == NULL)) {
			f = (functionptr) (ptrint) os::dlsym(ne->handle, name->text);

			if (f == NULL)
				f = (functionptr) (ptrint) os::dlsym(ne->handle, newname->text);

			ne = ne->hashlink;
		}

		le = le->hashlink;
	}

# if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
	if (f == NULL) {
		/* We can resolve the function directly from
		   java.lang.ClassLoader as it's a static function. */
		/* XXX should be done in native_init */

		method_findNative =
			class_resolveclassmethod(class_java_lang_ClassLoader,
									 utf_findNative,
									 utf_java_lang_ClassLoader_java_lang_String__J,
									 class_java_lang_ClassLoader,
									 true);

		if (method_findNative == NULL)
			return NULL;

		/* try the normal name */

		s = javastring_new(name);

		f = (functionptr) (intptr_t) vm_call_method_long(method_findNative,
														 NULL, cl, s);

		/* if not found, try the mangled name */

		if (f == NULL) {
			s = javastring_new(newname);

			f = (functionptr) (intptr_t) vm_call_method_long(method_findNative,
															 NULL, cl, s);
		}
	}
# endif

	if (f != NULL)
		if (opt_verbosejni)
			printf("JNI ]\n");
#endif

	/* If not found, try to find the native function symbol in the
	   main program. */

	if (f == NULL) {
		f = native_method_find(m);

		if (f != NULL)
			if (opt_verbosejni)
				printf("internal ]\n");
	}

#if defined(ENABLE_JVMTI)
	/* fire Native Method Bind event */
	if (jvmti) jvmti_NativeMethodBind(m, f, &f);
#endif

	/* no symbol found? throw exception */

	if (f == NULL) {
		if (opt_verbosejni)
			printf("failed ]\n");

		exceptions_throw_unsatisfiedlinkerror(m->name);
	}

	return f;
}


/* native_library_open *********************************************************

   Open a native library with the given utf8 name.

   IN:
       filename ... filename of the library to open

   RETURN:
       handle of the opened library

*******************************************************************************/

#if defined(ENABLE_DL)
void* native_library_open(utf *filename)
{
	void* handle;

	if (opt_verbosejni) {
		printf("[Loading native library ");
		utf_display_printable_ascii(filename);
		printf(" ... ");
	}

	/* try to open the library */

	handle = os::dlopen(filename->text, RTLD_LAZY);

	if (handle == NULL) {
		if (opt_verbosejni)
			printf("failed ]\n");

		if (opt_verbose) {
			log_start();
			log_print("native_library_open: os::dlopen failed: ");
			log_print(dlerror());
			log_finish();
		}

		return NULL;
	}

	if (opt_verbosejni)
		printf("OK ]\n");

	return handle;
}
#endif


/* native_library_close ********************************************************

   Close the native library of the given handle.

   IN:
       handle ... handle of the open library

*******************************************************************************/

#if defined(ENABLE_DL)
void native_library_close(void* handle)
{
	int result;

	if (opt_verbosejni) {
		printf("[Unloading native library ");
/* 		utf_display_printable_ascii(filename); */
		printf(" ... ");
	}

	/* Close the library. */

	result = os::dlclose(handle);

	if (result != 0) {
		if (opt_verbose) {
			log_start();
			log_print("native_library_close: os::dlclose failed: ");
			log_print(dlerror());
			log_finish();
		}
	}
}
#endif


/* native_library_add **********************************************************

   Adds an entry to the native library hashtable.

*******************************************************************************/

#if defined(ENABLE_DL)
void native_library_add(utf *filename, classloader_t *loader, void* handle)
{
	hashtable_library_loader_entry *le;
	hashtable_library_name_entry   *ne; /* library name                       */
	u4   key;                           /* hashkey                            */
	u4   slot;                          /* slot in hashtable                  */

	hashtable_library->mutex->lock();

	/* normally addresses are aligned to 4, 8 or 16 bytes */

	key  = ((u4) (ptrint) loader) >> 4;        /* align to 16-byte boundaries */
	slot = key & (hashtable_library->size - 1);
	le   = (hashtable_library_loader_entry*) hashtable_library->ptr[slot];

	/* search external hash chain for the entry */

	while (le) {
		if (le->loader == loader)
			break;

		le = le->hashlink;                  /* next element in external chain */
	}

	/* no loader found? create a new entry */

	if (le == NULL) {
		le = NEW(hashtable_library_loader_entry);

		le->loader   = loader;
		le->namelink = NULL;

		/* insert entry into hashtable */

		le->hashlink =
			(hashtable_library_loader_entry *) hashtable_library->ptr[slot];
		hashtable_library->ptr[slot] = le;

		/* update number of hashtable-entries */

		hashtable_library->entries++;
	}


	/* search for library name */

	ne = le->namelink;

	while (ne) {
		if (ne->name == filename) {
			hashtable_library->mutex->unlock();
			return;
		}

		ne = ne->hashlink;                  /* next element in external chain */
	}

	/* not found? add the library name to the classloader */

	ne = NEW(hashtable_library_name_entry);

	ne->name   = filename;
	ne->handle = handle;

	/* insert entry into external chain */

	ne->hashlink = le->namelink;
	le->namelink = ne;

	hashtable_library->mutex->unlock();
}
#endif


/* native_library_find *********************************************************

   Find an entry in the native library hashtable.

*******************************************************************************/

#if defined(ENABLE_DL)
hashtable_library_name_entry *native_library_find(utf *filename,
												  classloader_t *loader)
{
	hashtable_library_loader_entry *le;
	hashtable_library_name_entry   *ne; /* library name                       */
	u4   key;                           /* hashkey                            */
	u4   slot;                          /* slot in hashtable                  */

	/* normally addresses are aligned to 4, 8 or 16 bytes */

	key  = ((u4) (ptrint) loader) >> 4;        /* align to 16-byte boundaries */
	slot = key & (hashtable_library->size - 1);
	le   = (hashtable_library_loader_entry*) hashtable_library->ptr[slot];

	/* search external hash chain for the entry */

	while (le) {
		if (le->loader == loader)
			break;

		le = le->hashlink;                  /* next element in external chain */
	}

	/* no loader found? return NULL */

	if (le == NULL)
		return NULL;

	/* search for library name */

	ne = le->namelink;

	while (ne) {
		if (ne->name == filename)
			return ne;

		ne = ne->hashlink;                  /* next element in external chain */
	}

	/* return entry, if no entry was found, ne is NULL */

	return ne;
}
#endif


/* native_library_load *********************************************************

   Load a native library and initialize it, if possible.

   IN:
       name ... name of the library
	   cl ..... classloader which loads this library

   RETURN:
       1 ... library loaded successfully
       0 ... error

*******************************************************************************/

int native_library_load(JNIEnv *env, utf *name, classloader_t *cl)
{
#if defined(ENABLE_DL)
	void*   handle;
# if defined(ENABLE_JNI)
	void*   onload;
	int32_t version;
# endif

	if (name == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}

	/* Is the library already loaded? */

	if (native_library_find(name, cl) != NULL)
		return 1;

	/* Open the library. */

	handle = native_library_open(name);

	if (handle == NULL)
		return 0;

# if defined(ENABLE_JNI)
	/* Resolve JNI_OnLoad function. */

	onload = os::dlsym(handle, "JNI_OnLoad");

	if (onload != NULL) {
		JNIEXPORT int32_t (JNICALL *JNI_OnLoad) (JavaVM *, void *);
		JavaVM *vm;

		JNI_OnLoad = (JNIEXPORT int32_t (JNICALL *)(JavaVM *, void *)) (ptrint) onload;

		env->GetJavaVM(&vm);

		version = JNI_OnLoad(vm, NULL);

		/* If the version is not 1.2 and not 1.4 the library cannot be
		   loaded. */

		if ((version != JNI_VERSION_1_2) && (version != JNI_VERSION_1_4)) {
			os::dlclose(handle);
			return 0;
		}
	}
# endif

	/* Insert the library name into the library hash. */

	native_library_add(name, cl, handle);

	return 1;
#else
	vm_abort("native_library_load: not available");

	/* Keep compiler happy. */

	return 0;
#endif
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
