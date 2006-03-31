/* src/vm/jit/powerpc/patcher.c - PowerPC code patching functions

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

   Authors: Christian Thalinger

   Changes:

   $Id: patcher.c 4714 2006-03-31 07:14:10Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "mm/memory.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/field.h"
#include "vm/initialize.h"
#include "vm/options.h"
#include "vm/resolve.h"
#include "vm/references.h"
#include "vm/jit/asmpart.h"
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

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* check if the field's class is initialized */

	if (!(fi->class->state & CLASS_INITIALIZED)) {
		if (!initialize_class(fi->class)) {
			PATCHER_MONITOREXIT;

			return false;
		}
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	md_icacheflush(ra, 4);

	/* patch the field value's address */

	*((ptrint *) (pv + disp)) = (ptrint) &(fi->value);

	/* synchronize data cache */

	md_dcacheflush(pv + disp, SIZEOF_VOID_P);

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

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf))) {
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
		s2 disp;

		/* If the field has type long, we have to patch two
		   instructions.  But we have to check which instruction is
		   first.  We do that with the offset of the first
		   instruction. */

		disp = *((u4 *) (ra + 0));

#if WORDS_BIGENDIAN == 1
		if (disp == 4) {
			*((u4 *) (ra + 0)) |= (s2) ((fi->offset + 4) & 0x0000ffff);
			*((u4 *) (ra + 4)) |= (s2) ((fi->offset + 0) & 0x0000ffff);

		} else {
			*((u4 *) (ra + 0)) |= (s2) ((fi->offset + 0) & 0x0000ffff);
			*((u4 *) (ra + 4)) |= (s2) ((fi->offset + 4) & 0x0000ffff);
		}
#else
#error Fix me for LE
#endif
	} else {
		*((u4 *) ra) |= (s2) (fi->offset & 0x0000ffff);
	}

	/* synchronize instruction cache */

	md_icacheflush(ra, 8);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_aconst **************************************************************

   Machine code:

   <patched call postition>
   806dffc4    lwz   r3,-60(r13)
   81adffc0    lwz   r13,-64(r13)
   7da903a6    mtctr r13
   4e800421    bctrl

*******************************************************************************/

bool patcher_aconst(u1 *sp)
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

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	md_icacheflush(ra, 4);

	/* patch the classinfo pointer */

	*((ptrint *) (pv + disp)) = (ptrint) c;

	/* synchronize data cache */

	md_dcacheflush(pv + disp, SIZEOF_VOID_P);

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

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	md_icacheflush(ra, 4);

	/* patch the classinfo pointer */

	*((ptrint *) (pv + disp)) = (ptrint) c;

	/* synchronize data cache */

	md_dcacheflush(pv + disp, SIZEOF_VOID_P);

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

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	md_icacheflush(ra, 4);

	/* patch the classinfo pointer */

	*((ptrint *) (pv + disp)) = (ptrint) c;

	/* synchronize data cache */

	md_dcacheflush(pv + disp, SIZEOF_VOID_P);

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

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	md_icacheflush(ra, 4);

	/* patch stubroutine */

	*((ptrint *) (pv + disp)) = (ptrint) m->stubroutine;

	/* synchronize data cache */

	md_dcacheflush(pv + disp, SIZEOF_VOID_P);

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
	s4                 disp;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	um    = (unresolved_method *) *((ptrint *) (sp + 2 * 4));

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (opt_showdisassemble)
		ra = ra + 4;

	/* patch vftbl index */

	disp = (OFFSET(vftbl_t, table[0]) + sizeof(methodptr) * m->vftblindex);

	*((s4 *) (ra + 4)) |= (disp & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra, 2 * 4);

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
	s4                 disp;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	um    = (unresolved_method *) *((ptrint *) (sp + 2 * 4));

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (opt_showdisassemble)
		ra = ra + 4;

	/* patch interfacetable index */

	disp = (OFFSET(vftbl_t, interfacetable[0]) - sizeof(methodptr*) * m->class->index);

	*((s4 *) (ra + 1 * 4)) |= (disp & 0x0000ffff);

	/* patch method offset */

	disp = (sizeof(methodptr) * (m - m->class->methods));

	*((s4 *) (ra + 2 * 4)) |= (disp & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra, 3 * 4);

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

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	md_icacheflush(ra, 4);

	/* patch class flags */

	*((s4 *) (pv + disp)) = (s4) c->flags;

	/* synchronize data cache */

	md_dcacheflush(pv + disp, SIZEOF_VOID_P);

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
	s4                 disp;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	cr    = (constant_classref *) *((ptrint *) (sp + 2 * 4));

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop */

	if (opt_showdisassemble)
		ra = ra + 4;

	/* patch super class index */

	disp = -(c->index);

	*((s4 *) (ra + 2 * 4)) |= (disp & 0x0000ffff);

	disp = OFFSET(vftbl_t, interfacetable[0]) - c->index * sizeof(methodptr*);

	*((s4 *) (ra + 4 * 4)) |= (disp & 0x0000ffff);

	/* synchronize instruction cache */

	md_icacheflush(ra, 5 * 4);

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

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	md_icacheflush(ra, 4);

	/* patch super class' vftbl */

	*((ptrint *) (pv + disp)) = (ptrint) c->vftbl;

	/* synchronize data cache */

	md_dcacheflush(pv + disp, SIZEOF_VOID_P);

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

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	md_icacheflush(ra, 4);

	/* patch super class' vftbl */

	*((ptrint *) (pv + disp)) = (ptrint) c->vftbl;

	/* synchronize data cache */

	md_dcacheflush(pv + disp, SIZEOF_VOID_P);

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

	PATCHER_MONITORENTER;

	/* check if the class is initialized */

	if (!(c->state & CLASS_INITIALIZED)) {
		if (!initialize_class(c)) {
			PATCHER_MONITOREXIT;

			return false;
		}
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	md_icacheflush(ra, 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_athrow_areturn ******************************************************

   Machine code:

   <patched call position>

*******************************************************************************/

#ifdef ENABLE_VERIFIER
bool patcher_athrow_areturn(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u4                 mcode;
	unresolved_class  *uc;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 5 * 4));
	o     = (java_objectheader *) *((ptrint *) (sp + 4 * 4));
	mcode =                       *((u4 *)     (sp + 3 * 4));
	uc    = (unresolved_class *)  *((ptrint *) (sp + 2 * 4));

	PATCHER_MONITORENTER;

	/* resolve the class */

	if (!resolve_class(uc, resolveEager, false, &c)) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) ra) = mcode;

	/* synchronize instruction cache */

	md_icacheflush(ra, 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}
#endif /* ENABLE_VERIFIER */


/* patcher_resolve_native ******************************************************

   XXX

*******************************************************************************/

#if !defined(WITH_STATIC_CLASSPATH)
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

	ra = ra - 1 * 4;
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

	md_icacheflush(ra, 4);

	/* patch native function pointer */

	*((ptrint *) (pv + disp)) = (ptrint) f;

	/* synchronize data cache */

	md_dcacheflush(pv + disp, SIZEOF_VOID_P);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}
#endif /* !defined(WITH_STATIC_CLASSPATH) */


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
