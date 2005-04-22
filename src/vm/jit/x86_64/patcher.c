/* src/vm/jit/x86_64/patcher.c - x86_64 code patching functions

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

   $Id: patcher.c 2355 2005-04-22 14:57:47Z twisti $

*/


#include "vm/jit/x86_64/types.h"
#include "vm/builtin.h"
#include "vm/field.h"
#include "vm/initialize.h"
#include "vm/references.h"
#include "vm/jit/helper.h"


/* patcher_get_putstatic *******************************************************

   XXX

   Machine code:

   4d 8b 15 86 fe ff ff             mov    -378(%rip),%r10

*******************************************************************************/

bool patcher_get_putstatic(u1 *sp)
{
	u1               *ra;
	u8                mcode;
	unresolved_field *uf;
	fieldinfo        *fi;
	ptrint           *dataaddress;
	s4                ripoffset;

	/* get stuff from the stack */

	ra    = (u1 *)               *((ptrint *) (sp + 2 * 8));
	mcode =                      *((u8 *)     (sp + 1 * 8));
	uf    = (unresolved_field *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 2 * 8)) = (ptrint) ra;

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf)))
		return false;

	/* check if the field's class is initialized */

	if (!fi->class->initialized)
		if (!initialize_class(fi->class))
			return false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* get RIP offset from machine instruction */

	ripoffset = *((u4 *) (ra + 3));

	/* calculate address in data segment (+ 7: is the size of the RIP move) */

	dataaddress = (ptrint *) (ra + ripoffset + 7);

	/* patch the field value's address */

	*dataaddress = (ptrint) &(fi->value);

	return true;
}


/* patcher_get_putfield ********************************************************

   XXX

   Machine code:

   45 8b 8f 00 00 00 00             mov    0x0(%r15),%r9d

*******************************************************************************/

bool patcher_get_putfield(u1 *sp)
{
	u1               *ra;
	u8                mcode;
	unresolved_field *uf;
	fieldinfo        *fi;

	/* get stuff from the stack */

	ra    = (u1 *)               *((ptrint *) (sp + 2 * 8));
	mcode =                      *((u8 *)     (sp + 1 * 8));
	uf    = (unresolved_field *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 2 * 8)) = (ptrint) ra;

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf)))
		return false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* patch the field's offset: we check for the field type, because the     */
	/* instructions have different lengths                                    */

	if (IS_FLT_DBL_TYPE(fi->type)) {
		*((u4 *) (ra + 5)) = (u4) (fi->offset);

	} else {
		u1 byte;

		/* check for special case: %rsp or %r12 as base register */

		byte = *(ra + 3);

		if (byte == 0x24)
			*((u4 *) (ra + 4)) = (u4) (fi->offset);
		else
			*((u4 *) (ra + 3)) = (u4) (fi->offset);
	}

	return true;
}


/* patcher_builtin_new *********************************************************

   XXX

   Machine code:

*******************************************************************************/

bool patcher_builtin_new(constant_classref *cr, u1 *sp)
{
	u1                *ra;
	classinfo         *c;

	/* get stuff from the stack */

	ra = (u1 *)                *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - (10 + 10 + 3);
	*((ptrint *) (sp + 0 * 8)) = (ptrint) ra;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr)))
		return false;

	/* patch the classinfo pointer */

	*((ptrint *) (ra + 2)) = (ptrint) c;

	/* patch new function address */

	*((ptrint *) (ra + 10 + 2)) = (ptrint) BUILTIN_new;

	return true;
}


/* patcher_builtin_newarray ****************************************************

   XXX

   Machine code:

*******************************************************************************/

bool patcher_builtin_newarray(u1 *sp, constant_classref *cr)
{
	u1                *ra;
	classinfo         *c;

	/* get stuff from the stack */

	ra = (u1 *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - (10 + 10 + 3);
	*((ptrint *) (sp + 0 * 8)) = (ptrint) ra;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr)))
		return false;

	/* patch the class' vftbl pointer */

	*((ptrint *) (ra + 2)) = (ptrint) c->vftbl;

	/* patch new function address */

	*((ptrint *) (ra + 10 + 2)) = (ptrint) BUILTIN_newarray;

	return true;
}


/* patcher_builtin_multianewarray **********************************************

   XXX

   Machine code:

*******************************************************************************/

bool patcher_builtin_multianewarray(u1 *sp, constant_classref *cr)
{
	u1                *ra;
	classinfo         *c;

	/* get stuff from the stack */

	ra = (u1 *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - (10 + 10 + 3 + 10 + 3);
	*((ptrint *) (sp + 0 * 8)) = (ptrint) ra;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr)))
		return false;

	/* patch the class' vftbl pointer */

	*((ptrint *) (ra + 10 + 2)) = (ptrint) c->vftbl;

	/* patch new function address */

	*((ptrint *) (ra + 10 + 10 + 3 + 2)) = (ptrint) BUILTIN_multianewarray;

	return true;
}


