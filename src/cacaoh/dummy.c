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

   $Id: headers.c 6286 2007-01-10 10:03:38Z twisti $

*/


#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "toolbox/logging.h"

#include "vm/types.h"

#include "vm/global.h"
#include "vm/vm.h"

#include "vm/jit/code.h"

#include "vmcore/class.h"
#include "vmcore/method.h"
#include "vmcore/utf8.h"


/* global variables ***********************************************************/

char *_Jv_bootclasspath;


void compiler_lock()
{
}

void compiler_unlock()
{
}

java_objectheader *javastring_new_slash_to_dot(utf *u)
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
								 s4 memberflags)
{
	vm_abort("access_is_accessible_member");

	return true;
}


/* asm ************************************************************************/

void asm_abstractmethoderror(void)
{
	abort();
}


/* builtin ********************************************************************/

java_objectheader *builtin_clone(void *env, java_objectheader *o)
{
	abort();
}

s4 builtin_isanysubclass(classinfo *sub, classinfo *super)
{
	abort();
}

java_objectheader *builtin_new(classinfo *c)
{
	abort();
}


/* code ***********************************************************************/

void code_free_code_of_method(methodinfo *m)
{
}


/* codegen ********************************************************************/

codeinfo *codegen_createnativestub(functionptr f, methodinfo *m)
{
	return NULL;
}

u1 *createcompilerstub(methodinfo *m)
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

void exceptions_throw_outofmemoryerror(void)
{
	fprintf(stderr, "java.lang.OutOfMemoryError\n");

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

void exceptions_throw_illegalaccessexception(classinfo *c)
{
	fprintf(stderr, "java.lang.IllegalAccessException: ");

	if (c != NULL)
		utf_fprint_printable_ascii_classname(stderr, c->name);

	fputc('\n', stderr);

	abort();
}

void exceptions_throw_nullpointerexception(void)
{
	fprintf(stderr, "java.lang.NullPointerException\n");

	abort();
}

void classnotfoundexception_to_noclassdeffounderror(void)
{
	/* Can that one happen? */

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


/* heap ***********************************************************************/

void *heap_alloc_uncollectable(u4 bytelength)
{
	return calloc(bytelength, 1);
}


/* jit ************************************************************************/

void jit_invalidate_code(methodinfo *m)
{
	vm_abort("jit_invalidate_code");
}


/* lock ***********************************************************************/

void lock_init_object_lock(java_objectheader *o)
{
}

bool lock_monitor_enter(java_objectheader *o)
{
	return true;
}

bool lock_monitor_exit(java_objectheader *o)
{
	return true;
}


/* md *************************************************************************/

void md_param_alloc(methoddesc *md)
{
}


/* memory *********************************************************************/

void *mem_alloc(s4 size)
{
	return malloc(size);
}

void *mem_realloc(void *src, s4 len1, s4 len2)
{
	return realloc(src, len2);
}

void mem_free(void *m, s4 size)
{
	free(m);
}

void *dump_alloc(s4 size)
{
	return malloc(size);
}

void dump_release(s4 size)
{
}

s4 dump_size(void)
{
	return 0;
}


/* properties *****************************************************************/

char *properties_get(char *key)
{
	return NULL;
}


/* stacktrace *****************************************************************/

java_objectarray *stacktrace_getClassContext()
{
	return NULL;
}


/* threads ********************************************************************/

pthread_key_t threads_current_threadobject_key;


/* typeinfo *******************************************************************/

bool typeinfo_init_class(typeinfo *info, classref_or_classinfo c)
{
	return true;
}

void typeinfo_init_classinfo(typeinfo *info, classinfo *c)
{
}

typecheck_result typeinfo_is_assignable_to_class(typeinfo *value, classref_or_classinfo dest)
{
	return typecheck_TRUE;
}


/* vm *************************************************************************/

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

java_objectheader *vm_call_method(methodinfo *m, java_objectheader *o, ...)
{
	return NULL;
}


/* XXX */

void stringtable_update(void)
{
	log_println("stringtable_update: REMOVE ME!");
}

java_objectheader *literalstring_new(utf *u)
{
	log_println("literalstring_new: REMOVE ME!");

	return NULL;
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
