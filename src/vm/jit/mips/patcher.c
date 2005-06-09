/* src/vm/jit/mips/patcher.c - MIPS code patching functions

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

   $Id: patcher.c 2624 2005-06-09 20:35:21Z twisti $

*/


#include "config.h"
#include "vm/jit/mips/types.h"

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
   dfc1ffb8    ld       at,-72(s8)
   fc250000    sd       a1,0(at)

*******************************************************************************/

bool patcher_get_putstatic(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_field  *uf;
	u1                *pv;
	fieldinfo         *fi;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 2 * 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* check if the field's class is initialized */

	if (!fi->class->initialized)
		if (!initialize_class(fi->class))
			return false;

	/* patch back original code */

	*((u4 *) (ra + 0 * 4)) = mcode;
	*((u4 *) (ra + 1 * 4)) = mcode >> 32;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch the field value's address */

	*((ptrint *) (pv + offset)) = (ptrint) &(fi->value);

	/* synchronize instruction cache */

	docacheflush(ra, 2 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_get_putfield ********************************************************

   Machine code:

   <patched call position>
   8ee90020    lw       a5,32(s7)

*******************************************************************************/

bool patcher_get_putfield(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_field  *uf;
	fieldinfo         *fi;

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 2 * 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0 * 4)) = mcode;
	*((u4 *) (ra + 1 * 4)) = mcode >> 32;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* patch the field's offset */

	*((u4 *) ra) |= (s2) (fi->offset & 0x0000ffff);

	/* synchronize instruction cache */

	docacheflush(ra, 2 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_new *********************************************************

   Machine code:

   dfc4ff98    ld       a0,-104(s8)
   <patched call postition>
   dfd9ff90    ld       t9,-112(s8)
   0320f809    jalr     t9
   00000000    nop

*******************************************************************************/

bool patcher_builtin_new(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - (4 + 2 * 4);
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 1 * 4)) = mcode;
	*((u4 *) (ra + 2 * 4)) = mcode >> 32;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch the classinfo pointer */

	*((ptrint *) (pv + offset)) = (ptrint) c;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) (ra + 4)) & 0x0000ffff);

	/* patch new function address */

	*((ptrint *) (pv + offset)) = (ptrint) BUILTIN_new;

	/* synchronize instruction cache */

	docacheflush(ra + 4, 2 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_newarray ****************************************************

   Machine code:

   dfc5ffa0    ld       a1,-96(s8)
   <patched call position>
   dfd9ff98    ld       t9,-104(s8)
   0320f809    jalr     t9
   00000000    nop

*******************************************************************************/

bool patcher_builtin_newarray(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - (4 + 2 * 4);
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 1 * 4)) = mcode;
	*((u4 *) (ra + 2 * 4)) = mcode >> 32;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch the class' vftbl pointer */

	*((ptrint *) (pv + offset)) = (ptrint) c->vftbl;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) (ra + 4)) & 0x0000ffff);

	/* patch new function address */

	*((ptrint *) (pv + offset)) = (ptrint) BUILTIN_newarray;

	/* synchronize instruction cache */

	docacheflush(ra + 4, 2 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_multianewarray **********************************************

   Machine code:

   <patched call position>
   24040002    addiu    a0,zero,2
   dfc5ff90    ld       a1,-112(s8)
   03a03025    move     a2,sp
   dfd9ff88    ld       t9,-120(s8)
   0320f809    jalr     t9
   00000000    nop

*******************************************************************************/

bool patcher_builtin_multianewarray(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 2 * 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0 * 4)) = mcode;
	*((u4 *) (ra + 1 * 4)) = mcode >> 32;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) (ra + 4)) & 0x0000ffff);

	/* patch the class' vftbl pointer */

	*((ptrint *) (pv + offset)) = (ptrint) c->vftbl;

	/* synchronize instruction cache */

	docacheflush(ra, 2 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_arraycheckcast **********************************************

   Machine code:

   dfc5ffc0    ld       a1,-64(s8)
   <patched call position>
   dfd9ffb8    ld       t9,-72(s8)
   0320f809    jalr     t9
   00000000    nop

*******************************************************************************/

bool patcher_builtin_arraycheckcast(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 3 * 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 1 * 4)) = mcode;
	*((u4 *) (ra + 2 * 4)) = mcode >> 32;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch the class' vftbl pointer */

	*((ptrint *) (pv + offset)) = (ptrint) c->vftbl;

	/* if we show disassembly, we have to skip the nop */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) (ra + 1 * 4)) & 0x0000ffff);

	/* patch new function address */

	*((ptrint *) (pv + offset)) = (ptrint) BUILTIN_arraycheckcast;

	/* synchronize instruction cache */

	docacheflush(ra + 4, 2 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_arrayinstanceof *********************************************

   Machine code:

   dfc5fe98    ld       a1,-360(s8)
   <patched call position>
   dfd9fe90    ld       t9,-368(s8)
   0320f809    jalr     t9
   00000000    nop

*******************************************************************************/

bool patcher_builtin_arrayinstanceof(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s4                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 3 * 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 1 * 4)) = mcode;
	*((u4 *) (ra + 2 * 4)) = mcode >> 32;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch the class' vftbl pointer */
	
	*((ptrint *) (pv + offset)) = (ptrint) c->vftbl;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) (ra + 1 * 4)) & 0x0000ffff);

	/* patch new function address */

	*((ptrint *) (pv + offset)) = (ptrint) BUILTIN_arrayinstanceof;

	/* synchronize instruction cache */

	docacheflush(ra + 4, 2 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokestatic_special ************************************************

   Machine code:

   <patched call position>
   dfdeffc0    ld       s8,-64(s8)
   03c0f809    jalr     s8
   00000000    nop

******************************************************************************/

bool patcher_invokestatic_special(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_method *um;
	u1                *pv;
	methodinfo        *m;
	s4                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	um    = (unresolved_method *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 2 * 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0 * 4)) = mcode;
	*((u4 *) (ra + 1 * 4)) = mcode >> 32;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch stubroutine */

	*((ptrint *) (pv + offset)) = (ptrint) m->stubroutine;

	/* synchronize instruction cache */

	docacheflush(ra, 2 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

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

bool patcher_invokevirtual(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	um    = (unresolved_method *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 2 * 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0 * 4)) = mcode;
	*((u4 *) (ra + 1 * 4)) = mcode >> 32;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* patch vftbl index */

	*((s4 *) (ra + 1 * 4)) |= (s4) ((OFFSET(vftbl_t, table[0]) +
									 sizeof(methodptr) * m->vftblindex) & 0x0000ffff);

	/* synchronize instruction cache */

	docacheflush(ra, 2 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

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

bool patcher_invokeinterface(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	um    = (unresolved_method *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 2 * 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0 * 4)) = mcode;
	*((u4 *) (ra + 1 * 4)) = mcode >> 32;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* patch interfacetable index */

	*((s4 *) (ra + 1 * 4)) |= (s4) ((OFFSET(vftbl_t, interfacetable[0]) -
									 sizeof(methodptr*) * m->class->index) & 0x0000ffff);

	/* patch method offset */

	*((s4 *) (ra + 2 * 4)) |=
		(s4) ((sizeof(methodptr) * (m - m->class->methods)) & 0x0000ffff);

	/* synchronize instruction cache */

	docacheflush(ra, 2 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_instanceof_flags ******************************************

   Machine code:

   <patched call position>
   8fc3ff24    lw       v1,-220(s8)
   30630200    andi     v1,v1,512
   1060000d    beq      v1,zero,0x000000001051824c
   00000000    nop

*******************************************************************************/

bool patcher_checkcast_instanceof_flags(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 2 * 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0 * 4)) = mcode;
	*((u4 *) (ra + 1 * 4)) = mcode >> 32;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) ra) & 0x0000ffff);

	/* patch class flags */

	*((s4 *) (pv + offset)) = (s4) c->flags;

	/* synchronize instruction cache */

	docacheflush(ra, 2 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_instanceof_interface **************************************

   Machine code:

   <patched call position>
   dd030000    ld       v1,0(a4)
   8c79001c    lw       t9,28(v1)
   27390000    addiu    t9,t9,0
   1b200082    blez     t9,zero,0x000000001051843c
   00000000    nop
   dc790000    ld       t9,0(v1)

*******************************************************************************/

bool patcher_checkcast_instanceof_interface(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 2 * 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0 * 4)) = mcode;
	*((u4 *) (ra + 1 * 4)) = mcode >> 32;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* patch super class index */

	*((s4 *) (ra + 2 * 4)) |= (s4) (-(c->index) & 0x0000ffff);

	*((s4 *) (ra + 5 * 4)) |= (s4) ((OFFSET(vftbl_t, interfacetable[0]) -
									 c->index * sizeof(methodptr*)) & 0x0000ffff);

	/* synchronize instruction cache */

	docacheflush(ra, 6 * 4);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_instanceof_class ******************************************

   Machine code:

   <patched call position>
   dd030000    ld       v1,0(a4)
   dfd9ff18    ld       t9,-232(s8)

*******************************************************************************/

bool patcher_checkcast_instanceof_class(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	u1                *pv;
	classinfo         *c;
	s2                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));
	pv    = (u1 *)                *((ptrint *) (sp - 2 * 8));

	/* calculate and set the new return address */

	ra = ra - 2 * 4;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u4 *) (ra + 0 * 4)) = mcode;
	*((u4 *) (ra + 1 * 4)) = mcode >> 32;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 2 * 4;

	/* get the offset from machine instruction */

	offset = (s2) (*((u4 *) (ra + 1 * 4)) & 0x0000ffff);

	/* patch super class' vftbl */

	*((ptrint *) (pv + offset)) = (ptrint) c->vftbl;

	/* synchronize instruction cache */

	docacheflush(ra, 2 * 4);

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
	u8                 mcode;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	c     = (classinfo *)         *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 2 * 4;
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

	*((u4 *) (ra + 0)) = mcode;
	*((u4 *) (ra + 4)) = mcode >> 32;

	/* synchronize instruction cache */

	docacheflush(ra, 2 * 4);

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
