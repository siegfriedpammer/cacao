/* src/vm/jit/alpha/patcher.cpp - Alpha code patching functions

   Copyright (C) 1996-2013
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

#include <cassert>

#include "vm/types.hpp"

#include "vm/jit/aarch64/md.hpp"
#include "vm/jit/aarch64/emit-asm.hpp"

#include "mm/memory.hpp"

#include "native/native.hpp"

#include "vm/jit/builtin.hpp"
#include "vm/class.hpp"
#include "vm/field.hpp"
#include "vm/initialize.hpp"
#include "vm/options.hpp"
#include "vm/references.hpp"
#include "vm/resolve.hpp"

#include "vm/jit/asmpart.hpp"
#include "vm/jit/codegen-common.hpp"
#include "vm/jit/patcher-common.hpp"
#include "vm/jit/methodheader.hpp"


/**
 * Helper function to patch in the correct LOAD instruction
 * as we have ambiguity.
 */
static void patch_helper_ldr(u1 *codeptr, s4 offset, bool isint)
{
	assert(offset >= -255);

	codegendata codegen;
	codegendata *cd = &codegen; // just a 'dummy' codegendata so we can reuse emit methods
	cd->mcodeptr = codeptr;
	u4 code = *((s4 *) cd->mcodeptr);
	u1 targetreg = code & 0x1f;
	u1 basereg = (code >> 5) & 0x1f;
	u8 oldptr = (u8) cd->mcodeptr;
	if (isint)
		emit_ldr_imm32(cd, targetreg, basereg, offset);
	else
		emit_ldr_imm(cd, targetreg, basereg, offset);
	assert(((u8) cd->mcodeptr) - oldptr == 4);
}


static void patch_helper_cmp_imm(u1 *codeptr, s4 offset)
{
	codegendata codegen;
	codegendata *cd = &codegen; // just a 'dummy' codegendata so we can reuse emit methods
	cd->mcodeptr = codeptr;
	u4 code = *((s4 *) cd->mcodeptr);
	u1 reg = (code >> 5) & 0x1f;
	u8 oldptr = (u8) cd->mcodeptr;
	if (offset >= 0)
		emit_cmn_imm32(cd, reg, offset);
	else
		emit_cmp_imm32(cd, reg, -offset);
	assert(((u8) cd->mcodeptr) - oldptr == 4);
}

static void patch_helper_mov_imm(u1 *codeptr, s4 offset)
{
	codegendata codegen;
	codegendata *cd = &codegen; // just a 'dummy' codegendata so we can reuse emit methods
	cd->mcodeptr = codeptr;
	u4 code = *((s4 *) cd->mcodeptr);
	u1 reg = code & 0x1f;
	u8 oldptr = (u8) cd->mcodeptr;
	if (offset >= 0)
		emit_mov_imm(cd, reg, offset);
	else 
		emit_movn_imm(cd, reg, -offset-1);
	assert(((u8) cd->mcodeptr) - oldptr == 4);
}



/* patcher_patch_code **********************************************************

   Just patches back the original machine code.

*******************************************************************************/

void patcher_patch_code(patchref_t* pr)
{
	// Patch back original code.
	*((uint32_t*) pr->mpc) = pr->mcode;

	// Synchronize instruction cache.
	md_icacheflush((void*) pr->mpc, 4);
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

	/* patch the classinfo pointer */
	*((ptrint *) datap) = (ptrint) c;

	// Synchronize data cache
	md_dcacheflush(datap, SIZEOF_VOID_P);

	// Patch back the original code.
	patcher_patch_code(pr);

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

	/* patch super class' vftbl */
	*((ptrint *) datap) = (ptrint) c->vftbl;

	// Synchronize data cache
	md_dcacheflush(datap, SIZEOF_VOID_P);

	// Patch back the original code.
	patcher_patch_code(pr);

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

	/* patch class flags */
	*((s4 *) datap) = (s4) c->flags;

	// Synchronize data cache
	md_dcacheflush(datap, 4);

	// Patch back the original code.
	patcher_patch_code(pr);

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

	/* patch the field value's address */
	*((intptr_t *) datap) = (intptr_t) fi->value;

	// Synchroinze data cache
	md_dcacheflush(datap, SIZEOF_VOID_P);

	// Patch back the original code.
	patcher_patch_code(pr);

	return true;
}


/* patcher_get_putfield ********************************************************

   Machine code:

   <patched call position>
   a2af0020    ldl     a5,32(s6)

*******************************************************************************/

