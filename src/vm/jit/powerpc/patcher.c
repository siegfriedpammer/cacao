/* src/vm/jit/powerpc/patcher.c - PowerPC code patching functions

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
#include <stdint.h>

#include "vm/types.h"

#include "vm/jit/powerpc/md.h"

#include "mm/memory.h"

#include "native/native.h"

#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/field.h"
#include "vm/initialize.h"
#include "vm/options.h"
#include "vm/references.h"
#include "vm/resolve.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/patcher-common.h"


#define PATCH_BACK_ORIGINAL_MCODE \
	*((u4 *) pr->mpc) = (u4) pr->mcode; \
	md_icacheflush((u1 *) pr->mpc, 4);


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
   806dffc4    lwz   r3,-60(r13)
   81adffc0    lwz   r13,-64(r13)
   7da903a6    mtctr r13
   4e800421    bctrl


   MULTIANEWARRAY:

   <patched call position>
   808dffc0    lwz   r4,-64(r13)
   38a10038    addi  r5,r1,56
   81adffbc    lwz   r13,-68(r13)
   7da903a6    mtctr r13
   4e800421    bctrl


   ARRAYCHECKCAST:

   <patched call position>
   808dffd8    lwz   r4,-40(r13)
   81adffd4    lwz   r13,-44(r13)
   7da903a6    mtctr r13
   4e800421    bctrl

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

	/* synchronize data cache */

	md_dcacheflush(datap, SIZEOF_VOID_P);

	return true;
}


/* patcher_resolve_classref_to_vftbl *******************************************

   CHECKCAST (class):

   <patched call position>
   81870000    lwz   r12,0(r7)
   800c0014    lwz   r0,20(r12)
   818dff78    lwz   r12,-136(r13)


   INSTANCEOF (class):

   <patched call position>
   817d0000    lwz   r11,0(r29)
   818dff8c    lwz   r12,-116(r13)

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

	/* synchronize data cache */

	md_dcacheflush(datap, SIZEOF_VOID_P);

	return true;
}


/* patcher_resolve_classref_to_flags *******************************************

   CHECKCAST/INSTANCEOF:

   <patched call position>
   818dff7c    lwz   r12,-132(r13)

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

	/* synchronize data cache */

	md_dcacheflush(datap, SIZEOF_VOID_P);

	return true;
}


/* patcher_get_putstatic *******************************************************

   Machine code:

   <patched call position>
   816dffc8    lwz   r11,-56(r13)
   80ab0000    lwz   r5,0(r11)

*******************************************************************************/

bool patcher_get_putstatic(patchref_t *pr)
{
	u1               *ra;
	unresolved_field *uf;
	u1               *datap;
	fieldinfo        *fi;

	/* get stuff from the stack */

	ra    = (u1 *)                pr->mpc;
	uf    = (unresolved_field *)  pr->ref;
	datap = (u1 *)                pr->datap;

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

	/* synchronize data cache */

	md_dcacheflush(datap, SIZEOF_VOID_P);

	return true;
}


/* patcher_get_putfield ********************************************************

   Machine code:

   <patched call position>
   811f0014    lwz   r8,20(r31)

*******************************************************************************/

bool patcher_get_putfield(patchref_t *pr)
{
	u1               *ra;
	unresolved_field *uf;
	fieldinfo        *fi;
	s2                disp;

	ra = (u1 *)               pr->mpc;
	uf = (unresolved_field *) pr->ref;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show NOPs, we have to skip them */

	if (opt_shownops)
		ra = ra + 1 * 4;

	/* patch the field's offset */

	if (IS_LNG_TYPE(fi->type)) {
		/* If the field has type long, we have to patch two
		   instructions.  But we have to check which instruction
		   is first.  We do that with the offset of the first
		   instruction. */

		disp = *((u4 *) (ra + 0 * 4));

		if (disp == 4) {
			*((u4 *) (ra + 0 * 4)) &= 0xffff0000;
			*((u4 *) (ra + 0 * 4)) |= (s2) ((fi->offset + 4) & 0x0000ffff);
			*((u4 *) (ra + 1 * 4)) |= (s2) ((fi->offset + 0) & 0x0000ffff);
		}
		else {
			*((u4 *) (ra + 0 * 4)) |= (s2) ((fi->offset + 0) & 0x0000ffff);
			*((u4 *) (ra + 1 * 4)) &= 0xffff0000;
			*((u4 *) (ra + 1 * 4)) |= (s2) ((fi->offset + 4) & 0x0000ffff);
		}
	}
	else
		*((u4 *) (ra + 0 * 4)) |= (s2) (fi->offset & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra + 0 * 4, 2 * 4);

	return true;
}


