/* src/vm/method.c - method functions

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
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

   Contact: cacao@cacaojvm.org

   Authors: Reinhard Grafl

   Changes: Andreas Krall
            Roman Obermaiser
            Mark Probst
            Edwin Steiner
            Christian Thalinger

   $Id: method.c 5785 2006-10-15 22:25:54Z edwin $

*/


#include "config.h"

#include <assert.h>
#include <stdio.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "vm/class.h"
#include "vm/global.h"
#include "vm/linker.h"
#include "vm/loader.h"
#include "vm/method.h"
#include "vm/options.h"
#include "vm/jit/methodheader.h"


/* method_free *****************************************************************

   Frees all memory that was allocated for this method.

*******************************************************************************/

void method_free(methodinfo *m)
{
	if (m->jcode)
		MFREE(m->jcode, u1, m->jcodelength);

	if (m->rawexceptiontable)
		MFREE(m->rawexceptiontable, raw_exception_entry, m->rawexceptiontablelength);

	code_free_code_of_method(m);

	if (m->stubroutine) {
		if (m->flags & ACC_NATIVE) {
			removenativestub(m->stubroutine);

		} else {
			removecompilerstub(m->stubroutine);
		}
	}
}


/* method_canoverwrite *********************************************************

   Check if m and old are identical with respect to type and
   name. This means that old can be overwritten with m.
	
*******************************************************************************/

bool method_canoverwrite(methodinfo *m, methodinfo *old)
{
	if (m->name != old->name)
		return false;

	if (m->descriptor != old->descriptor)
		return false;

	if (m->flags & ACC_STATIC)
		return false;

	return true;
}


/* method_vftbl_lookup *********************************************************

   Does a method lookup in the passed virtual function table.  This
   function does exactly the same thing as JIT, but additionally
   relies on the fact, that the methodinfo pointer is at the first
   data segment slot (even for compiler stubs).

*******************************************************************************/

methodinfo *method_vftbl_lookup(vftbl_t *vftbl, methodinfo* m)
{
	methodptr   mptr;
	methodptr  *pmptr;
	methodinfo *resm;                   /* pointer to new resolved method     */
	codeinfo   *code;

	/* If the method is not an instance method, just return it. */

	if (m->flags & ACC_STATIC)
		return m;

	assert(vftbl);

	/* Get the method from the virtual function table.  Is this an
	   interface method? */

	if (m->class->flags & ACC_INTERFACE) {
		pmptr = vftbl->interfacetable[-(m->class->index)];
		mptr  = pmptr[(m - m->class->methods)];
	}
	else {
		mptr = vftbl->table[m->vftblindex];
	}

	/* and now get the codeinfo pointer from the first data segment slot */

	code = *((codeinfo **) (mptr + CodeinfoPointer));

	resm = code->m;

	return resm;
}


/* method_printflags ***********************************************************

   Prints the flags of a method to stdout like.

*******************************************************************************/

#if !defined(NDEBUG)
void method_printflags(methodinfo *m)
{
	if (m == NULL) {
		printf("NULL");
		return;
	}

	if (m->flags & ACC_PUBLIC)       printf(" PUBLIC");
	if (m->flags & ACC_PRIVATE)      printf(" PRIVATE");
	if (m->flags & ACC_PROTECTED)    printf(" PROTECTED");
   	if (m->flags & ACC_STATIC)       printf(" STATIC");
   	if (m->flags & ACC_FINAL)        printf(" FINAL");
   	if (m->flags & ACC_SYNCHRONIZED) printf(" SYNCHRONIZED");
   	if (m->flags & ACC_VOLATILE)     printf(" VOLATILE");
   	if (m->flags & ACC_TRANSIENT)    printf(" TRANSIENT");
   	if (m->flags & ACC_NATIVE)       printf(" NATIVE");
   	if (m->flags & ACC_INTERFACE)    printf(" INTERFACE");
   	if (m->flags & ACC_ABSTRACT)     printf(" ABSTRACT");
}
#endif /* !defined(NDEBUG) */


/* method_print ****************************************************************

   Prints a method to stdout like:

   java.lang.Object.<init>()V

*******************************************************************************/

#if !defined(NDEBUG)
void method_print(methodinfo *m)
{
	if (m == NULL) {
		printf("NULL");
		return;
	}

	utf_display_printable_ascii_classname(m->class->name);
	printf(".");
	utf_display_printable_ascii(m->name);
	utf_display_printable_ascii(m->descriptor);

	method_printflags(m);
}
#endif /* !defined(NDEBUG) */


/* method_println **************************************************************

   Prints a method plus new line to stdout like:

   java.lang.Object.<init>()V

*******************************************************************************/

#if !defined(NDEBUG)
void method_println(methodinfo *m)
{
	if (opt_debugcolor) printf("\033[31m");	/* red */
	method_print(m);
	if (opt_debugcolor) printf("\033[m");	
	printf("\n");
}
#endif /* !defined(NDEBUG) */


/* method_methodref_print ******************************************************

   Prints a method reference to stdout.

*******************************************************************************/

#if !defined(NDEBUG)
void method_methodref_print(constant_FMIref *mr)
{
	if (!mr) {
		printf("(constant_FMIref *)NULL");
		return;
	}

	if (IS_FMIREF_RESOLVED(mr)) {
		printf("<method> ");
		method_print(mr->p.method);
	}
	else {
		printf("<methodref> ");
		utf_display_printable_ascii_classname(mr->p.classref->name);
		printf(".");
		utf_display_printable_ascii(mr->name);
		utf_display_printable_ascii(mr->descriptor);
	}
}
#endif /* !defined(NDEBUG) */


/* method_methodref_println ****************************************************

   Prints a method reference to stdout, followed by a newline.

*******************************************************************************/

#if !defined(NDEBUG)
void method_methodref_println(constant_FMIref *mr)
{
	method_methodref_print(mr);
	printf("\n");
}
#endif /* !defined(NDEBUG) */


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
