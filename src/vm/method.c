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

   $Id: method.c 4357 2006-01-22 23:33:38Z twisti $

*/


#include "config.h"

#include <stdio.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "vm/class.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/method.h"


/* method_free *****************************************************************

   Frees all memory that was allocated for this method.

*******************************************************************************/

void method_free(methodinfo *m)
{
	if (m->jcode)
		MFREE(m->jcode, u1, m->jcodelength);

	if (m->exceptiontable)
		MFREE(m->exceptiontable, exceptiontable, m->exceptiontablelength);

	if (m->mcode)
		CFREE((void *) (ptrint) m->mcode, m->mcodelength);

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

/* method_descriptor2types *****************************************************

   Fills in the following members of the given methodinfo:
       returntype
   	   paramcount
       paramtypes	   

   Note:
       This function uses dump_alloc functions to allocate memory.

*******************************************************************************/		

void method_descriptor2types(methodinfo *m)
{
	u1 *types, *tptr;
	int pcount, i;
	methoddesc *md = m->parseddesc;
	typedesc *paramtype;
	
	pcount = md->paramcount;
	if ((m->flags & ACC_STATIC) == 0)
		pcount++; /* count this pointer */
	
	types = DMNEW(u1,pcount);
	tptr = types;
	
	if (!(m->flags & ACC_STATIC)) {
		/* this pointer */
		*tptr++ = TYPE_ADR;
	}

	paramtype = md->paramtypes;
	for (i=0; i<md->paramcount; ++i,++paramtype)
		*tptr++ = paramtype->type;

	m->returntype = md->returntype.type;
	m->paramcount = pcount;
	m->paramtypes = types;
}


/* method_printflags ***********************************************************

   Prints the flags of a method to stdout like.

*******************************************************************************/

#if !defined(NDEBUG)
void method_printflags(methodinfo *m)
{
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
	utf_display_classname(m->class->name);
	printf(".");
	utf_display(m->name);
	utf_display(m->descriptor);

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
	method_print(m);
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
 */