/* patcher_invokestatic_special ************************************************

   Machine code:

   <patched call position>
   81adffd8    lwz   r13,-40(r13)
   7da903a6    mtctr r13
   4e800421    bctrl

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

	/* synchronize data cache */

	md_dcacheflush(datap, SIZEOF_VOID_P);

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

bool patcher_invokevirtual(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;
	s4                 disp;

	/* get stuff from the stack */

	ra = (u1 *)                pr->mpc;
	um = (unresolved_method *) pr->ref;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show NOPs, we have to skip them */

	if (opt_shownops)
		ra = ra + 1 * 4;

	/* patch vftbl index */

	disp = (OFFSET(vftbl_t, table[0]) + sizeof(methodptr) * m->vftblindex);

	*((s4 *) (ra + 1 * 4)) |= (disp & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra + 1 * 4, 1 * 4);

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

bool patcher_invokeinterface(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;
	s4                 disp;

	/* get stuff from the stack */

	ra = (u1 *)                pr->mpc;
	um = (unresolved_method *) pr->ref;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show NOPs, we have to skip them */

	if (opt_shownops)
		ra = ra + 1 * 4;

	/* patch interfacetable index */

	disp = OFFSET(vftbl_t, interfacetable[0]) -
		sizeof(methodptr*) * m->clazz->index;

	/* XXX TWISTI: check displacement */

	*((s4 *) (ra + 1 * 4)) |= (disp & 0x0000ffff);

	/* patch method offset */

	disp = sizeof(methodptr) * (m - m->clazz->methods);

	/* XXX TWISTI: check displacement */

	*((s4 *) (ra + 2 * 4)) |= (disp & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra + 1 * 4, 2 * 4);

	return true;
}


/* patcher_checkcast_interface *************************************************

   Machine code:

   <patched call position>
   81870000    lwz     r12,0(r7)
   800c0010    lwz     r0,16(r12)
   34000000    addic.  r0,r0,0
   41810008    bgt-    0x014135d8
   83c00003    lwz     r30,3(0)
   800c0000    lwz     r0,0(r12)

*******************************************************************************/

bool patcher_checkcast_interface(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;
	s4                 disp;

	/* get stuff from the stack */

	ra = (u1 *)                pr->mpc;
	cr = (constant_classref *) pr->ref;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show NOPs, we have to skip them */

	if (opt_shownops)
		ra = ra + 1 * 4;

	/* patch super class index */

	disp = -(c->index);

	*((s4 *) (ra + 2 * 4)) |= (disp & 0x0000ffff);

	disp = OFFSET(vftbl_t, interfacetable[0]) - c->index * sizeof(methodptr*);

	*((s4 *) (ra + 5 * 4)) |= (disp & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra + 2 * 4, 4 * 4);

	return true;
}


/* patcher_instanceof_interface ************************************************

   Machine code:

   <patched call position>
   81870000    lwz     r12,0(r7)
   800c0010    lwz     r0,16(r12)
   34000000    addic.  r0,r0,0
   41810008    bgt-    0x014135d8
   83c00003    lwz     r30,3(0)
   800c0000    lwz     r0,0(r12)

*******************************************************************************/

bool patcher_instanceof_interface(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;
	s4                 disp;

	/* get stuff from the stack */

	ra = (u1 *)                pr->mpc;
	cr = (constant_classref *) pr->ref;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show NOPs, we have to skip them */

	if (opt_shownops)
		ra = ra + 1 * 4;

	/* patch super class index */

	disp = -(c->index);

	*((s4 *) (ra + 2 * 4)) |= (disp & 0x0000ffff);

	disp = OFFSET(vftbl_t, interfacetable[0]) - c->index * sizeof(methodptr*);

	*((s4 *) (ra + 4 * 4)) |= (disp & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra + 2 * 4, 3 * 4);

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
