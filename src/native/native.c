/* src/native/native.c - table of native functions

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

   $Id: native.c 7912 2007-05-18 13:12:09Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <ctype.h>

#if !defined(WITH_STATIC_CLASSPATH)
# include <ltdl.h>
#endif

#include "vm/types.h"

#include "mm/memory.h"

#include "native/jni.h"
#include "native/native.h"

#include "native/vm/nativevm.h"

#include "threads/lock-common.h"

#include "toolbox/avl.h"
#include "toolbox/hashtable.h"
#include "toolbox/logging.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"

#include "vmcore/loader.h"
#include "vmcore/options.h"
#include "vm/resolve.h"

#if defined(ENABLE_JVMTI)
#include "native/jvmti/cacaodbg.h"
#endif


/* include table of native functions ******************************************/

#if defined(WITH_STATIC_CLASSPATH)
# include "native/nativetable.inc"
#endif


/* tables for methods *********************************************************/

#if defined(WITH_STATIC_CLASSPATH)
#define NATIVETABLESIZE  (sizeof(nativetable)/sizeof(struct nativeref))

/* table for fast string comparison */
static nativecompref nativecomptable[NATIVETABLESIZE];

/* string comparsion table initialized */
static bool nativecompdone = false;
#endif


/* global variables ***********************************************************/

static avl_tree_t *tree_native_methods;
static hashtable *hashtable_library;


/* prototypes *****************************************************************/

static s4 native_tree_native_methods_comparator(const void *treenode, const void *node);


/* native_init *****************************************************************

   Initializes the native subsystem.

*******************************************************************************/

