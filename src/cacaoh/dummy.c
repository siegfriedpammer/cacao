/* src/cacaoh/dummy.c - dummy functions for cacaoh

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: dummy.c 8368 2007-08-20 21:36:53Z michi $

*/


#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "toolbox/logging.h"

#include "vm/global.h"
#include "vm/primitive.h"
#include "vm/vm.h"

#include "vmcore/class.h"
#include "vmcore/method.h"
#include "vmcore/utf8.h"
#include "vmcore/classcache.h"
#include "vmcore/loader.h"


/* global variables ***********************************************************/

char *_Jv_bootclasspath;


java_handle_t *javastring_new_slash_to_dot(utf *u)
{
	vm_abort("javastring_new_slash_to_dot");

	return NULL;
}


/* access *********************************************************************/

bool access_is_accessible_class(classinfo *referer, classinfo *cls)
{
	return true;
}

bool access_is_accessible_member(classinfo *referer, classinfo *declarer,
								 int32_t memberflags)
{
	vm_abort("access_is_accessible_member");

	return true;
}


/* asm ************************************************************************/

void asm_abstractmethoderror(void)
{
	abort();
}

void intrp_asm_abstractmethoderror(void)
{
	abort();
}

void asm_getclassvalues_atomic(vftbl_t *super, vftbl_t *sub, castinfo *out)
{
	abort();
}

void intrp_asm_getclassvalues_atomic(vftbl_t *super, vftbl_t *sub, castinfo *out)
{
	abort();
}


/* builtin ********************************************************************/

java_handle_t *builtin_clone(void *env, java_handle_t *o)
{
	abort();

	return NULL;
}

int32_t builtin_isanysubclass(classinfo *sub, classinfo *super)
{
	abort();

	return 0;
}

java_handle_t *builtin_new(classinfo *c)
{
	abort();

	return NULL;
}

java_handle_objectarray_t *builtin_anewarray(int32_t size, classinfo *componentclass)
{
	abort();

	return NULL;
}

java_handle_bytearray_t *builtin_newarray_byte(int32_t size)
{
	abort();

	return NULL;
}


/* code ***********************************************************************/

void code_free_code_of_method(methodinfo *m)
{
}


methodinfo *code_get_methodinfo_for_pv(u1 *pv)
{
	return NULL;
}


/* codegen ********************************************************************/

u1 *codegen_generate_stub_compiler(methodinfo *m)
{
	return NULL;
}

codeinfo *codegen_generate_stub_native(methodinfo *m, functionptr f)
{
	return NULL;
}

#if defined(ENABLE_INTRP)
u1 *intrp_createcompilerstub(methodinfo *m)
{
	return NULL;
}
#endif

void removecompilerstub(u1 *stub)
{
}

void removenativestub(u1 *stub)
{
}


/* exceptions *****************************************************************/

void exceptions_clear_exception(void)
{
}

void exceptions_print_current_exception(void)
{
	abort();
}

void exceptions_throw_abstractmethoderror(void)
{
	fprintf(stderr, "java.lang.AbstractMethodError\n");

	abort();
}

void exceptions_throw_classcircularityerror(classinfo *c)
{
	fprintf(stderr, "java.lang.ClassCircularityError: ");

	utf_display_printable_ascii(c->name);
	fputc('\n', stderr);

	abort();
}

void exceptions_throw_classformaterror(classinfo *c, const char *message, ...)
{
	va_list ap;

	fprintf(stderr, "java.lang.ClassFormatError: ");

	utf_display_printable_ascii(c->name);
	fprintf(stderr, ": ");

	va_start(ap, message);
	vfprintf(stderr, message, ap);
	va_end(ap);

	fputc('\n', stderr);

	abort();
}

void exceptions_throw_incompatibleclasschangeerror(classinfo *c)
{
	fprintf(stderr, "java.lang.IncompatibleClassChangeError: ");

	if (c != NULL)
		utf_fprint_printable_ascii_classname(stderr, c->name);

	fputc('\n', stderr);

	abort();
}

void exceptions_throw_internalerror(const char *message, ...)
{
	va_list ap;

	fprintf(stderr, "java.lang.InternalError: ");

	va_start(ap, message);
	vfprintf(stderr, message, ap);
	va_end(ap);

	abort();
}

