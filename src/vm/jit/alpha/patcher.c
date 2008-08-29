/* src/vm/jit/alpha/patcher.c - Alpha code patching functions

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "vm/jit/alpha/md.h"

#include "mm/memory.h"

#include "native/native.hpp"

#include "vm/jit/builtin.hpp"
#include "vm/class.h"
#include "vm/field.hpp"
#include "vm/initialize.h"
#include "vm/options.h"
#include "vm/references.h"
#include "vm/resolve.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/patcher-common.hpp"
#include "vm/jit/methodheader.h"


#define PATCH_BACK_ORIGINAL_MCODE \
	*((u4 *) pr->mpc) = (u4) pr->mcode; \
    md_icacheflush(NULL, 0);


/* patcher_patch_code **********************************************************

   Just patches back the original machine code.

*******************************************************************************/

void patcher_patch_code(patchref_t *pr)
{
	PATCH_BACK_ORIGINAL_MCODE;
}


/* patcher_resolve_classref_to_classinfo ***************************************

   ACONST:

   <patched call postition>
   a61bff80    ldq     a0,-128(pv)

   MULTIANEWARRAY:

   <patched call position>
   a63bff80    ldq     a1,-128(pv)
   47de0412    mov     sp,a2
   a77bff78    ldq     pv,-136(pv)
   6b5b4000    jsr     (pv)

   ARRAYCHECKCAST:

   <patched call position>
   a63bfe60    ldq     a1,-416(pv)
   a77bfe58    ldq     pv,-424(pv)
   6b5b4000    jsr     (pv)

*******************************************************************************/

bool patcher_resolve_classref_to_classinfo(patchref_t *pr)
{
	constant_classref *cr;
	u1                *datap;
	classinfo         *c;

	/* get stuff from the stack */

	cr    = (constant_classref *) pr->ref;
	datap = (u1 *)                pr->datap;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the classinfo pointer */

	*((ptrint *) datap) = (ptrint) c;

	return true;
}


/* patcher_resolve_classref_to_vftbl *******************************************

   CHECKCAST (class):
   INSTANCEOF (class):

   <patched call position>
   a7940000    ldq     at,0(a4)
   a7bbff28    ldq     gp,-216(pv)

*******************************************************************************/

bool patcher_resolve_classref_to_vftbl(patchref_t *pr)
{
	constant_classref *cr;
	u1                *datap;
	classinfo         *c;

	/* get stuff from the stack */

	cr    = (constant_classref *) pr->ref;
	datap = (u1 *)                pr->datap;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch super class' vftbl */

	*((ptrint *) datap) = (ptrint) c->vftbl;

	return true;
}


/* patcher_resolve_classref_to_flags *******************************************

   CHECKCAST/INSTANCEOF:

   <patched call position>

*******************************************************************************/

bool patcher_resolve_classref_to_flags(patchref_t *pr)
{
	constant_classref *cr;
	u1                *datap;
	classinfo         *c;

	/* get stuff from the stack */

	cr    = (constant_classref *) pr->ref;
	datap = (u1 *)                pr->datap;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch class flags */

	*((s4 *) datap) = (s4) c->flags;

	return true;
}


/* patcher_get_putstatic *******************************************************

   Machine code:

   <patched call position>
   a73bff98    ldq     t11,-104(pv)
   a2590000    ldl     a2,0(t11)

*******************************************************************************/

bool patcher_get_putstatic(patchref_t *pr)
{
	unresolved_field *uf;
	u1               *datap;
	fieldinfo        *fi;

	/* get stuff from the stack */

	uf    = (unresolved_field *) pr->ref;
	datap = (u1 *)               pr->datap;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	/* check if the field's class is initialized */

	if (!(fi->clazz->state & CLASS_INITIALIZED))
		if (!initialize_class(fi->clazz))
			return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the field value's address */

	*((intptr_t *) datap) = (intptr_t) fi->value;

	return true;
}