bool native_init(void)
{
#if !defined(WITH_STATIC_CLASSPATH)
	/* initialize libltdl */

	if (lt_dlinit())
		vm_abort("native_init: lt_dlinit failed: %s\n", lt_dlerror());

	/* initialize library hashtable, 10 entries should be enough */

	hashtable_library = NEW(hashtable);

	hashtable_create(hashtable_library, 10);
#endif

	/* initialize the native methods table */

	tree_native_methods = avl_create(&native_tree_native_methods_comparator);

	/* register the intern native functions */

	nativevm_init();

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

	treenmn = treenode;
	nmn     = node;

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

#if !defined(WITH_STATIC_CLASSPATH)
static utf *native_make_overloaded_function(utf *name, utf *descriptor)
{
	char *newname;
	s4    namelen;
	char *utf_ptr;
	u2    c;
	s4    i;
	s4    dumpsize;
	utf  *u;

	/* mark memory */

	dumpsize = dump_size();

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

	newname = DMNEW(char, namelen);
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

	dump_release(dumpsize);

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
	s4    dumpsize;
	utf  *u;

	/* mark memory */

	dumpsize = dump_size();

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

	name = DMNEW(char, namelen);

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

	assert(pos <= namelen);

	/* make a utf-string */

	u = utf_new_char(name);

	/* release memory */

	dump_release(dumpsize);

	return u;
}


/* native_method_register ******************************************************

   Register a native method in the native method table.

*******************************************************************************/

void native_method_register(utf *classname, JNINativeMethod *methods, s4 count)
{
	native_methods_node_t *nmn;
	utf                   *name;
	utf                   *descriptor;
	s4                     i;

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

	tmpnmn.classname  = m->class->name;
	tmpnmn.name       = m->name;
	tmpnmn.descriptor = m->descriptor;

	/* find the entry in the AVL-tree */

	nmn = avl_find(tree_native_methods, &tmpnmn);

	if (nmn == NULL)
		return NULL;

	return nmn->function;
}


/* native_library_open *********************************************************

   Open a native library with the given utf8 name.

*******************************************************************************/

#if !defined(WITH_STATIC_CLASSPATH)
lt_dlhandle native_library_open(utf *filename)
{
	lt_dlhandle handle;

	/* try to open the library */

	handle = lt_dlopen(filename->text);

	if (handle == NULL) {
		if (opt_verbose) {
			log_start();
			log_print("native_library_load: lt_dlopen failed: ");
			log_print(lt_dlerror());
			log_finish();
		}

		return NULL;
	}

	return handle;
}
#endif


/* native_library_add **********************************************************

   Adds an entry to the native library hashtable.

*******************************************************************************/

#if !defined(WITH_STATIC_CLASSPATH)
void native_library_add(utf *filename, java_objectheader *loader,
						lt_dlhandle handle)
{
	hashtable_library_loader_entry *le;
	hashtable_library_name_entry   *ne; /* library name                       */
	u4   key;                           /* hashkey                            */
	u4   slot;                          /* slot in hashtable                  */

	LOCK_MONITOR_ENTER(hashtable_library->header);

	/* normally addresses are aligned to 4, 8 or 16 bytes */

	key  = ((u4) (ptrint) loader) >> 4;        /* align to 16-byte boundaries */
	slot = key & (hashtable_library->size - 1);
	le   = hashtable_library->ptr[slot];

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
			LOCK_MONITOR_EXIT(hashtable_library->header);

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

	LOCK_MONITOR_EXIT(hashtable_library->header);
}
#endif /* !defined(WITH_STATIC_CLASSPATH) */


/* native_library_find *********************************************************

   Find an entry in the native library hashtable.

*******************************************************************************/

#if !defined(WITH_STATIC_CLASSPATH)
hashtable_library_name_entry *native_library_find(utf *filename,
												  java_objectheader *loader)
{
	hashtable_library_loader_entry *le;
	hashtable_library_name_entry   *ne; /* library name                       */
	u4   key;                           /* hashkey                            */
	u4   slot;                          /* slot in hashtable                  */

	/* normally addresses are aligned to 4, 8 or 16 bytes */

	key  = ((u4) (ptrint) loader) >> 4;        /* align to 16-byte boundaries */
	slot = key & (hashtable_library->size - 1);
	le   = hashtable_library->ptr[slot];

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
#endif /* !defined(WITH_STATIC_CLASSPATH) */


/* native_findfunction *********************************************************

   Looks up a method (must have the same class name, method name,
   descriptor and 'static'ness) and returns a function pointer to it.
   Returns: function pointer or NULL (if there is no such method)

   Remark: For faster operation, the names/descriptors are converted
   from C strings to Unicode the first time this function is called.

*******************************************************************************/

#if defined(WITH_STATIC_CLASSPATH)
functionptr native_findfunction(utf *cname, utf *mname, utf *desc,
								bool isstatic)
{
	/* entry of table for fast string comparison */
	struct nativecompref *n;
	s4 i;

	isstatic = isstatic ? true : false;
	
	if (!nativecompdone) {
		for (i = 0; i < NATIVETABLESIZE; i++) {
			nativecomptable[i].classname  = 
				utf_new_char(nativetable[i].classname);

			nativecomptable[i].methodname = 
				utf_new_char(nativetable[i].methodname);

			nativecomptable[i].descriptor =
				utf_new_char(nativetable[i].descriptor);

			nativecomptable[i].isstatic   = nativetable[i].isstatic;
			nativecomptable[i].func       = nativetable[i].func;
		}

		nativecompdone = true;
	}

	for (i = 0; i < NATIVETABLESIZE; i++) {
		n = &(nativecomptable[i]);

		if (cname == n->classname && mname == n->methodname &&
		    desc == n->descriptor && isstatic == n->isstatic)
			return n->func;
	}

	/* no function was found, throw exception */

	*exceptionptr =
			new_exception_utfmessage(string_java_lang_UnsatisfiedLinkError,
									 mname);

	return NULL;
}
#endif /* defined(WITH_STATIC_CLASSPATH) */


/* native_resolve_function *****************************************************

   Resolves a native function, maybe from a dynamic library.

*******************************************************************************/

functionptr native_resolve_function(methodinfo *m)
{
	utf                            *name;
	utf                            *newname;
	functionptr                     f;
	hashtable_library_loader_entry *le;
	hashtable_library_name_entry   *ne;
	u4                              key;    /* hashkey                        */
	u4                              slot;   /* slot in hashtable              */

	/* verbose output */

	if (opt_verbosejni) {
		printf("[Dynamic-linking native method ");
		utf_display_printable_ascii_classname(m->class->name);
		printf(".");
		utf_display_printable_ascii(m->name);
		printf(" ... ");
	}

	/* generate method symbol string */

	name = native_method_symbol(m->class->name, m->name);

	/* generate overloaded function (having the types in it's name)           */

	newname = native_make_overloaded_function(name, m->descriptor);

	/* check the library hash entries of the classloader of the
	   methods's class  */

	f = NULL;

	/* normally addresses are aligned to 4, 8 or 16 bytes */

	key  = ((u4) (ptrint) m->class->classloader) >> 4;    /* align to 16-byte */
	slot = key & (hashtable_library->size - 1);
	le   = hashtable_library->ptr[slot];

	/* iterate through loaders in this hash slot */

	while ((le != NULL) && (f == NULL)) {
		/* iterate through names in this loader */

		ne = le->namelink;
			
		while ((ne != NULL) && (f == NULL)) {
			f = (functionptr) (ptrint) lt_dlsym(ne->handle, name->text);

			if (f == NULL)
				f = (functionptr) (ptrint) lt_dlsym(ne->handle, newname->text);

			ne = ne->hashlink;
		}

		le = le->hashlink;
	}

	if (f != NULL)
		if (opt_verbosejni)
			printf("JNI ]\n");

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
#endif /* !defined(WITH_STATIC_CLASSPATH) */


/* native_new_and_init *********************************************************

   Creates a new object on the heap and calls the initializer.
   Returns the object pointer or NULL if memory is exhausted.
			
*******************************************************************************/

java_objectheader *native_new_and_init(classinfo *c)
{
	methodinfo *m;
	java_objectheader *o;

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


java_objectheader *native_new_and_init_string(classinfo *c, java_objectheader *s)
{
	methodinfo        *m;
	java_objectheader *o;

	if (c == NULL)
		vm_abort("native_new_and_init_string: c == NULL");

	/* create object */

	o = builtin_new(c);

	if (o == NULL)
		return NULL;

	/* find initializer */

	m = class_resolveclassmethod(c,
								 utf_init,
								 utf_java_lang_String__void,
								 NULL,
								 true);

	/* initializer not found */

	if (m == NULL)
		return NULL;

	/* call initializer */

	(void) vm_call_method(m, o, s);

	return o;
}


java_objectheader *native_new_and_init_int(classinfo *c, s4 i)
{
	methodinfo *m;
	java_objectheader *o;

	if (c == NULL)
		vm_abort("native_new_and_init_int: c == NULL");

	/* create object */

	o = builtin_new(c);
	
	if (o == NULL)
		return NULL;

	/* find initializer */

	m = class_resolveclassmethod(c, utf_init, utf_int__void, NULL, true);

	/* initializer not found  */

	if (m == NULL)
		return NULL;

	/* call initializer */

	(void) vm_call_method(m, o, i);

	return o;
}


java_objectheader *native_new_and_init_throwable(classinfo *c, java_objectheader *t)
{
	java_objectheader *o;
	methodinfo        *m;

	if (c == NULL)
		vm_abort("native_new_and_init_throwable: c == NULL");

	/* create object */

	o = builtin_new(c);
	
	if (o == NULL)
		return NULL;

	/* find initializer */

	m = class_findmethod(c, utf_init, utf_java_lang_Throwable__void);
	                      	                      
	/* initializer not found */

	if (m == NULL)
		return NULL;

	/* call initializer */

	(void) vm_call_method(m, o, t);

	return o;
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
