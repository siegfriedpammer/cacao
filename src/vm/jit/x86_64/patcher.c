/* src/vm/jit/x86_64/patcher.c - x86_64 code patching functions

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

   $Id: patcher.c 2412 2005-04-29 15:22:08Z twisti $

*/


#include "vm/jit/x86_64/types.h"
#include "vm/builtin.h"
#include "vm/field.h"
#include "vm/initialize.h"
#include "vm/options.h"
#include "vm/references.h"
#include "vm/jit/helper.h"
#include "vm/exceptions.h"


/* patcher_get_putstatic *******************************************************

   Machine code:

   <patched call position>
   4d 8b 15 86 fe ff ff             mov    -378(%rip),%r10

*******************************************************************************/

bool patcher_get_putstatic(u1 *sp)
{
	u1                *ra;
	java_objectheader *o;
	u8                 mcode;
	unresolved_field  *uf;
	fieldinfo         *fi;
	s4                 offset;
	void              *beginJavaStack;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 0 * 8));

	beginJavaStack=              (void*)(sp + 3 * 8);

	*dontfillinexceptionstacktrace=true;

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	printf("monitorenter before: %p\n", THREADOBJECT);
	builtin_monitorenter(o);
	printf("monitorenter after : %p\n", THREADOBJECT);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* check if the field's class is initialized */

	*dontfillinexceptionstacktrace=false;
	if (!fi->class->initialized) {
		bool init;
		{
			/*struct native_stackframeinfo {
	        		void *oldThreadspecificHeadValue;
		        	void **addressOfThreadspecificHead;
			        methodinfo *method;
        			void *beginOfJavaStackframe; only used if != 0
			        void *returnToFromNative;
			}*/
			/* more or less the same as the above sfi setup is done in the assembler code by the prepare/remove functions*/
			native_stackframeinfo sfi;
			sfi.returnToFromNative=(void*)ra;
			sfi.beginOfJavaStackframe=beginJavaStack;
			sfi.method=0; /*internal*/
			sfi.addressOfThreadspecificHead=builtin_asm_get_stackframeinfo();
			sfi.oldThreadspecificHeadValue=*(sfi.addressOfThreadspecificHead);
			*(sfi.addressOfThreadspecificHead)=&sfi;

			init=initialize_class(fi->class);

			*(sfi.addressOfThreadspecificHead)=sfi.oldThreadspecificHeadValue;
		}
		if (!init)
		{
			return false;
		}
	}

	*dontfillinexceptionstacktrace=false;

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* get RIP offset from machine instruction */

	offset = *((u4 *) (ra + 3));

	/* patch the field value's address (+ 7: is the size of the RIP move) */

	*((ptrint *) (ra + 7 + offset)) = (ptrint) &(fi->value);

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

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

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	uf    = (unresolved_field *)  *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the fieldinfo */

	if (!(fi = helper_resolve_fieldinfo(uf))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch the field's offset: we check for the field type, because the     */
	/* instructions have different lengths                                    */

	if (IS_FLT_DBL_TYPE(fi->type)) {
		*((u4 *) (ra + 5)) = (u4) (fi->offset);

	} else {
		u1 byte;

		/* check for special case: %rsp or %r12 as base register */

		byte = *(ra + 3);

		if (byte == 0x24)
			*((u4 *) (ra + 4)) = (u4) (fi->offset);
		else
			*((u4 *) (ra + 3)) = (u4) (fi->offset);
	}

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

	return true;
}


/* patcher_builtin_new *********************************************************

   Machine code:

   48 bf a0 f0 92 00 00 00 00 00    mov    $0x92f0a0,%rdi
   <patched call position>
   48 b8 00 00 00 00 00 00 00 00    mov    $0x0,%rax
   48 ff d0                         callq  *%rax

*******************************************************************************/

bool patcher_builtin_new(u1 *sp)
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

	ra = ra - (10 + 5);
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* patch back original code */

	*((u8 *) (ra + 10)) = mcode;

	/* patch the classinfo pointer */

	*((ptrint *) (ra + 2)) = (ptrint) c;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch new function address */

	*((ptrint *) (ra + 10 + 2)) = (ptrint) BUILTIN_new;

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

	return true;
}


