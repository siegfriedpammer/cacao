/* src/vm/jit/i386/patcher.c - i386 code patching functions

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

   Authors: Christian Thalinger

   Changes:

   $Id: patcher.c 2312 2005-04-20 16:01:00Z twisti $

*/


#include "vm/jit/i386/types.h"
#include "vm/field.h"
#include "vm/references.h"
#include "vm/jit/asmhelper.c"


/* patcher_get_putstatic *******************************************************

   XXX

   Machine code:

   b8 00 00 00 00             mov    $0x00000000,%eax

*******************************************************************************/

bool patcher_get_putstatic(u1 *sp)
{
	u1               *ra;
	u8                mcode;
	unresolved_field *uf;
	fieldinfo        *fi;

	/* get stuff from the stack */

	ra    = (u1 *)               *((ptrint *) (sp + 3 * 4));
	mcode =                      *((u8 *)     (sp + 1 * 4));
	uf    = (unresolved_field *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra -= 5;
	*(sp + 3 * 4) = (ptrint) ra;

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf)))
		return false;

	/* check if the field's class is initialized */

	if (!fi->class->initialized)
		if (!initialize_class(fi->class))
			return false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* patch the field value's address */

	*((ptrint *) (ra + 1)) = (ptrint) &(fi->value);

	return true;
}


/* patcher_get_putfield ********************************************************

   XXX

   Machine code:

   from:
   e8 00 00 00 00             call   0x00000000

   to:
   8b 88 00 00 00 00          mov    0x0(%eax),%ecx

*******************************************************************************/

bool patcher_get_putfield(u1 *sp)
{
	u1               *ra;
	u8                mcode;
	unresolved_field *uf;
	fieldinfo        *fi;

	/* get stuff from the stack */

	ra    = (u1 *)               *((ptrint *) (sp + 3 * 4));
	mcode =                      *((u8 *)     (sp + 1 * 4));
	uf    = (unresolved_field *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra -= 5;
	*(sp + 3 * 4) = (ptrint) ra;

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf)))
		return false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* patch the field's offset */

	*((u4 *) (ra + 2)) = (u4) (fi->offset);

	/* if the field has type long, we need to patch the second move too */

	if (fi->type == TYPE_LNG)
		*((u4 *) (ra + 6 + 2)) = (u4) (fi->offset + 4);

	return true;
}


/* patcher_builtin_new *********************************************************

   XXX

   Machine code:

   c7 04 24 00 00 00 00       movl   $0x0000000,(%esp)
   b8 00 00 00 00             mov    $0x0000000,%eax
   ff d0                      call   *%eax

*******************************************************************************/

bool patcher_builtin_new(u1 *sp)
{
	constant_classref *cr;
	u1                *ra;
	classinfo         *c;

	/* get stuff from the stack */

	cr = (constant_classref *) *((ptrint *) (sp + 1 * 4));
	ra = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra -= (7 + 5 + 2);
	*(sp + 0 * 4) = (ptrint) ra;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr)))
		return false;

	/* patch the classinfo pointer */

	*((ptrint *) (ra + 3)) = (ptrint) c;

	/* patch new function address */

	*((ptrint *) (ra + 7 + 1)) = (ptrint) BUILTIN_new;

	return true;
}


/* patcher_clinit **************************************************************

   XXX

*******************************************************************************/

bool patcher_clinit(u1 *sp)
{
	u1        *ra;
	u8         mcode;
	classinfo *c;

	/* get stuff from the stack */

	ra    = (u1 *)        *((ptrint *) (sp + 3 * 4));
	mcode =               *((u8 *)     (sp + 1 * 4));
	c     = (classinfo *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra -= 5;
	*(sp + 3 * 4) = (ptrint) ra;

	/* check if the class is initialized */

	if (!c->initialized)
		if (!initialize_class(c))
			return false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	return true;
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
