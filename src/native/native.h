/* src/native/native.h - table of native functions

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


#ifndef _NATIVE_H
#define _NATIVE_H

#include "config.h"

#if defined(ENABLE_LTDL) && defined(HAVE_LTDL_H)
# include <ltdl.h>
#endif

#include <stdint.h>

#include "native/jni.h"

#include "vm/global.h"

#include "vmcore/class.h"
#include "vmcore/loader.h"
#include "vmcore/method.h"
#include "vmcore/utf8.h"


/* defines ********************************************************************/

#define NATIVE_METHODS_COUNT    sizeof(methods) / sizeof(JNINativeMethod)


/* native_methods_node_t ******************************************************/

typedef struct native_methods_node_t native_methods_node_t;

struct native_methods_node_t {
	utf         *classname;             /* class name                         */
	utf         *name;                  /* method name                        */
	utf         *descriptor;            /* descriptor name                    */
	functionptr  function;              /* pointer to the implementation      */
};


/* hashtable_library_loader_entry *********************************************/

#if defined(ENABLE_LTDL)
typedef struct hashtable_library_loader_entry hashtable_library_loader_entry;
typedef struct hashtable_library_name_entry   hashtable_library_name_entry;

struct hashtable_library_loader_entry {
	classloader                    *loader;  /* class loader                  */
	hashtable_library_name_entry   *namelink;/* libs loaded by this loader    */
	hashtable_library_loader_entry *hashlink;/* link for external chaining    */
};
#endif


/* hashtable_library_name_entry ***********************************************/

#if defined(ENABLE_LTDL)
struct hashtable_library_name_entry {
	utf                          *name;      /* library name                  */
	lt_dlhandle                   handle;    /* libtool library handle        */
	hashtable_library_name_entry *hashlink;  /* link for external chaining    */
};
#endif


/* function prototypes ********************************************************/

bool native_init(void);

void        native_method_register(utf *classname, const JNINativeMethod *methods, int32_t count);
functionptr native_method_resolve(methodinfo *m);

#if defined(ENABLE_LTDL)
lt_dlhandle native_library_open(utf *filename);
void        native_library_close(lt_dlhandle handle);
void        native_library_add(utf *filename, classloader *loader, lt_dlhandle handle);
hashtable_library_name_entry *native_library_find(utf *filename, classloader *loader);
int         native_library_load(JNIEnv *env, utf *name, classloader *cl);
#endif

java_handle_t *native_new_and_init(classinfo *c);
java_handle_t *native_new_and_init_string(classinfo *c, java_handle_t *s);

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
 * vim:noexpandtab:sw=4:ts=4:
 */