/* patcher_builtin_newarray ****************************************************

   Machine code:

   48 be 88 13 9b 00 00 00 00 00    mov    $0x9b1388,%rsi
   <patched call position>
   48 b8 00 00 00 00 00 00 00 00    mov    $0x0,%rax
   48 ff d0                         callq  *%rax

*******************************************************************************/

bool patcher_builtin_newarray(u1 *sp)
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

	ra = ra - (10 + 5);
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* patch back original code */

	*((u8 *) (ra + 10)) = mcode;

	/* patch the class' vftbl pointer */

	*((ptrint *) (ra + 2)) = (ptrint) c->vftbl;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch new function address */

	*((ptrint *) (ra + 10 + 2)) = (ptrint) BUILTIN_newarray;

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

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

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	cr    = (constant_classref *) *((ptrint *) (sp + 0 * 8));

	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch the class' vftbl pointer */

	*((ptrint *) (ra + 10 + 2)) = (ptrint) c->vftbl;

	/* patch new function address */

	*((ptrint *) (ra + 10 + 10 + 3 + 2)) = (ptrint) BUILTIN_multianewarray;

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

	return true;
}


/* patcher_builtin_checkarraycast **********************************************

   Machine code:

   48 be b8 3f b2 00 00 00 00 00    mov    $0xb23fb8,%rsi
   <patched call position>
   48 b8 00 00 00 00 00 00 00 00    mov    $0x0,%rax
   48 ff d0                         callq  *%rax

*******************************************************************************/

bool patcher_builtin_checkarraycast(u1 *sp)
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

	ra = ra - (10 + 5);
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* patch back original code */

	*((u8 *) (ra + 10)) = mcode;

	/* patch the class' vftbl pointer */

	*((ptrint *) (ra + 2)) = (ptrint) c->vftbl;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch new function address */

	*((ptrint *) (ra + 10 + 2)) = (ptrint) BUILTIN_checkarraycast;

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

	return true;
}


/* patcher_builtin_arrayinstanceof *********************************************

   Machine code:

   48 be 30 3c b2 00 00 00 00 00    mov    $0xb23c30,%rsi
   <patched call position>
   48 b8 00 00 00 00 00 00 00 00    mov    $0x0,%rax
   48 ff d0                         callq  *%rax

*******************************************************************************/

bool patcher_builtin_arrayinstanceof(u1 *sp)
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

	ra = ra - (10 + 5);
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the classinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* patch back original code */

	*((u8 *) (ra + 10)) = mcode;

	/* patch the class' vftbl pointer */

	*((ptrint *) (ra + 2)) = (ptrint) c->vftbl;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch new function address */

	*((ptrint *) (ra + 10 + 2)) = (ptrint) BUILTIN_arrayinstanceof;

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

	return true;
}


/* patcher_invokestatic_special ************************************************

   XXX

*******************************************************************************/

bool patcher_invokestatic_special(u1 *sp)
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

	ra = ra - 5;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}
	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch stubroutine */

	*((ptrint *) (ra + 2)) = (ptrint) m->stubroutine;

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

	return true;
}


/* patcher_invokevirtual *******************************************************

   XXX

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

	ra = ra - 5;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch vftbl index */

	*((s4 *) (ra + 3 + 3)) = (s4) (OFFSET(vftbl_t, table[0]) +
								   sizeof(methodptr) * m->vftblindex);

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

	return true;
}


/* patcher_invokeinterface *****************************************************

   XXX

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

	ra = ra - 5;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the fieldinfo */

	if (!(m = helper_resolve_methodinfo(um))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch interfacetable index */

	*((s4 *) (ra + 3 + 3)) = (s4) (OFFSET(vftbl_t, interfacetable[0]) -
								   sizeof(methodptr) * m->class->index);

	/* patch method offset */

	*((s4 *) (ra + 3 + 7 + 3)) =
		(s4) (sizeof(methodptr) * (m - m->class->methods));

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

	return true;
}


