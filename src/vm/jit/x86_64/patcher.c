/* src/vm/jit/x86_64/patcher.c - x86_64 code patching functions

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

   $Id: patcher.c 4530 2006-02-21 09:11:53Z twisti $

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
#include "vm/references.h"
#include "vm/resolve.h"
#include "vm/jit/patcher.h"


/* patcher_get_putstatic *******************************************************

   Machine code:

   <patched call position>
   4d 8b 15 86 fe ff ff             mov    -378(%rip),%r10
   49 8b 32                         mov    (%r10),%rsi

*******************************************************************************/

bool patcher_get_putstatic(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_field  *uf;
	s4                 disp;
	fieldinfo         *fi;
	s4                 offset;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 1 * 8));
	disp  =                       *((s4 *)     (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

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

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* get RIP offset from machine instruction */

	offset = *((u4 *) (ra + 3));

	/* patch the field value's address (+ 7: is the size of the RIP move) */

	*((ptrint *) (ra + 7 + offset)) = (ptrint) &(fi->value);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_get_putfield ********************************************************

   Machine code:

   <patched call position>
   45 8b 8f 00 00 00 00             mov    0x0(%r15),%r9d

*******************************************************************************/

bool patcher_get_putfield(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_field  *uf;
	fieldinfo         *fi;
	u1                 byte;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code (instruction code is smaller than 8 bytes) */

	*((u4 *) (ra + 0)) = (u4) mcode;
	*((u1 *) (ra + 4)) = (u1) (mcode >> 32);

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch the field's offset: we check for the field type, because the     */
	/* instructions have different lengths                                    */

	if (IS_INT_LNG_TYPE(fi->type)) {
		/* check for special case: %rsp or %r12 as base register */

		byte = *(ra + 3);

		if (byte == 0x24)
			*((u4 *) (ra + 4)) = (u4) (fi->offset);
		else
			*((u4 *) (ra + 3)) = (u4) (fi->offset);

	} else {
		/* check for special case: %rsp or %r12 as base register */

		byte = *(ra + 5);

		if (byte == 0x24)
			*((u4 *) (ra + 6)) = (u4) (fi->offset);
		else
			*((u4 *) (ra + 5)) = (u4) (fi->offset);
	}

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_putfieldconst *******************************************************

   Machine code:

   <patched call position>
   41 c7 85 00 00 00 00 7b 00 00 00    movl   $0x7b,0x0(%r13)

*******************************************************************************/

bool patcher_putfieldconst(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_field  *uf;
	fieldinfo         *fi;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch the field's offset */

	if (IS_2_WORD_TYPE(fi->type) || IS_ADR_TYPE(fi->type)) {
		/* handle special case when the base register is %r12 */

		if (*(ra + 2) == 0x84) {
			*((u4 *) (ra + 4)) = (u4) (fi->offset);
			*((u4 *) (ra + 12 + 4)) = (u4) (fi->offset + 4);

		} else {
			*((u4 *) (ra + 3)) = (u4) (fi->offset);
			*((u4 *) (ra + 11 + 3)) = (u4) (fi->offset + 4);
		}

	} else {
		/* handle special case when the base register is %r12 */

		if (*(ra + 2) == 0x84)
			*((u4 *) (ra + 4)) = (u4) (fi->offset);
		else
			*((u4 *) (ra + 3)) = (u4) (fi->offset);
	}

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_aconst **************************************************************

   Machine code:

   <patched call position>
   48 bf a0 f0 92 00 00 00 00 00    mov    $0x92f0a0,%rdi

*******************************************************************************/

bool patcher_aconst(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch the classinfo pointer */

	*((ptrint *) (ra + 2)) = (ptrint) c;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_multianewarray **********************************************

   Machine code:

   <patched call position>
   48 bf 02 00 00 00 00 00 00 00    mov    $0x2,%rdi
   48 be 30 40 b2 00 00 00 00 00    mov    $0xb24030,%rsi
   48 89 e2                         mov    %rsp,%rdx
   48 b8 7c 96 4b 00 00 00 00 00    mov    $0x4b967c,%rax
   48 ff d0                         callq  *%rax

*******************************************************************************/

bool patcher_builtin_multianewarray(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch the classinfo pointer */

	*((ptrint *) (ra + 10 + 2)) = (ptrint) c;

	/* patch new function address */

	*((ptrint *) (ra + 10 + 10 + 3 + 2)) = (ptrint) BUILTIN_multianewarray;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_builtin_arraycheckcast **********************************************

   Machine code:

   <patched call position>
   48 be b8 3f b2 00 00 00 00 00    mov    $0xb23fb8,%rsi
   48 b8 00 00 00 00 00 00 00 00    mov    $0x0,%rax
   48 ff d0                         callq  *%rax

*******************************************************************************/

bool patcher_builtin_arraycheckcast(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch the classinfo pointer */

	*((ptrint *) (ra + 2)) = (ptrint) c;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokestatic_special ************************************************

   Machine code:

   <patched call position>
   49 ba 00 00 00 00 00 00 00 00    mov    $0x0,%r10
   49 ff d2                         callq  *%r10

*******************************************************************************/

bool patcher_invokestatic_special(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	um    = (unresolved_method *) *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}
	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch stubroutine */

	*((ptrint *) (ra + 2)) = (ptrint) m->stubroutine;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokevirtual *******************************************************

   Machine code:

   <patched call position>
   4c 8b 17                         mov    (%rdi),%r10
   49 8b 82 00 00 00 00             mov    0x0(%r10),%rax
   48 ff d0                         callq  *%rax

*******************************************************************************/

bool patcher_invokevirtual(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	um    = (unresolved_method *) *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch vftbl index */

	*((s4 *) (ra + 3 + 3)) = (s4) (OFFSET(vftbl_t, table[0]) +
								   sizeof(methodptr) * m->vftblindex);

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_invokeinterface *****************************************************

   Machine code:

   <patched call position>
   4c 8b 17                         mov    (%rdi),%r10
   4d 8b 92 00 00 00 00             mov    0x0(%r10),%r10
   49 8b 82 00 00 00 00             mov    0x0(%r10),%rax
   48 ff d0                         callq  *%rax

*******************************************************************************/

bool patcher_invokeinterface(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	um    = (unresolved_method *) *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(m = resolve_method_eager(um))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch interfacetable index */

	*((s4 *) (ra + 3 + 3)) = (s4) (OFFSET(vftbl_t, interfacetable[0]) -
								   sizeof(methodptr) * m->class->index);

	/* patch method offset */

	*((s4 *) (ra + 3 + 7 + 3)) =
		(s4) (sizeof(methodptr) * (m - m->class->methods));

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_instanceof_flags ******************************************

   Machine code:

   <patched call position>
   41 ba 00 00 00 00                mov    $0x0,%r10d
   41 81 e2 00 02 00 00             and    $0x200,%r10d
   0f 84 35 00 00 00                je     0x00002aaaaab01479

*******************************************************************************/

bool patcher_checkcast_instanceof_flags(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch class flags */

	*((s4 *) (ra + 2)) = (s4) c->flags;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_instanceof_interface **************************************

   Machine code:

   <patched call position>
   45 8b 9a 1c 00 00 00             mov    0x1c(%r10),%r11d
   49 81 eb 00 00 00 00             sub    $0x0,%r11
   4d 85 db                         test   %r11,%r11
   0f 8e 94 04 00 00                jle    0x00002aaaaab018f8
   4d 8b 9a 00 00 00 00             mov    0x0(%r10),%r11

*******************************************************************************/

bool patcher_checkcast_instanceof_interface(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch super class index */

	*((s4 *) (ra + 7 + 3)) = (s4) c->index;

	*((s4 *) (ra + 7 + 7 + 3 + 6 + 3)) =
		(s4) (OFFSET(vftbl_t, interfacetable[0]) -
			  c->index * sizeof(methodptr*));

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_checkcast_class *****************************************************

   Machine code:

   <patched call position>
   49 bb 00 00 00 00 00 00 00 00    mov    $0x0,%r11
   45 8b 92 20 00 00 00             mov    0x20(%r10),%r10d
   45 8b 9b 20 00 00 00             mov    0x20(%r11),%r11d
   4d 29 da                         sub    %r11,%r10
   49 bb 00 00 00 00 00 00 00 00    mov    $0x0,%r11

*******************************************************************************/

bool patcher_checkcast_class(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch super class' vftbl */

	*((ptrint *) (ra + 2)) = (ptrint) c->vftbl;
	*((ptrint *) (ra + 10 + 7 + 7 + 3 + 2)) = (ptrint) c->vftbl;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_instanceof_class ****************************************************

   Machine code:

   <patched call position>
   49 ba 00 00 00 00 00 00 00 00    mov    $0x0,%r10

*******************************************************************************/

bool patcher_instanceof_class(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	constant_classref *cr;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* get the fieldinfo */

	if (!(c = resolve_classref_eager(cr))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch super class' vftbl */

	*((ptrint *) (ra + 2)) = (ptrint) c->vftbl;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}


/* patcher_clinit **************************************************************

   May be used for GET/PUTSTATIC and in native stub.

   Machine code:

   <patched call position>
   4d 8b 15 92 ff ff ff             mov    -110(%rip),%r10
   49 89 1a                         mov    %rbx,(%r10)

*******************************************************************************/

bool patcher_clinit(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	c     = (classinfo *)         *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* check if the class is initialized */

	if (!(c->state & CLASS_INITIALIZED)) {
		if (!initialize_class(c)) {
			PATCHER_MONITOREXIT;

			return false;
		}
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

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
	u8                 mcode;
	unresolved_class  *uc;
	classinfo         *c;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	uc    = (unresolved_class *)  *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* resolve the class */

	if (!resolve_class(uc, resolveEager, false, &c)) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	PATCHER_MARK_PATCHED_MONITOREXIT;

	return true;
}
#endif /* ENABLE_VERIFIER */


/* patcher_resolve_native ******************************************************

   Machine code:

   <patched call position>
   48 b8 00 00 00 00 00 00 00 00    mov    $0x0,%rax
   48 ff d0                         callq  *%rax

*******************************************************************************/

#if !defined(WITH_STATIC_CLASSPATH)
bool patcher_resolve_native(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	methodinfo        *m;
	functionptr        f;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 4 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 3 * 8));
	mcode =                       *((u8 *)     (sp + 2 * 8));
	m     = (methodinfo *)        *((ptrint *) (sp + 1 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 4 * 8)) = (ptrint) ra;

	PATCHER_MONITORENTER;

	/* resolve native function */

	if (!(f = native_resolve_function(m))) {
		PATCHER_MONITOREXIT;

		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (opt_showdisassemble)
		ra = ra + 5;

	/* patch native function pointer */

	*((ptrint *) (ra + 2)) = (ptrint) f;

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