void exceptions_throw_linkageerror(const char *message, classinfo *c)
{
	fprintf(stderr, "java.lang.LinkageError: %s", message);

	if (c != NULL)
		utf_fprint_printable_ascii_classname(stderr, c->name);

	fputc('\n', stderr);

	abort();
}

void exceptions_throw_noclassdeffounderror(utf *name)
{
	fprintf(stderr, "java.lang.NoClassDefFoundError: ");
	utf_fprint_printable_ascii(stderr, name);
	fputc('\n', stderr);

	abort();
}

void exceptions_throw_noclassdeffounderror_wrong_name(classinfo *c, utf *name)
{
	fprintf(stderr, "java.lang.NoClassDefFoundError: ");
	utf_fprint_printable_ascii(stderr, c->name);
	fprintf(stderr, " (wrong name: ");
	utf_fprint_printable_ascii(stderr, name);
	fprintf(stderr, ")\n");

	abort();
}

void exceptions_throw_verifyerror(methodinfo *m, const char *message)
{
	fprintf(stderr, "java.lang.VerifyError: ");
	utf_fprint_printable_ascii(stderr, m->name);
	fprintf(stderr, ": %s", message);

	abort();
}

void exceptions_throw_nosuchfielderror(classinfo *c, utf *name)
{
	fprintf(stderr, "java.lang.NoSuchFieldError: ");
	utf_fprint_printable_ascii(stderr, c->name);
	fprintf(stderr, ".");
	utf_fprint_printable_ascii(stderr, name);
	fputc('\n', stderr);

	abort();
}

void exceptions_throw_nosuchmethoderror(classinfo *c, utf *name, utf *desc)
{
	fprintf(stderr, "java.lang.NoSuchMethodError: ");
	utf_fprint_printable_ascii(stderr, c->name);
	fprintf(stderr, ".");
	utf_fprint_printable_ascii(stderr, name);
	utf_fprint_printable_ascii(stderr, desc);
	fputc('\n', stderr);

	abort();
}

void exceptions_throw_unsupportedclassversionerror(classinfo *c,
												   const char *message, ...)
{
	va_list ap;

	fprintf(stderr, "java.lang.UnsupportedClassVersionError: " );

	utf_display_printable_ascii(c->name);
	fprintf(stderr, ": ");

	va_start(ap, message);
	vfprintf(stderr, message, ap);
	va_end(ap);

	fputc('\n', stderr);

	abort();
}

void exceptions_throw_classnotfoundexception(utf *name)
{
	fprintf(stderr, "java.lang.ClassNotFoundException: ");
	utf_fprint_printable_ascii(stderr, name);
	fputc('\n', stderr);

	abort();
}

void exceptions_throw_nullpointerexception(void)
{
	fprintf(stderr, "java.lang.NullPointerException\n");

	abort();
}


/* finalizer ******************************************************************/

void finalizer_notify(void)
{
	vm_abort("finalizer_notify");
}

void finalizer_run(void *o, void *p)
{
	vm_abort("finalizer_run");
}


/* gc *************************************************************************/

void gc_reference_register(java_object_t **ref)
{
	vm_abort("gc_reference_register");
}

int64_t gc_get_heap_size(void)
{
	return 0;
}

int64_t gc_get_free_bytes(void)
{
	return 0;
}

int64_t gc_get_total_bytes(void)
{
	return 0;
}

int64_t gc_get_max_heap_size(void)
{
	return 0;
}


/* heap ***********************************************************************/

void *heap_alloc_uncollectable(uint32_t bytelength)
{
	return calloc(bytelength, 1);
}

s4 heap_get_hashcode(java_object_t *o)
{
	return 0;
}


/* jit ************************************************************************/

void jit_invalidate_code(methodinfo *m)
{
	vm_abort("jit_invalidate_code");
}


/* llni ***********************************************************************/

void llni_critical_start()
{
	vm_abort("llni_critical_start");
}

void llni_critical_end()
{
	vm_abort("llni_critical_end");
}


/* lock ***********************************************************************/

void lock_init_object_lock(java_object_t *o)
{
}

bool lock_monitor_enter(java_object_t *o)
{
	return true;
}

bool lock_monitor_exit(java_object_t *o)
{
	return true;
}


/* md *************************************************************************/

void md_param_alloc(methoddesc *md)
{
}