/* patcher_get_putfield ********************************************************

   Machine code:

   <patched call position>
   a2af0020    ldl     a5,32(s6)

*******************************************************************************/

bool patcher_get_putfield(patchref_t *pr)
{
	u1               *ra;
	unresolved_field *uf;
	fieldinfo        *fi;

	ra    = (u1 *)               pr->mpc;
	uf    = (unresolved_field *) pr->ref;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the field's offset into the instruction */

	*((u4 *) ra) |= (s2) (fi->offset & 0x0000ffff);

	md_icacheflush(NULL, 0);

	return true;
}


/* patcher_invokestatic_special ************************************************

   Machine code:

   <patched call position>
   a77bffa8    ldq     pv,-88(pv)
   6b5b4000    jsr     (pv)

******************************************************************************/

bool patcher_invokestatic_special(patchref_t *pr)
{
	unresolved_method *um;
	u1                *datap;
	methodinfo        *m;

	/* get stuff from the stack */

	um    = (unresolved_method *) pr->ref;
	datap = (u1 *)                pr->datap;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch stubroutine */

	*((ptrint *) datap) = (ptrint) m->stubroutine;

	return true;
}


/* patcher_invokevirtual *******************************************************

   Machine code:

   <patched call position>
   a7900000    ldq     at,0(a0)
   a77c0100    ldq     pv,256(at)
   6b5b4000    jsr     (pv)

*******************************************************************************/

bool patcher_invokevirtual(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	um    = (unresolved_method *) pr->ref;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch vftbl index */

	*((s4 *) (ra + 4)) |= (s4) ((OFFSET(vftbl_t, table[0]) +
								 sizeof(methodptr) * m->vftblindex) & 0x0000ffff);

	md_icacheflush(NULL, 0);

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

bool patcher_invokeinterface(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	um    = (unresolved_method *) pr->ref;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch interfacetable index */

	*((s4 *) (ra + 4)) |= (s4) ((OFFSET(vftbl_t, interfacetable[0]) -
								 sizeof(methodptr*) * m->clazz->index) & 0x0000ffff);

	/* patch method offset */

	*((s4 *) (ra + 4 + 4)) |=
		(s4) ((sizeof(methodptr) * (m - m->clazz->methods)) & 0x0000ffff);

	md_icacheflush(NULL, 0);

	return true;
}


/* patcher_checkcast_interface *************************************************

   Machine code:

   <patched call position>
   a78e0000    ldq     at,0(s5)
   a3bc001c    ldl     gp,28(at)
   23bdfffd    lda     gp,-3(gp)
   efa0002e    ble     gp,0x00000200002bf6b0
   a7bcffe8    ldq     gp,-24(at)

*******************************************************************************/

bool patcher_checkcast_interface(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	cr    = (constant_classref *) pr->ref;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch super class index */

	*((s4 *) (ra + 2 * 4)) |= (s4) (-(c->index) & 0x0000ffff);

	*((s4 *) (ra + 5 * 4)) |= (s4) ((OFFSET(vftbl_t, interfacetable[0]) -
									 c->index * sizeof(methodptr*)) & 0x0000ffff);

	md_icacheflush(NULL, 0);

	return true;
}


/* patcher_instanceof_interface ************************************************

   Machine code:

   <patched call position>
   a78e0000    ldq     at,0(s5)
   a3bc001c    ldl     gp,28(at)
   23bdfffd    lda     gp,-3(gp)
   efa0002e    ble     gp,0x00000200002bf6b0
   a7bcffe8    ldq     gp,-24(at)

*******************************************************************************/

bool patcher_instanceof_interface(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	cr    = (constant_classref *) pr->ref;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch super class index */

	*((s4 *) (ra + 2 * 4)) |= (s4) (-(c->index) & 0x0000ffff);

	*((s4 *) (ra + 4 * 4)) |= (s4) ((OFFSET(vftbl_t, interfacetable[0]) -
									 c->index * sizeof(methodptr*)) & 0x0000ffff);

	md_icacheflush(NULL, 0);

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
