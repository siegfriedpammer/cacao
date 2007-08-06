/* src/vm/jit/mips/patcher.c - MIPS code patching functions

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: patcher.c 8264 2007-08-06 16:02:28Z twisti $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "vm/jit/mips/codegen.h"

#include "mm/memory.h"

#include "native/native.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/initialize.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/md.h"
#include "vm/jit/patcher-common.h"

#include "vmcore/class.h"
#include "vmcore/field.h"
#include "vmcore/options.h"
#include "vm/resolve.h"
#include "vmcore/references.h"


#define PATCH_BACK_ORIGINAL_MCODE \
	*((u4 *) pr->mpc) = (u4) pr->mcode; \
	md_icacheflush((u1 *) pr->mpc, PATCHER_CALL_SIZE);


/* patcher_initialize_class ****************************************************

   Initalizes a given classinfo pointer.  This function does not patch
   any data.

*******************************************************************************/

bool patcher_initialize_class(patchref_t *pr)
{
	classinfo *c;

	/* get stuff from the stack */

	c = (classinfo *) pr->ref;

	/* check if the class is initialized */

	if (!(c->state & CLASS_INITIALIZED))
		if (!initialize_class(c))
			return false;

	PATCH_BACK_ORIGINAL_MCODE;

	return true;
}


/* patcher_resolve_class *****************************************************

   Initalizes a given classinfo pointer.  This function does not patch
   any data.

*******************************************************************************/

#ifdef ENABLE_VERIFIER
bool patcher_resolve_class(patchref_t *pr)
{
	unresolved_class *uc;

	/* get stuff from the stack */

	uc = (unresolved_class *) pr->ref;

	/* resolve the class and check subtype constraints */

	if (!resolve_class_eager_no_access_check(uc))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	return true;
}
#endif /* ENABLE_VERIFIER */


/* patcher_get_putstatic *******************************************************

   Machine code:

   <patched call position>
   dfc1ffb8    ld       at,-72(s8)
   fc250000    sd       a1,0(at)

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

	if (!(fi->class->state & CLASS_INITIALIZED))
		if (!initialize_class(fi->class))
			return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the field value's address */

	*((ptrint *) datap) = (ptrint) &(fi->value);

	/* synchronize data cache */

	md_dcacheflush(datap, SIZEOF_VOID_P);

	return true;
}


/* patcher_get_putfield ********************************************************

   Machine code:

   <patched call position>
   8ee90020    lw       a5,32(s7)

*******************************************************************************/

bool patcher_get_putfield(patchref_t *pr)
{
	u1               *ra;
	unresolved_field *uf;
	fieldinfo        *fi;

	/* get stuff from the stack */

	ra = (u1 *)               pr->mpc;
	uf = (unresolved_field *) pr->ref;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_shownops)
		ra = ra + PATCHER_CALL_SIZE;

#if SIZEOF_VOID_P == 4
	if (IS_LNG_TYPE(fi->type)) {
# if WORDS_BIGENDIAN == 1
		/* ATTENTION: order of these instructions depend on M_LLD_INTERN */
		*((u4 *) (ra + 0 * 4)) |= (s2) ((fi->offset + 0) & 0x0000ffff);
		*((u4 *) (ra + 1 * 4)) |= (s2) ((fi->offset + 4) & 0x0000ffff);
# else
		/* ATTENTION: order of these instructions depend on M_LLD_INTERN */
		*((u4 *) (ra + 0 * 4)) |= (s2) ((fi->offset + 4) & 0x0000ffff);
		*((u4 *) (ra + 1 * 4)) |= (s2) ((fi->offset + 0) & 0x0000ffff);
# endif
	}
	else
#endif
		*((u4 *) (ra + 0 * 4)) |= (s2) (fi->offset & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra, 2 * 4);

	return true;
}


/* patcher_resolve_classref_to_classinfo ***************************************

   ACONST:

   <patched call postition>
   dfc4ff98    ld       a0,-104(s8)

   MULTIANEWARRAY:

   <patched call position>
   dfc5ff90    ld       a1,-112(s8)
   03a03025    move     a2,sp
   dfd9ff88    ld       t9,-120(s8)
   0320f809    jalr     t9
   00000000    nop

   ARRAYCHECKCAST:

   <patched call position>
   dfc5ffc0    ld       a1,-64(s8)
   dfd9ffb8    ld       t9,-72(s8)
   0320f809    jalr     t9
   00000000    nop

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

	*((intptr_t *) datap) = (intptr_t) c;

	/* synchronize data cache */

	md_dcacheflush(datap, SIZEOF_VOID_P);

	return true;
}


/* patcher_resolve_classref_to_vftbl *******************************************

   CHECKCAST (class):
   INSTANCEOF (class):

   <patched call position>
   dd030000    ld       v1,0(a4)
   dfd9ff18    ld       t9,-232(s8)

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

	*((intptr_t *) datap) = (intptr_t) c->vftbl;

	/* synchronize data cache */

	md_dcacheflush(datap, SIZEOF_VOID_P);

	return true;
}