/* patcher_checkcast_instanceof_flags ******************************************

   XXX

*******************************************************************************/

bool patcher_checkcast_instanceof_flags(u1 *sp)
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

	ra = ra - 5;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch class flags */

	*((s4 *) (ra + 2)) = (s4) c->flags;

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

	return true;
}


/* patcher_checkcast_instanceof_interface **************************************

   XXX

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

	ra = ra - 5;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch super class index */

	*((s4 *) (ra + 7 + 3)) = (s4) c->index;

	*((s4 *) (ra + 7 + 7 + 3 + 6 + 3)) =
		(s4) (OFFSET(vftbl_t, interfacetable[0]) -
			  c->index * sizeof(methodptr*));

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

	return true;
}


/* patcher_checkcast_class *****************************************************

   XXX

*******************************************************************************/

bool patcher_checkcast_class(u1 *sp)
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

	ra = ra - 5;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch super class' vftbl */

	*((ptrint *) (ra + 2)) = (ptrint) c->vftbl;
	*((ptrint *) (ra + 10 + 7 + 7 + 3 + 2)) = (ptrint) c->vftbl;

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

	return true;
}


/* patcher_instanceof_class ****************************************************

   XXX

*******************************************************************************/

bool patcher_instanceof_class(u1 *sp)
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

	ra = ra - 5;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

	*dontfillinexceptionstacktrace=true;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* get the fieldinfo */

	if (!(c = helper_resolve_classinfo(cr))) {
		*dontfillinexceptionstacktrace=false;
		return false;
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

	/* if we show disassembly, we have to skip the nop's */

	if (showdisassemble)
		ra = ra + 5;

	/* patch super class' vftbl */

	*((ptrint *) (ra + 2)) = (ptrint) c->vftbl;

	*dontfillinexceptionstacktrace=false;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

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
	void              *beginJavaStack;

	/* get stuff from the stack */

	ra    = (u1 *)                *((ptrint *) (sp + 3 * 8));
	o     = (java_objectheader *) *((ptrint *) (sp + 2 * 8));
	mcode =                       *((u8 *)     (sp + 1 * 8));
	c     = (classinfo *)         *((ptrint *) (sp + 0 * 8));

	beginJavaStack =      (void*) (sp + 3 * 8);

	/*printf("beginJavaStack: %p, ra %p\n",beginJavaStack,ra);*/
	/* calculate and set the new return address */

	ra = ra - 5;
	*((ptrint *) (sp + 3 * 8)) = (ptrint) ra;

#if defined(USE_THREADS)
	/* enter a monitor on the patching position */

	builtin_monitorenter(o);

	/* check if the position has already been patched */

	if (o->vftbl) {
		builtin_monitorexit(o);

		return true;
	}
#endif

	/* check if the class is initialized */

	if (!c->initialized) {
		bool init;
		{
			/*struct native_stackframeinfo {
	        		void *oldThreadspecificHeadValue;
		        	void **addressOfThreadspecificHead;
			        methodinfo *method;
        			void *beginOfJavaStackframe; only used if != 0
			        void *returnToFromNative;
			}*/
			/* more or less the same as the above sfi setup is done in the assembler code by the prepare/remove functions*/
			native_stackframeinfo sfi;
			sfi.returnToFromNative=(void*)ra;
			sfi.beginOfJavaStackframe=beginJavaStack;
			sfi.method=0; /*internal*/
			sfi.addressOfThreadspecificHead=builtin_asm_get_stackframeinfo();
			sfi.oldThreadspecificHeadValue=*(sfi.addressOfThreadspecificHead);
			*(sfi.addressOfThreadspecificHead)=&sfi;

			init=initialize_class(c);

			*(sfi.addressOfThreadspecificHead)=sfi.oldThreadspecificHeadValue;
		}
		if (!init)
		{
			return false;
		}
	}

	/* patch back original code */

	*((u8 *) ra) = mcode;

#if defined(USE_THREADS)
	/* leave the monitor on the patching position */

	builtin_monitorexit(o);

	/* this position has been patched */

	o->vftbl = (vftbl_t *) 1;
#endif

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