bool patcher_get_putfield(patchref_t *pr)
{
	unresolved_field *uf;
	fieldinfo        *fi;

	uf    = (unresolved_field *) pr->ref;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	/* We need some stuff for code generation */
	codegendata codegen;
	codegen.mcodeptr = (u1 *) &pr->mcode;
	AsmEmitter asme(&codegen);
	u4 code = *((s4 *) codegen.mcodeptr);
	u1 targetreg = code & 0x1f;
	u1 basereg = (code >> 5) & 0x1f;

	/* patch the field's offset into the instruction */
	u1 isload = (pr->mcode >> 22) & 0x3;
	if (isload) {
		switch (fi->type) {
			case TYPE_ADR:
				asme.ald(targetreg, basereg, fi->offset);
				break;
			case TYPE_LNG:
				asme.lld(targetreg, basereg, fi->offset);
				break;
			case TYPE_INT:
				asme.ild(targetreg, basereg, fi->offset);
				break;
			case TYPE_FLT:
				asme.fld(targetreg, basereg, fi->offset);
				break;
			case TYPE_DBL:
				asme.dld(targetreg, basereg, fi->offset);
				break;
			default:
				os::abort("Unsupported type in patcher_get_putfield");
		}
	} else {
		switch (fi->type) {
			case TYPE_ADR:
				asme.ast(targetreg, basereg, fi->offset);
				break;
			case TYPE_LNG:
				asme.lst(targetreg, basereg, fi->offset);
				break;
			case TYPE_INT:
				asme.ist(targetreg, basereg, fi->offset);
				break;
			case TYPE_FLT:
				asme.fst(targetreg, basereg, fi->offset);
				break;
			case TYPE_DBL:
				asme.dst(targetreg, basereg, fi->offset);
				break;
			default:
				os::abort("Unsupported type in patcher_get_putfield");
		}
	}

	// Synchronize instruction cache
	md_icacheflush((void*)pr->mpc, 8);

	// Patch back the original code.
	patcher_patch_code(pr);

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

	/* patch stubroutine */
	*((ptrint *) datap) = (ptrint) m->stubroutine;

	// Synchronize data cache
	md_dcacheflush(datap, SIZEOF_VOID_P);

	// Patch back the original code.
	patcher_patch_code(pr);

	return true;
}


/* patcher_invokevirtual *******************************************************

   Machine code:

   <patched call position>
   a7900000    ldr     x16, [x0]
   a77c0100    ldr     x17, [x16, #patched]
   6b5b4000    blr     x17

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

	/* patch vftbl index */
	s4 disp = OFFSET(vftbl_t, table[0]) + sizeof(methodptr) * m->vftblindex;
	patch_helper_ldr((ra + 4), disp, false);
	
	// Synchronize instruction cache
	md_icacheflush(ra + 4, 4);

	// Patch back the original code.
	patcher_patch_code(pr);

	return true;
}


/* patcher_invokeinterface *****************************************************

   Machine code:

   <patched call position>
   mov      x9, <interfacetable index>
   mov      x10, <method offset>
   ldr		x16, [x0, _]
   ldr      x16, [x16, x9]
   ldr      x17, [x16, x10]

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

	/* patch interfacetable index */

	s4 offset = OFFSET(vftbl_t, interfacetable[0]) - sizeof(methodptr*) * m->clazz->index;
	patch_helper_mov_imm((ra + 4) , offset);

	/* patch method offset */
	offset = (s4) (sizeof(methodptr) * (m - m->clazz->methods));
	patch_helper_mov_imm((ra + 8), offset);

	// Synchronize instruction cache
	md_icacheflush(ra + 4, 8);

	// Patch back the original code.
	patcher_patch_code(pr);

	return true;
}


/* patcher_checkcast_interface *************************************************

   Machine code:

   <patched call position>
   ldr		w11, [x10, _]
   cmp      w11, <super index to patch>
   b.?
   <illegal instruction for classcast ex>
   mov      x11, <second offset to patch>
   ldr      x11, [x10, x11]

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

	/* patch super class index */
	s4 offset = (s4) (-(c->index));
	patch_helper_cmp_imm((ra + 1 * 4), offset);

	offset = (s4) (OFFSET(vftbl_t, interfacetable[0]) - c->index * sizeof(methodptr*));
	patch_helper_mov_imm((ra + 4 * 4), offset);

	// Synchronize instruction cache
	md_icacheflush(ra + 4, 3 * 4);

	// Patch back the original code.
	patcher_patch_code(pr);

	return true;
}


/* patcher_instanceof_interface ************************************************

   Machine code:

   <patched call position>
   ldr		w11, [w9, OFFSET(vftbl_t, interfacetablelength)]
   cmp      w11, <superindex to patch>
   b.le     <somewhere below>
   mov      x11, <second offset to patch>
   ldr      x9, [x9, x11]

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

	/* patch super class index */

	s4 offset = (s4) (-(c->index));
	patch_helper_cmp_imm((ra + 2 * 4), offset);

	offset = (OFFSET(vftbl_t, interfacetable[0]) - c->index * sizeof(methodptr*));
	patch_helper_mov_imm((ra + 4 * 4), offset);

	// Synchronize instruction cache
	md_icacheflush(ra + 2 * 4, 8);

	// Patch back the original code.
	patcher_patch_code(pr);

	return true;
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
