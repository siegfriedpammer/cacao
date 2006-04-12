/* src/vm/field.h - field functions header

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

   Changes: Christian Thalinger

   $Id: field.h 4753 2006-04-12 08:52:14Z edwin $
*/


#ifndef _FIELD_H
#define _FIELD_H

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"
#include "vm/utf8.h"
#include "vm/references.h"
#include "vm/descriptor.h"


/* fieldinfo ******************************************************************/

struct fieldinfo {	      /* field of a class                                 */
	s4         flags;     /* ACC flags                                        */
	s4         type;      /* basic data type                                  */
	utf       *name;      /* name of field                                    */
	utf       *descriptor;/* JavaVM descriptor string of field                */
	typedesc  *parseddesc;/* parsed descriptor                                */

	s4         offset;    /* offset from start of object (instance variables) */

	imm_union  value;     /* storage for static values (class variables)      */

	classinfo *class;     /* needed by typechecker. Could be optimized        */
	                      /* away by using constant_FMIref instead of         */
	                      /* fieldinfo throughout the compiler.               */
};


/* function prototypes ********************************************************/

void field_free(fieldinfo *f);

#if !defined(NDEBUG)
void field_printflags(fieldinfo *f);
void field_print(fieldinfo *f);
void field_println(fieldinfo *f);
#endif

#endif /* _FIELD_H */


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