void md_param_alloc_native(methoddesc *md)
{
}


/* memory *********************************************************************/

void *mem_alloc(int32_t size)
{
	/* real implementation in src/mm/memory.c clears memory */

	return calloc(size, 1);
}

void *mem_realloc(void *src, int32_t len1, int32_t len2)
{
	return realloc(src, len2);
}

void mem_free(void *m, int32_t size)
{
	free(m);
}

void *dump_alloc(int32_t size)
{
	return malloc(size);
}

void dump_release(int32_t size)
{
}

int32_t dump_size(void)
{
	return 0;
}


/* primitive ******************************************************************/

classinfo *primitive_class_get_by_type(int type)
{
	abort();
	return NULL;
}

classinfo *primitive_class_get_by_char(char ch)
{
	abort();
	return NULL;
}


/* properties *****************************************************************/

void properties_add(char *key, char *value)
{
}

char *properties_get(char *key)
{
	return NULL;
}


/* resolve ********************************************************************/

bool resolve_class_from_typedesc(typedesc *d, bool checkaccess, bool link, classinfo **result)
{
	abort();

	return false;
}

/* stupid resolving implementation used by resolve_classref_or_classinfo_eager */
/* This function does eager resolving without any access checks.               */

static classinfo * dummy_resolve_class_from_name(classinfo *referer,
                                                 utf *classname,
                                                 bool checkaccess)
{
	classinfo *cls = NULL;
	char *utf_ptr;
	int len;
	
	assert(referer);
	assert(classname);
	
	/* lookup if this class has already been loaded */

	cls = classcache_lookup(referer->classloader, classname);

	if (!cls) {
		/* resolve array types */

		if (classname->text[0] == '[') {
			utf_ptr = classname->text + 1;
			len = classname->blength - 1;

			/* classname is an array type name */

			switch (*utf_ptr) {
				case 'L':
					utf_ptr++;
					len -= 2;
					/* FALLTHROUGH */
				case '[':
					/* the component type is a reference type */
					/* resolve the component type */
					if ((cls = dummy_resolve_class_from_name(referer,
									   utf_new(utf_ptr,len),
									   checkaccess)) == NULL)
						return NULL; /* exception */

					/* create the array class */
					cls = class_array_of(cls,false);
					if (!cls)
						return NULL; /* exception */
			}
		}

		/* load the class */
		if (!cls) {
			if (!(cls = load_class_from_classloader(classname,
													referer->classloader)))
				return false; /* exception */
		}
	}

	/* the class is now loaded */
	assert(cls);
	assert(cls->state & CLASS_LOADED);

	return cls;
}


classinfo * resolve_classref_or_classinfo_eager(classref_or_classinfo cls,
												bool checkaccess)
{
	classinfo         *c;
	
	assert(cls.any);

	if (IS_CLASSREF(cls)) {
		/* we must resolve this reference */

		if ((c = dummy_resolve_class_from_name(cls.ref->referer, cls.ref->name,
									           checkaccess)) == NULL)
			return NULL;
	}
	else {
		/* cls has already been resolved */
		c = cls.cls;
	}

	assert(c);
	assert(c->state & CLASS_LOADED);

	/* succeeded */
	return c;
}


/* stacktrace *****************************************************************/

java_handle_objectarray_t *stacktrace_getClassContext()
{
	return NULL;
}


/* threads ********************************************************************/

intptr_t threads_get_current_tid(void)
{
	return 0;
}

void threads_stopworld(void)
{
}

void threads_startworld(void)
{
}


/* vm *************************************************************************/

void vm_printconfig(void)
{
}

void vm_abort(const char *text, ...)
{
	va_list ap;

	/* print the log message */

	va_start(ap, text);
	vfprintf(stderr, text, ap);
	va_end(ap);

	/* now abort the VM */

	abort();
}

java_handle_t *vm_call_method(methodinfo *m, java_handle_t *o, ...)
{
	return NULL;
}


/* XXX */

void stringtable_update(void)
{
	log_println("stringtable_update: REMOVE ME!");
}

java_object_t *literalstring_new(utf *u)
{
	log_println("literalstring_new: REMOVE ME!");

	return NULL;
}


void print_dynamic_super_statistics(void)
{
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
 * vim:noexpandtab:sw=4:ts=4:
 */