/* patcher_builtin_checkarraycast **********************************************

   XXX

   Machine code:

*******************************************************************************/

bool patcher_builtin_checkarraycast(u1 *sp, constant_classref *cr)
{
	u1                *ra;
	classinfo         *c;

	/* get stuff from the stack */

	ra = (u1 *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - (10 + 10 + 3);
	*((ptrint *) (sp + 0 * 8)) = (ptrint) ra;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr)))
		return false;

	/* patch the class' vftbl pointer */

	*((ptrint *) (ra + 2)) = (ptrint) c->vftbl;

	/* patch new function address */

	*((ptrint *) (ra + 10 + 2)) = (ptrint) BUILTIN_checkarraycast;

	return true;
}


/* patcher_builtin_arrayinstanceof *********************************************

   XXX

   Machine code:

*******************************************************************************/

bool patcher_builtin_arrayinstanceof(u1 *sp, constant_classref *cr)
{
	u1                *ra;
	classinfo         *c;

	/* get stuff from the stack */

	ra = (u1 *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - (10 + 10 + 3);
	*((ptrint *) (sp + 0 * 8)) = (ptrint) ra;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr)))
		return false;

	/* patch the class' vftbl pointer */

	*((ptrint *) (ra + 2)) = (ptrint) c->vftbl;

	/* patch new function address */

	*((ptrint *) (ra + 10 + 2)) = (ptrint) BUILTIN_arrayinstanceof;

	return true;
}


/* patcher_invokestatic_special ************************************************

   XXX

*******************************************************************************/

bool patcher_invokestatic_special(u1 *sp)
{
	u1                *ra;
	u8                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	um    = (unresolved_method *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 2 * 8)) = (ptrint) ra;

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um)))
		return false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* patch stubroutine */

	*((ptrint *) (ra + 2)) = (ptrint) m->stubroutine;

	return true;
}


/* patcher_invokevirtual *******************************************************

   XXX

*******************************************************************************/

bool patcher_invokevirtual(u1 *sp)
{
	u1                *ra;
	u8                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	um    = (unresolved_method *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 2 * 8)) = (ptrint) ra;

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um)))
		return false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* patch vftbl index */

	*((s4 *) (ra + 3 + 3)) = (s4) (OFFSET(vftbl_t, table[0]) +
								   sizeof(methodptr) * m->vftblindex);

	return true;
}


/* patcher_invokeinterface *****************************************************

   XXX

*******************************************************************************/

bool patcher_invokeinterface(u1 *sp)
{
	u1                *ra;
	u8                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	um    = (unresolved_method *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 2 * 8)) = (ptrint) ra;

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um)))
		return false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* patch interfacetable index */

	*((s4 *) (ra + 3 + 3)) = (s4) (OFFSET(vftbl_t, interfacetable[0]) -
								   sizeof(methodptr) * m->class->index);

	/* patch method offset */

	*((s4 *) (ra + 3 + 7 + 3)) =
		(s4) (sizeof(methodptr) * (m - m->class->methods));

	return true;
}


/* patcher_checkcast_instanceof_flags ******************************************

   XXX

*******************************************************************************/

bool patcher_checkcast_instanceof_flags(u1 *sp)
{
	u1                *ra;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 2 * 8)) = (ptrint) ra;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr)))
		return false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* patch class flags */

	*((s4 *) (ra + 2)) = (s4) c->flags;

	return true;
}


/* patcher_checkcast_instanceof_interface **************************************

   XXX

*******************************************************************************/

bool patcher_checkcast_instanceof_interface(u1 *sp)
{
	u1                *ra;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 2 * 8)) = (ptrint) ra;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr)))
		return false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* patch super class index */

	*((s4 *) (ra + 7 + 3)) = (s4) c->index;

	*((s4 *) (ra + 7 + 7 + 3 + 6 + 3)) =
		(s4) (OFFSET(vftbl_t, interfacetable[0]) -
			  c->index * sizeof(methodptr*));

	return true;
}


/* patcher_checkcast_class *****************************************************

   XXX

*******************************************************************************/

bool patcher_checkcast_class(u1 *sp)
{
	u1                *ra;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 2 * 8)) = (ptrint) ra;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr)))
		return false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* patch super class' vftbl */

	*((ptrint *) (ra + 2)) = (ptrint) c->vftbl;
	*((ptrint *) (ra + 10 + 7 + 7 + 3 + 2)) = (ptrint) c->vftbl;

	return true;
}


/* patcher_instanceof_class ****************************************************

   XXX

*******************************************************************************/

bool patcher_instanceof_class(u1 *sp)
{
	u1                *ra;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 2 * 8)) = (ptrint) ra;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr)))
		return false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* patch super class' vftbl */

	*((ptrint *) (ra + 2)) = (ptrint) c->vftbl;

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

	ra    = (u1 *)        *((ptrint *) (sp + 2 * 8));
	mcode =               *((u8 *)     (sp + 1 * 8));
	c     = (classinfo *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 2 * 8)) = (ptrint) ra;

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
