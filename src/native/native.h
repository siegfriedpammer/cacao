/* src/native/native.h - table of native functions

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Reinhard Grafl

   Changes: Christian Thalinger

   $Id: native.h 4120 2006-01-10 15:50:13Z twisti $

*/


#ifndef _NATIVE_H
#define _NATIVE_H

#include "config.h"

#if !defined(ENABLE_STATICVM)
# include <ltdl.h>
#endif

#include "vm/class.h"
#include "vm/global.h"
#include "vm/method.h"
#include "vm/utf8.h"
#include "native/jni.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_ClassLoader.h"
#include "native/include/java_lang_Throwable.h"


/* table for locating native methods */

typedef struct nativeref nativeref;
typedef struct nativecompref nativecompref;


#if !defined(ENABLE_STATICVM)
typedef struct hashtable_library_loader_entry hashtable_library_loader_entry;
typedef struct hashtable_library_name_entry hashtable_library_name_entry;


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


/* initialize native subsystem */
bool native_init(void);

/* find native function */
functionptr native_findfunction(utf *cname, utf *mname, 
								utf *desc, bool isstatic);

#if !defined(ENABLE_STATICVM)
/* add a library to the library hash */
void native_hashtable_library_add(utf *filename, java_objectheader *loader,
								  lt_dlhandle handle);

/* find a library entry in the library hash */
hashtable_library_name_entry *native_hashtable_library_find(utf *filename,
															java_objectheader *loader);

/* resolve native function */
functionptr native_resolve_function(methodinfo *m);
#endif /* !defined(ENABLE_STATICVM) */

/* create new object on the heap and call the initializer */
java_objectheader *native_new_and_init(classinfo *c);

/* create new object on the heap and call the initializer 
   mainly used for exceptions with a message */
java_objectheader *native_new_and_init_string(classinfo *c, java_lang_String *s);

/* create new object on the heap and call the initializer 
   mainly used for exceptions with an index */
java_objectheader *native_new_and_init_int(classinfo *c, s4 i);

/* create new object on the heap and call the initializer 
   mainly used for exceptions with cause */
java_objectheader *native_new_and_init_throwable(classinfo *c, java_lang_Throwable *t);

java_objectarray *native_get_parametertypes(methodinfo *m);
java_objectarray *native_get_exceptiontypes(methodinfo *m);
classinfo *native_get_returntype(methodinfo *m);

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
