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

   $Id: patcher.c 3463 2005-10-20 10:07:16Z edwin $

*/


#include "config.h"
#include "vm/types.h"

#include "mm/memory.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/field.h"
#include "vm/initialize.h"
#include "vm/options.h"
#include "vm/references.h"
#include "vm/jit/patcher.h"


/* patcher_get_putstatic *******************************************************

   Machine code:

   <patched call position>
   b8 00 00 00 00             mov    $0x00000000,%eax

*******************************************************************************/

bool patcher_get_putstatic(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_field  *uf;
	fieldinfo         *fi;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* check if the field's class is initialized */

	if (!fi->class->initialized) {
		if (!initialize_class(fi->class)) {
			PATCHER_MONITOREXIT;

			return false;
		}
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch the field value's address */

	*((ptrint *) (ra + 1)) = (ptrint) &(fi->value);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_getfield ************************************************************

   Machine code:

   <patched call position>
   8b 88 00 00 00 00          mov    0x00000000(%eax),%ecx

*******************************************************************************/

bool patcher_getfield(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_field  *uf;
	fieldinfo         *fi;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch the field's offset */

	*((u4 *) (ra + 2)) = (u4) (fi->offset);

	/* if the field has type long, we need to patch the second move too */

	if (fi->type == TYPE_LNG)
		*((u4 *) (ra + 6 + 2)) = (u4) (fi->offset + 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_putfield ************************************************************

   Machine code:

   <patched call position>
   8b 88 00 00 00 00          mov    0x00000000(%eax),%ecx

*******************************************************************************/

bool patcher_putfield(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_field  *uf;
	fieldinfo         *fi;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch the field's offset */

	if (fi->type != TYPE_LNG) {
		*((u4 *) (ra + 2)) = (u4) (fi->offset);

	} else {
		/* long code is very special:
		 *
		 * 8b 8c 24 00 00 00 00       mov    0x00000000(%esp),%ecx
		 * 8b 94 24 00 00 00 00       mov    0x00000000(%esp),%edx
		 * 89 8d 00 00 00 00          mov    %ecx,0x00000000(%ebp)
		 * 89 95 00 00 00 00          mov    %edx,0x00000000(%ebp)
		 */

		*((u4 *) (ra + 7 + 7 + 2)) = (u4) (fi->offset);
		*((u4 *) (ra + 7 + 7 + 6 + 2)) = (u4) (fi->offset + 4);
	}

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_putfieldconst *******************************************************

   Machine code:

   <patched call position>
   c7 85 00 00 00 00 7b 00 00 00    movl   $0x7b,0x0(%ebp)

*******************************************************************************/

bool patcher_putfieldconst(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_field  *uf;
	fieldinfo         *fi;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch the field's offset */

	if (!IS_2_WORD_TYPE(fi->type)) {
		*((u4 *) (ra + 2)) = (u4) (fi->offset);

	} else {
		/* long/double code is different:
		 *
		 * c7 80 00 00 00 00 c8 01 00 00    movl   $0x1c8,0x0(%eax)
		 * c7 80 04 00 00 00 00 00 00 00    movl   $0x0,0x4(%eax)
		 */

		*((u4 *) (ra + 2)) = (u4) (fi->offset);
		*((u4 *) (ra + 10 + 2)) = (u4) (fi->offset + 4);
	}

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_new *********************************************************

   Machine code:

   c7 04 24 00 00 00 00       movl   $0x0000000,(%esp)
   <patched call postition>
   b8 00 00 00 00             mov    $0x0000000,%eax
   ff d0                      call   *%eax

*******************************************************************************/

bool patcher_builtin_new(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - (7 + 5);
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = resolve_classref_eager_nonabstract(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 7 + 0)) = (u4) mcode;
	*((u1 *) (ra + 7 + 4)) = (u1) (mcode >> 32);

	/* patch the classinfo pointer */

	*((ptrint *) (ra + 3)) = (ptrint) c;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch new function address */

	*((ptrint *) (ra + 7 + 1)) = (ptrint) BUILTIN_new;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_newarray ****************************************************

   Machine code:

   c7 44 24 08 00 00 00 00    movl   $0x00000000,0x8(%esp)
   <patched call position>
   b8 00 00 00 00             mov    $0x00000000,%eax
   ff d0                      call   *%eax

*******************************************************************************/

bool patcher_builtin_newarray(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - (8 + 5);
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 8 + 0)) = (u4) mcode;
	*((u1 *) (ra + 8 + 4)) = (u1) (mcode >> 32);

	/* patch the class' vftbl pointer */

	*((ptrint *) (ra + 4)) = (ptrint) c->vftbl;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch new function address */

	*((ptrint *) (ra + 8 + 1)) = (ptrint) BUILTIN_newarray;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_multianewarray **********************************************

   Machine code:

   <patched call position>
   c7 04 24 02 00 00 00       movl   $0x2,(%esp)
   c7 44 24 04 00 00 00 00    movl   $0x00000000,0x4(%esp)
   89 e0                      mov    %esp,%eax
   83 c0 0c                   add    $0xc,%eax
   89 44 24 08                mov    %eax,0x8(%esp)
   b8 00 00 00 00             mov    $0x00000000,%eax
   ff d0                      call   *%eax

*******************************************************************************/

bool patcher_builtin_multianewarray(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch the class' vftbl pointer */

	*((ptrint *) (ra + 7 + 4)) = (ptrint) c->vftbl;

	/* patch new function address */

	*((ptrint *) (ra + 7 + 8 + 2 + 3 + 4 + 1)) = (ptrint) BUILTIN_multianewarray;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_arraycheckcast **********************************************

   Machine code:

   <patched call position>
   c7 44 24 04 00 00 00 00    movl   $0x00000000,0x4(%esp)
   ba 00 00 00 00             mov    $0x00000000,%edx
   ff d2                      call   *%edx

*******************************************************************************/

bool patcher_builtin_arraycheckcast(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch the class' vftbl pointer */

	*((ptrint *) (ra + 4)) = (ptrint) c->vftbl;

	/* patch new function address */

	*((ptrint *) (ra + 8 + 1)) = (ptrint) BUILTIN_arraycheckcast;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_arrayinstanceof *********************************************

   Machine code:

   c7 44 24 08 00 00 00 00    movl   $0x00000000,0x8(%esp)
   <patched call position>
   b8 00 00 00 00             mov    $0x00000000,%eax
   ff d0                      call   *%eax

*******************************************************************************/

bool patcher_builtin_arrayinstanceof(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - (8 + 5);
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 8 + 0)) = (u4) mcode;
	*((u1 *) (ra + 8 + 4)) = (u1) (mcode >> 32);

	/* patch the class' vftbl pointer */

	*((ptrint *) (ra + 4)) = (ptrint) c->vftbl;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch new function address */

	*((ptrint *) (ra + 8 + 1)) = (ptrint) BUILTIN_arrayinstanceof;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokestatic_special ************************************************

   Machine code:

   <patched call position>
   b9 00 00 00 00             mov    $0x00000000,%ecx
   ff d1                      call   *%ecx

*******************************************************************************/

bool patcher_invokestatic_special(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	um    = (unresolved_method *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch stubroutine */

	*((ptrint *) (ra + 1)) = (ptrint) m->stubroutine;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokevirtual *******************************************************

   Machine code:

   <patched call position>
   8b 08                      mov    (%eax),%ecx
   8b 81 00 00 00 00          mov    0x00000000(%ecx),%eax
   ff d0                      call   *%eax

*******************************************************************************/

bool patcher_invokevirtual(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	um    = (unresolved_method *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch vftbl index */

	*((s4 *) (ra + 2 + 2)) = (s4) (OFFSET(vftbl_t, table[0]) +
								   sizeof(methodptr) * m->vftblindex);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokeinterface *****************************************************

   Machine code:

   <patched call position>
   8b 00                      mov    (%eax),%eax
   8b 88 00 00 00 00          mov    0x00000000(%eax),%ecx
   8b 81 00 00 00 00          mov    0x00000000(%ecx),%eax
   ff d0                      call   *%eax

*******************************************************************************/

bool patcher_invokeinterface(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	um    = (unresolved_method *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch interfacetable index */

	*((s4 *) (ra + 2 + 2)) = (s4) (OFFSET(vftbl_t, interfacetable[0]) -
								   sizeof(methodptr) * m->class->index);

	/* patch method offset */

	*((s4 *) (ra + 2 + 6 + 2)) =
		(s4) (sizeof(methodptr) * (m - m->class->methods));

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_instanceof_flags ******************************************

   Machine code:

   <patched call position>
   b9 00 00 00 00             mov    $0x00000000,%ecx

*******************************************************************************/

bool patcher_checkcast_instanceof_flags(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch class flags */

	*((s4 *) (ra + 1)) = (s4) c->flags;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_instanceof_interface **************************************

   Machine code:

   <patched call position>
   8b 91 00 00 00 00          mov    0x00000000(%ecx),%edx
   81 ea 00 00 00 00          sub    $0x00000000,%edx
   85 d2                      test   %edx,%edx
   0f 8e 00 00 00 00          jle    0x00000000
   8b 91 00 00 00 00          mov    0x00000000(%ecx),%edx

*******************************************************************************/

bool patcher_checkcast_instanceof_interface(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch super class index */

	*((s4 *) (ra + 6 + 2)) = (s4) c->index;

	*((s4 *) (ra + 6 + 6 + 2 + 6 + 2)) =
		(s4) (OFFSET(vftbl_t, interfacetable[0]) -
			  c->index * sizeof(methodptr*));

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_class *****************************************************

   Machine code:

   <patched call position>
   ba 00 00 00 00             mov    $0x00000000,%edx
   8b 89 00 00 00 00          mov    0x00000000(%ecx),%ecx
   8b 92 00 00 00 00          mov    0x00000000(%edx),%edx
   29 d1                      sub    %edx,%ecx
   ba 00 00 00 00             mov    $0x00000000,%edx

*******************************************************************************/

bool patcher_checkcast_class(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch super class' vftbl */

	*((ptrint *) (ra + 1)) = (ptrint) c->vftbl;
	*((ptrint *) (ra + 5 + 6 + 6 + 2 + 1)) = (ptrint) c->vftbl;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_instanceof_class ****************************************************

   Machine code:

   <patched call position>
   b9 00 00 00 00             mov    $0x0,%ecx
   8b 40 14                   mov    0x14(%eax),%eax
   8b 51 18                   mov    0x18(%ecx),%edx
   8b 49 14                   mov    0x14(%ecx),%ecx

*******************************************************************************/

bool patcher_instanceof_class(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch super class' vftbl */

	*((ptrint *) (ra + 1)) = (ptrint) c->vftbl;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_clinit **************************************************************

   Is used int PUT/GETSTATIC and native stub.

   Machine code:

   <patched call position>

*******************************************************************************/

bool patcher_clinit(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	c     = (classinfo *)         *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* check if the class is initialized */

	if (!c->initialized) {
		if (!initialize_class(c)) {
			PATCHER_MONITOREXIT;

			return false;
		}
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_athrow_areturn ******************************************************

   Machine code:

   <patched call position>

*******************************************************************************/

bool patcher_athrow_areturn(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_class  *uc;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	uc    = (unresolved_class *)  *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* resolve the class */

	if (!resolve_class(uc, resolveEager, false, &c)) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_resolve_native ******************************************************

   Is used in native stub.

   Machine code:

   <patched call position>
   c7 44 24 04 28 90 01 40    movl   $0x40019028,0x4(%esp)

*******************************************************************************/

#if !defined(ENABLE_STATICVM)
bool patcher_resolve_native(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	methodinfo        *m;
	functionptr        f;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 4));
	mcode =                       *((u8 *)     (sp + 1 * 4));
	m     = (methodinfo *)        *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* resolve native function */

	if (!(f = native_resolve_function(m))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch native function pointer */

	*((ptrint *) (ra + 4)) = (ptrint) f;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}
#endif /* !defined(ENABLE_STATICVM) */


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