/* patcher_resolve_classref_to_flags *******************************************

   CHECKCAST/INSTANCEOF:

   <patched call position>
   8fc3ff24    lw       v1,-220(s8)
   30630200    andi     v1,v1,512
   1060000d    beq      v1,zero,0x000000001051824c
   00000000    nop

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

	*((int32_t *) datap) = (int32_t) c->flags;

	/* synchronize data cache */

	md_dcacheflush(datap, sizeof(int32_t));

	return true;
}


/* patcher_resolve_native ******************************************************

   XXX

*******************************************************************************/

#if !defined(WITH_STATIC_CLASSPATH)
bool patcher_resolve_native_function(patchref_t *pr)
{
	methodinfo  *m;
	u1          *datap;
	functionptr  f;

	/* get stuff from the stack */

	m     = (methodinfo *) pr->ref;
	datap = (u1 *)         pr->datap;

	/* resolve native function */

	if (!(f = native_resolve_function(m)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch native function pointer */

	*((ptrint *) datap) = (ptrint) f;

	/* synchronize data cache */

	md_dcacheflush(datap, SIZEOF_VOID_P);

	return true;
}
#endif /* !defined(WITH_STATIC_CLASSPATH) */


/* patcher_invokestatic_special ************************************************

   Machine code:

   <patched call position>
   dfdeffc0    ld       s8,-64(s8)
   03c0f809    jalr     s8
   00000000    nop

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
   dc990000    ld       t9,0(a0)
   df3e0040    ld       s8,64(t9)
   03c0f809    jalr     s8
   00000000    nop

*******************************************************************************/

bool patcher_invokevirtual(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra = (u1 *)                pr->mpc;
	um = (unresolved_method *) pr->ref;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_shownops)
		ra = ra + PATCHER_CALL_SIZE;

	/* patch vftbl index */

	*((s4 *) (ra + 1 * 4)) |=
		(s4) ((OFFSET(vftbl_t, table[0]) +
			   sizeof(methodptr) * m->vftblindex) & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra + 1 * 4, 1 * 4);

	return true;
}


/* patcher_invokeinterface *****************************************************

   Machine code:

   <patched call position>
   dc990000    ld       t9,0(a0)
   df39ffa0    ld       t9,-96(t9)
   df3e0018    ld       s8,24(t9)
   03c0f809    jalr     s8
   00000000    nop

*******************************************************************************/

bool patcher_invokeinterface(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra = (u1 *)                pr->mpc;
	um = (unresolved_method *) pr->ref;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_shownops)
		ra = ra + PATCHER_CALL_SIZE;

	/* patch interfacetable index */

	*((s4 *) (ra + 1 * 4)) |=
		(s4) ((OFFSET(vftbl_t, interfacetable[0]) -
			   sizeof(methodptr*) * m->class->index) & 0x0000ffff);

	/* patch method offset */

	*((s4 *) (ra + 2 * 4)) |=
		(s4) ((sizeof(methodptr) * (m - m->class->methods)) & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra + 1 * 4, 2 * 4);

	return true;
}


/* patcher_checkcast_interface *************************************************

   Machine code:

   <patched call position>
   dd030000    ld       v1,0(a4)
   8c79001c    lw       t9,28(v1)
   27390000    addiu    t9,t9,0
   1b200082    blez     t9,zero,0x000000001051843c
   00000000    nop
   dc790000    ld       t9,0(v1)

*******************************************************************************/

bool patcher_checkcast_interface(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra = (u1 *)                pr->mpc;
	cr = (constant_classref *) pr->ref;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_shownops)
		ra = ra + PATCHER_CALL_SIZE;

	/* patch super class index */

	*((s4 *) (ra + 2 * 4)) |= (s4) (-(c->index) & 0x0000ffff);
	/* 	*((s4 *) (ra + 5 * 4)) |= (s4) ((OFFSET(vftbl_t, interfacetable[0]) - */
	/* 									 c->index * sizeof(methodptr*)) & 0x0000ffff); */
	*((s4 *) (ra + 6 * 4)) |=
		(s4) ((OFFSET(vftbl_t, interfacetable[0]) -
			   c->index * sizeof(methodptr*)) & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra + 2 * 4, 5 * 4);

	return true;
}


/* patcher_instanceof_interface ************************************************

   Machine code:

   <patched call position>
   dd030000    ld       v1,0(a4)
   8c79001c    lw       t9,28(v1)
   27390000    addiu    t9,t9,0
   1b200082    blez     t9,zero,0x000000001051843c
   00000000    nop
   dc790000    ld       t9,0(v1)

*******************************************************************************/

bool patcher_instanceof_interface(patchref_t *pr)
{
	u1                *ra;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra = (u1 *)                pr->mpc;
	cr = (constant_classref *) pr->ref;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_shownops)
		ra = ra + PATCHER_CALL_SIZE;

	/* patch super class index */

	*((s4 *) (ra + 2 * 4)) |= (s4) (-(c->index) & 0x0000ffff);
	*((s4 *) (ra + 5 * 4)) |=
		(s4) ((OFFSET(vftbl_t, interfacetable[0]) -
			   c->index * sizeof(methodptr*)) & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra + 2 * 4, 4 * 4);

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
