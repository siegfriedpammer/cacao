/* src/vm/jit/powerpc/patcher.c - PowerPC code patching functions

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

   $Id: patcher.c 3169 2005-09-10 20:32:22Z twisti $

*/


#include "vm/jit/powerpc/types.h"

#include "mm/memory.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/field.h"
#include "vm/initialize.h"
#include "vm/options.h"
#include "vm/references.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/helper.h"
#include "vm/jit/patcher.h"


/* patcher_get_putstatic *******************************************************

   Machine code:

   <patched call position>
   816dffc8    lwz   r11,-56(r13)
   80ab0000    lwz   r5,0(r11)

*******************************************************************************/

bool patcher_get_putstatic(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	unresolved_field  *uf;
	s4                 disp;
	u1                *pv;
	fieldinfo         *fi;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf))) {
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

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	asm_cacheflush(ra, 4);

	/* patch the field value's address */

	*((ptrint *) (pv + disp)) = (ptrint) &(fi->value);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_get_putfield ********************************************************

   Machine code:

   <patched call position>
   811f0014    lwz   r8,20(r31)

*******************************************************************************/

bool patcher_get_putfield(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	unresolved_field  *uf;
	u1                *pv;
	fieldinfo         *fi;

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 2 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 1 * 4));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (opt_showdisassemble)
		ra = ra + 4;

	/* patch the field's offset */

	if (fi->type == TYPE_LNG) {
		/* if the field has type long, we have to patch two instructions */

		*((u4 *) ra) |= (s2) ((fi->offset + 4) & 0x0000ffff);
		*((u4 *) (ra + 4)) |= (s2) (fi->offset & 0x0000ffff);

	} else {
		*((u4 *) ra) |= (s2) (fi->offset & 0x0000ffff);
	}

	/* synchronize instruction cache */

	asm_cacheflush(ra, 8);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_new *********************************************************

   Machine code:

   806dffc4    lwz   r3,-60(r13)
   <patched call postition>
   81adffc0    lwz   r13,-64(r13)
   7da903a6    mtctr r13
   4e800421    bctrl

   NOTICE: Only the displacement for the function address is passed,
   but the address of the classinfo pointer is one below (above, in
   addresses speaking). This is for sure.

*******************************************************************************/

bool patcher_builtin_new(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	s4                 disp;
	u1                *pv;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - (2 * 4);
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 4)) = mcode;

	/* synchronize instruction cache */

	asm_cacheflush(ra + 4, 4);

	/* patch the classinfo pointer */

	*((ptrint *) (pv + (disp + SIZEOF_VOID_P))) = (ptrint) c;

	/* patch new function address */

	*((ptrint *) (pv + disp)) = (ptrint) BUILTIN_new;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_newarray ****************************************************

   Machine code:

   808dffc8    lwz   r4,-56(r13)
   <patched call position>
   81adffc4    lwz   r13,-60(r13)
   7da903a6    mtctr r13
   4e800421    bctrl

   NOTICE: Only the displacement for the function address is passed,
   but the address of the vftbl pointer is one below (above, in
   addresses speaking). This is for sure.

*******************************************************************************/

bool patcher_builtin_newarray(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	u1                *pv;
	s4                 disp;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 2 * 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 4)) = mcode;

	/* synchronize instruction cache */

	asm_cacheflush(ra + 4, 4);

	/* patch the class' vftbl pointer */

	*((ptrint *) (pv + (disp + SIZEOF_VOID_P))) = (ptrint) c->vftbl;

	/* patch new function address */

	*((ptrint *) (pv + disp)) = (ptrint) BUILTIN_newarray;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_multianewarray **********************************************

   Machine code:

   <patched call position>
   808dffc0    lwz   r4,-64(r13)
   38a10038    addi  r5,r1,56
   81adffbc    lwz   r13,-68(r13)
   7da903a6    mtctr r13
   4e800421    bctrl

*******************************************************************************/

