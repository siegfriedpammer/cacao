/* src/vm/jit/alpha/patcher.c - Alpha code patching functions

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

   $Id: patcher.c 2719 2005-06-15 15:59:49Z twisti $

*/


#include "vm/jit/alpha/types.h"

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
   a73bff98    ldq     t11,-104(pv)
   a2590000    ldl     a2,0(t11)

*******************************************************************************/

bool patcher_get_putstatic(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	unresolved_field  *uf;
	u1                *pv;
	fieldinfo         *fi;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

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

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch the field value's address */

	*((ptrint *) (pv + offset)) = (ptrint) &(fi->value);

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_get_putfield ********************************************************

   Machine code:

   <patched call position>
   a2af0020    ldl     a5,32(s6)

*******************************************************************************/

bool patcher_get_putfield(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	unresolved_field  *uf;
	u1                *pv;
	fieldinfo         *fi;

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* patch the field's offset */

	*((u4 *) ra) |= (s2) (fi->offset & 0x0000ffff);

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_new *********************************************************

   Machine code:

   a61bff80    ldq     a0,-128(pv)
   <patched call postition>
   a77bff78    ldq     pv,-136(pv)
   6b5b4000    jsr     (pv)

*******************************************************************************/

bool patcher_builtin_new(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s2                offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - (4 + 4);
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 4)) = mcode;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch the classinfo pointer */

	*((ptrint *) (pv + offset)) = (ptrint) c;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) (ra + 4)) & 0x0000ffff);

	/* patch new function address */

	*((ptrint *) (pv + offset)) = (ptrint) BUILTIN_new;

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_newarray ****************************************************

   Machine code:

   a63bff88    ldq     a1,-120(pv)
   <patched call position>
   a77bff80    ldq     pv,-128(pv)
   6b5b4000    jsr     (pv)

*******************************************************************************/

bool patcher_builtin_newarray(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - (4 + 4);
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 4)) = mcode;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch the class' vftbl pointer */

	*((ptrint *) (pv + offset)) = (ptrint) c->vftbl;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) (ra + 4)) & 0x0000ffff);

	/* patch new function address */

	*((ptrint *) (pv + offset)) = (ptrint) BUILTIN_newarray;

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_multianewarray **********************************************

   Machine code:

   <patched call position>
   221f0002    lda     a0,2(zero)
   a63bff80    ldq     a1,-128(pv)
   47de0412    mov     sp,a2
   a77bff78    ldq     pv,-136(pv)
   6b5b4000    jsr     (pv)

*******************************************************************************/

bool patcher_builtin_multianewarray(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) (ra + 4)) & 0x0000ffff);

	/* patch the class' vftbl pointer */

	*((ptrint *) (pv + offset)) = (ptrint) c->vftbl;

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_arraycheckcast **********************************************

   Machine code:

   a63bfe60    ldq     a1,-416(pv)
   <patched call position>
   a77bfe58    ldq     pv,-424(pv)
   6b5b4000    jsr     (pv)

*******************************************************************************/

bool patcher_builtin_arraycheckcast(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - (2 * 4);
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 4)) = mcode;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch the class' vftbl pointer */

	*((ptrint *) (pv + offset)) = (ptrint) c->vftbl;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) (ra + 4)) & 0x0000ffff);

	/* patch new function address */

	*((ptrint *) (pv + offset)) = (ptrint) BUILTIN_arraycheckcast;

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_arrayinstanceof *********************************************

   Machine code:

   a63bfeb0    ldq     a1,-336(pv)
   <patched call position>
   a77bfea8    ldq     pv,-344(pv)
   6b5b4000    jsr     (pv)

*******************************************************************************/

