/* src/native/native.h - table of native functions

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

   $Id: native.h 8132 2007-06-22 11:15:47Z twisti $

*/


#ifndef _NATIVE_H
#define _NATIVE_H

#include "config.h"

#if !defined(WITH_STATIC_CLASSPATH)
# include <ltdl.h>
#endif

#include <stdint.h>

#include "native/jni.h"

#include "vm/global.h"

#include "vmcore/class.h"
#include "vmcore/method.h"
#include "vmcore/utf8.h"


/* defines ********************************************************************/

#define NATIVE_METHODS_COUNT    sizeof(methods) / sizeof(JNINativeMethod)


/* table for locating native methods */

typedef struct nativeref nativeref;
typedef struct nativecompref nativecompref;


#if !defined(WITH_STATIC_CLASSPATH)
typedef struct hashtable_library_loader_entry hashtable_library_loader_entry;
typedef struct hashtable_library_name_entry hashtable_library_name_entry;


/* native_methods_node_t ******************************************************/

typedef struct native_methods_node_t native_methods_node_t;

struct native_methods_node_t {
	utf         *classname;             /* class name                         */
	utf         *name;                  /* method name                        */
	utf         *descriptor;            /* descriptor name                    */
	functionptr  function;              /* pointer to the implementation      */
};


/* hashtable_library_loader_entry *********************************************/

struct hashtable_library_loader_entry {
	java_objectheader              *loader;  /* class loader                  */
	hashtable_library_name_entry   *namelink;/* libs loaded by this loader    */
	hashtable_library_loader_entry *hashlink;/* link for external chaining    */
};


/* hashtable_library_name_entry ***********************************************/

struct hashtable_library_name_entry {
	utf                          *name;      /* library name                  */
	lt_dlhandle                   handle;    /* libtool library handle        */
	hashtable_library_name_entry *hashlink;  /* link for external chaining    */
};
#endif


struct nativeref {
	char       *classname;
	char       *methodname;
	char       *descriptor;
	bool        isstatic;
	functionptr func;
};

/* table for fast string comparison */

struct nativecompref {
	utf        *classname;
	utf        *methodname;
	utf        *descriptor;
	bool        isstatic;
	functionptr func;
};


/* function prototypes ********************************************************/

bool native_init(void);

void native_method_register(utf *classname, const JNINativeMethod *methods,
							int32_t count);

#if defined(WITH_STATIC_CLASSPATH)

functionptr native_findfunction(utf *cname, utf *mname, utf *desc,
								bool isstatic);

#else /* defined(WITH_STATIC_CLASSPATH) */

lt_dlhandle native_library_open(utf *filename);
void        native_library_add(utf *filename, java_objectheader *loader,
							   lt_dlhandle handle);

hashtable_library_name_entry *native_library_find(utf *filename,
												  java_objectheader *loader);

functionptr native_resolve_function(methodinfo *m);

#endif /* defined(WITH_STATIC_CLASSPATH) */

java_objectheader *native_new_and_init(classinfo *c);

java_objectheader *native_new_and_init_string(classinfo *c,
											  java_objectheader *s);

#endif /* _NATIVE_H */


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