bool patcher_builtin_multianewarray(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	s4                 disp;
	u1                *pv;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	asm_cacheflush(ra, 4);

	/* patch the class' vftbl pointer */

	*((ptrint *) (pv + disp)) = (ptrint) c->vftbl;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_arraycheckcast **********************************************

   Machine code:

   <patched call position>
   808dffd8    lwz   r4,-40(r13)
   81adffd4    lwz   r13,-44(r13)
   7da903a6    mtctr r13
   4e800421    bctrl

   NOTICE: Only the displacement of the vftbl pointer address is
   passed, but the address of the function pointer is one above
   (below, in addresses speaking). This is for sure.

*******************************************************************************/

bool patcher_builtin_arraycheckcast(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	s4                 disp;
	u1                *pv;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 1 * 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	asm_cacheflush(ra, 4);

	/* patch the class' vftbl pointer */

	*((ptrint *) (pv + disp)) = (ptrint) c->vftbl;

	/* patch new function address */

	*((ptrint *) (pv + (disp - SIZEOF_VOID_P))) =
		(ptrint) BUILTIN_arraycheckcast;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_arrayinstanceof *********************************************

   Machine code:

   808dff50    lwz   r4,-176(r13)
   <patched call position>
   81adff4c    lwz   r13,-180(r13)
   7da903a6    mtctr r13
   4e800421    bctrl

   NOTICE: Only the displacement for the function address is passed,
   but the address of the vftbl pointer is one below (above, in
   addresses speaking). This is for sure.

*******************************************************************************/

bool patcher_builtin_arrayinstanceof(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	s4                 disp;
	u1                *pv;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 2 * 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 4)) = mcode;

	/* synchronize instruction cache */

	asm_cacheflush(ra + 4, 4);

	/* patch the class' vftbl pointer */
	
	*((ptrint *) (pv + (disp + SIZEOF_VOID_P))) = (ptrint) c->vftbl;

	/* patch new function address */

	*((ptrint *) (pv + disp)) = (ptrint) BUILTIN_arrayinstanceof;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokestatic_special ************************************************

   Machine code:

   <patched call position>
   81adffd8    lwz   r13,-40(r13)
   7da903a6    mtctr r13
   4e800421    bctrl

******************************************************************************/

bool patcher_invokestatic_special(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	unresolved_method *um;
	s4                 disp;
	u1                *pv;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	um    = (unresolved_method *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	asm_cacheflush(ra, 4);

	/* patch stubroutine */

	*((ptrint *) (pv + disp)) = (ptrint) m->stubroutine;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokevirtual *******************************************************

   Machine code:

   <patched call position>
   81830000    lwz   r12,0(r3)
   81ac0088    lwz   r13,136(r12)
   7da903a6    mtctr r13
   4e800421    bctrl

*******************************************************************************/

bool patcher_invokevirtual(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	um    = (unresolved_method *) *((ptrint *) (sp + 2 * 4));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (opt_showdisassemble)
		ra = ra + 4;

	/* patch vftbl index */

	*((s4 *) (ra + 4)) |= (s4) ((OFFSET(vftbl_t, table[0]) +
								 sizeof(methodptr) * m->vftblindex) & 0x0000ffff);

	/* synchronize instruction cache */

	asm_cacheflush(ra, 2 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokeinterface *****************************************************

   Machine code:

   <patched call position>
   81830000    lwz   r12,0(r3)
   818cffd0    lwz   r12,-48(r12)
   81ac000c    lwz   r13,12(r12)
   7da903a6    mtctr r13
   4e800421    bctrl

*******************************************************************************/

bool patcher_invokeinterface(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	um    = (unresolved_method *) *((ptrint *) (sp + 2 * 4));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (opt_showdisassemble)
		ra = ra + 4;

	/* patch interfacetable index */

	*((s4 *) (ra + 1 * 4)) |= (s4) ((OFFSET(vftbl_t, interfacetable[0]) -
								 sizeof(methodptr*) * m->class->index) & 0x0000ffff);

	/* patch method offset */

	*((s4 *) (ra + 2 * 4)) |=
		(s4) ((sizeof(methodptr) * (m - m->class->methods)) & 0x0000ffff);

	/* synchronize instruction cache */

	asm_cacheflush(ra, 3 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_instanceof_flags ******************************************

   Machine code:

   <patched call position>
   818dff7c    lwz   r12,-132(r13)

*******************************************************************************/

bool patcher_checkcast_instanceof_flags(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	s4                 disp;
	u1                *pv;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	asm_cacheflush(ra, 4);

	/* patch class flags */

	*((s4 *) (pv + disp)) = (s4) c->flags;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_instanceof_interface **************************************

   Machine code:

   <patched call position>
   81870000    lwz   r12,0(r7)
   800c0010    lwz   r0,16(r12)
   34000000    addic.        r0,r0,0
   408101fc    ble-  0x3002e518
   800c0000    lwz   r0,0(r12)

*******************************************************************************/

bool patcher_checkcast_instanceof_interface(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (opt_showdisassemble)
		ra = ra + 4;

	/* patch super class index */

	*((s4 *) (ra + 2 * 4)) |= (s4) (-(c->index) & 0x0000ffff);

	*((s4 *) (ra + 4 * 4)) |= (s4) ((OFFSET(vftbl_t, interfacetable[0]) -
									 c->index * sizeof(methodptr*)) & 0x0000ffff);

	/* synchronize instruction cache */

	asm_cacheflush(ra, 5 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_class *****************************************************

   Machine code:

   <patched call position>
   81870000    lwz   r12,0(r7)
   800c0014    lwz   r0,20(r12)
   818dff78    lwz   r12,-136(r13)

*******************************************************************************/

bool patcher_checkcast_class(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	s4                 disp;
	u1                *pv;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	asm_cacheflush(ra, 4);

	/* patch super class' vftbl */

	*((ptrint *) (pv + disp)) = (ptrint) c->vftbl;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_instanceof_class ****************************************************

   Machine code:

   <patched call position>
   817d0000    lwz   r11,0(r29)
   818dff8c    lwz   r12,-116(r13)

*******************************************************************************/

bool patcher_instanceof_class(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	s4                 disp;
	u1                *pv;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	asm_cacheflush(ra, 4);

	/* patch super class' vftbl */

	*((ptrint *) (pv + disp)) = (ptrint) c->vftbl;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_clinit **************************************************************

   XXX

*******************************************************************************/

bool patcher_clinit(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	c     = (classinfo *)         *((ptrint *) (sp + 2 * 4));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* check if the class is initialized */

	if (!c->initialized) {
		if (!initialize_class(c)) {
			PATCHER_MONITOREXIT;

			return false;
		}
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	asm_cacheflush(ra, 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_resolve_native ******************************************************

   XXX

*******************************************************************************/

#if !defined(ENABLE_STATICVM)
bool patcher_resolve_native(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	methodinfo        *m;
	s4                 disp;
	u1                *pv;
	functionptr        f;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	m     = (methodinfo *)        *((ptrint *) (sp + 2 * 4));
	disp  =                       *((s4 *)     (sp + 1 * 4));
	pv    = (u1 *)                *((ptrint *) (sp + 0 * 4));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 5 * 4)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* resolve native function */

	if (!(f = native_resolve_function(m))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	asm_cacheflush(ra, 4);

	/* patch native function pointer */

	*((ptrint *) (pv + disp)) = (ptrint) f;

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