bool patcher_builtin_arrayinstanceof(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s4                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - (4 + 4);
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 4)) = mcode;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch the class' vftbl pointer */
	
	*((ptrint *) (pv + offset)) = (ptrint) c->vftbl;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) (ra + 4)) & 0x0000ffff);

	/* patch new function address */

	*((ptrint *) (pv + offset)) = (ptrint) BUILTIN_arrayinstanceof;

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokestatic_special ************************************************

   Machine code:

   <patched call position>
   a77bffa8    ldq     pv,-88(pv)
   6b5b4000    jsr     (pv)

******************************************************************************/

bool patcher_invokestatic_special(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	unresolved_method *um;
	u1                *pv;
	methodinfo        *m;
	s4                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	um    = (unresolved_method *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch stubroutine */

	*((ptrint *) (pv + offset)) = (ptrint) m->stubroutine;

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokevirtual *******************************************************

   Machine code:

   <patched call position>
   a7900000    ldq     at,0(a0)
   a77c0100    ldq     pv,256(at)
   6b5b4000    jsr     (pv)

*******************************************************************************/

bool patcher_invokevirtual(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	um    = (unresolved_method *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* patch vftbl index */

	*((s4 *) (ra + 4)) |= (s4) ((OFFSET(vftbl_t, table[0]) +
								 sizeof(methodptr) * m->vftblindex) & 0x0000ffff);

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokeinterface *****************************************************

   Machine code:

   <patched call position>
   a7900000    ldq     at,0(a0)
   a79cffa0    ldq     at,-96(at)
   a77c0018    ldq     pv,24(at)
   6b5b4000    jsr     (pv)

*******************************************************************************/

bool patcher_invokeinterface(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	um    = (unresolved_method *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* patch interfacetable index */

	*((s4 *) (ra + 4)) |= (s4) ((OFFSET(vftbl_t, interfacetable[0]) -
								 sizeof(methodptr*) * m->class->index) & 0x0000ffff);

	/* patch method offset */

	*((s4 *) (ra + 4 + 4)) |=
		(s4) ((sizeof(methodptr) * (m - m->class->methods)) & 0x0000ffff);

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_instanceof_flags ******************************************

   Machine code:

   <patched call position>

*******************************************************************************/

bool patcher_checkcast_instanceof_flags(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch class flags */

	*((s4 *) (pv + offset)) = (s4) c->flags;

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_instanceof_interface **************************************

   Machine code:

   <patched call position>
   a78e0000    ldq     at,0(s5)
   a3bc001c    ldl     gp,28(at)
   23bdfffd    lda     gp,-3(gp)
   efa0002e    ble     gp,0x00000200002bf6b0
   a7bcffe8    ldq     gp,-24(at)

*******************************************************************************/

bool patcher_checkcast_instanceof_interface(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* patch super class index */

	*((s4 *) (ra + 2 * 4)) |= (s4) (-(c->index) & 0x0000ffff);

	*((s4 *) (ra + 4 * 4)) |= (s4) ((OFFSET(vftbl_t, interfacetable[0]) -
									 c->index * sizeof(methodptr*)) & 0x0000ffff);

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_instanceof_class ******************************************

   Machine code:

   <patched call position>
   a7940000    ldq     at,0(a4)
   a7bbff28    ldq     gp,-216(pv)

*******************************************************************************/

bool patcher_checkcast_instanceof_class(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) (ra + 4)) & 0x0000ffff);

	/* patch super class' vftbl */

	*((ptrint *) (pv + offset)) = (ptrint) c->vftbl;

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

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

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	c     = (classinfo *)         *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

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

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_resolve_native ******************************************************

   XXX

*******************************************************************************/

bool patcher_resolve_native(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	methodinfo        *m;
	u1                *pv;
	functionptr        f;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u4 *)     (sp + 1 * 8));
	m     = (methodinfo *)        *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* resolve native function */

	if (!(f = native_resolve_function(m))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch native function pointer */

	*((ptrint *) (pv + offset)) = (ptrint) f;

	/* synchronize instruction cache */

	asm_sync_instruction_cache();

	PATCHER_MARK_PATCHED_MONITOREXIT;

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
