/* src/vm/jit/m68k/asmpart.S - Java-C interface functions for m68k

   Copyright (C) 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

*/


#include "config.h"

#include "md-abi.h"

#include "vm/jit/methodheader.h"


.text

.globl asm_vm_call_method
.globl asm_vm_call_method_int
.globl asm_vm_call_method_long
.globl asm_vm_call_method_float
.globl asm_vm_call_method_double
.globl asm_vm_call_method_end
.globl asm_vm_call_method_exception_handler

.globl asm_abstractmethoderror

.globl asm_handle_exception
.globl asm_handle_nat_exception

/*
 *	This functions implement the C prototyped funtion
 *	java_objectheader *asm_vm_call_method(methodinfo *m, s4 vmargscount,vm_arg *vmargs);
 *	the _int, _long, _float, _double are used for different return types
 *
 *	The function may still be uncompiled, so the jit compiler gets invoked.
 *
 *
 */

	/* this is the method header see src/vm/jit/methodheader.h */

	.align	4

	.long   0                           /* line number table start            */
	.long   0                           /* line number table size             */
	.long   0                           /* fltsave                            */
	.long   0                           /* intsave                            */
	.long   0                           /* isleaf                             */
	.long   0                           /* frame size                         */
	.long   0                           /* codeinfo pointer                   */


/*
	This method gets called with 3 arguments:
	1st arg: addres of method to call (fake invokevirtual here)
	2nd arg: uint64_t array of argument registers followed by stack 
	3rd arg: number of 8 byte stack slots to be copied.

	coldifire does not use any argument registers, so just the stack has to be copied
*/
asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:

#if defined(ENABLE_SOFTFLOAT)
	addal	#(-12*4),%sp						/* create stackframe to save registers, and 1 slot for method invocation */
	moveml	%d2/%d3/%d4/%d5/%d6/%d7/%a2/%a3/%a4/%a5/%fp,%sp@	/* save registers */

	/* fetch arguments from vmargs data structure */
	movel	%sp@(12*4+1*4),%a3				/* method */
	movel	%sp@(12*4+2*4),%a2				/* arg array */
	movel	%sp@(12*4+3*4),%d2				/* arg count */

	movel	%a3, %sp@(12*4)					/* copy method address to stackslot */
	leal	%sp@(12*4), %a3					/* and store that address in %a3 */
#else
	addal #(-12*4-6*8), %sp
	moveml	%d2/%d3/%d4/%d5/%d6/%d7/%a2/%a3/%a4/%a5/%fp,%sp@	/* save registers */
	fmovemd	%fp2/%fp3/%fp4/%fp5/%fp6/%fp7,%sp@(11*4)			/* save registers */

	/* fetch arguments from vmargs data structure */
	movel	%sp@(12*4+6*8+1*4),%a3				/* method */
	movel	%sp@(12*4+6*8+2*4),%a2				/* arg array */
	movel	%sp@(12*4+6*8+3*4),%d2				/* arg count */

	movel	%a3, %sp@(12*4+6*8)					/* copy method address to stackslot */
	leal	%sp@(12*4+6*8), %a3					/* and store that address in %a3 */
#endif

	moveal	%sp, %a5					/* memorize stack */

	tstl	%d2							/* do we have arguments ? */
	beq	L_asm_vm_call_method_copy_done
	movel	%d2, %d3						/* create stackframe */
	asll	#3,  %d3						/* number args * 8 */
	subal	%d3, %sp			
	moveal	%sp, %a4						/* %a4 is temp stack pointer */

L_asm_vm_call_method_copy_loop:
	movel	%a2@(0), %d3
	movel	%d3, %a4@(0)					/* copy 4 bytes */
	movel	%a2@(4), %d3
	movel	%d3, %a4@(4)					/* a whole stack slot has been copied */

	addal	#8, %a2
	addal	#8, %a4
	addl	#-1, %d2
	tstl	%d2								/* do we have more arguments ? */
	beq	L_asm_vm_call_method_copy_done
	br	L_asm_vm_call_method_copy_loop


L_asm_vm_call_method_copy_done:
	/* now we fake method invocation as it would happen from invokevirtual */
	/* this is needed as we patch the caller site, so we need a writeable slot */
	/* %a3 points to the address containing the method, %a3 == REG_METHODPTR */

	moveal	%a3@(0), %a4
	jsr	%a4@

L_asm_vm_call_method_return:
	movel	%a5, %sp						/* pop arguments off stack */

#if defined(ENABLE_SOFTFLOAT)
	moveml  %sp@, %d2/%d3/%d4/%d5/%d6/%d7/%a2/%a3/%a4/%a5/%fp	/* restore registers */
	addal 	#(12*4),%sp						/* restore stack */
#else
	fmovemd	%sp@(12*4), %fp2/%fp3/%fp4/%fp5/%fp6/%fp7			/* restore registers */
	moveml  %sp@, %d2/%d3/%d4/%d5/%d6/%d7/%a2/%a3/%a4/%a5/%fp	/* restore registers */
	addal 	#(12*4+6*8),%sp						/* restore stack */
#endif
	moveal	%d0, %a0						/* XXX return value in %a0, too, gcc sometimes expects addresses in %a0, wired */
asm_vm_call_method_end:						/* symbol needed to insert method into avl tree */
	rts										/* return to c code */

/* asm_vm_call_method_exception_handler	********************************************************************
 *
 * calls void *builtin_throw_exception(java_objectheader *xptr) when no other handler is appropiate
 * this functions gets called indirectly from asm_handle_exception, which back then moved xptr to %a2
 * clear software design is in the eye of the beholder.
 ************************************************************************************************************/
asm_vm_call_method_exception_handler:
	movel	%a2, %sp@-			/* push xptr argument */
	jsr 	builtin_throw_exception
	lea	%sp@(4), %sp			/* pop arg off stack */
	jmp 	L_asm_vm_call_method_return


asm_abstractmethoderror:

/* asm_handle_exception ********************************************************
*                                                                              *
*   This function handles an exception. It does not use the usual calling      *
*   conventions. The exception pointer is passed in REG_ATMP1 and the          *
*   pc from the exception raising position is passed in REG_ATMP2. It searches *
*   the local exception table for a handler. If no one is found, it unwinds    *
*   stacks and continues searching the callers.                                *
*                                                                              *
*   void asm_handle_exception (void);
*   exception object pointer...%a2   exception raising address...%a3           *
*                                                                              *
*******************************************************************************/
asm_handle_nat_exception:
	lea	%sp@(4), %sp
asm_handle_exception:
L_asm_handle_exception_stack_loop:
	/* save temporary registers */
	movel	%d0, %sp@-
	movel	%d1, %sp@-
	movel	%a0, %sp@-
	movel	%a1, %sp@-
#if !defined(ENABLE_SOFTFLOAT)
	addal	#-8*2, %sp
	fmovemd	%fp0, %sp@(0)
	fmovemd	%fp1, %sp@(8)
#endif

	/* we need the dseg, figure it out */
	movel	%a3, %sp@-				/* push ra argument */
	jsr	md_codegen_get_pv_from_pc	/* pv in %d0 now */
	movel	%d0, %d2				/* move to safe register */
	lea	%sp@(4), %sp				/* pop args off stack */

	/* now call the following c function */
	/* u1 *exceptions_handle_exception(java_objectheader *xptr, u1 *xpc, u1 *pv, u1 *sp) */
#if !defined(ENABLE_SOFTFLOAT)
	pea		%sp@(4*4 + 8*2)
#else
	pea		%sp@(4*4)
#endif
	movel	%d2,%sp@-
	movel	%a3,%sp@-
	movel	%a2,%sp@-
	jsr	exceptions_handle_exception		/* %d0 is address of handler or 0 when not catched */
	lea	%sp@(4*4), %sp				/* pop args off stack */
	tstl	%d0
	beq	L_asm_handle_exception_not_catched

	/* %d0 contains address of exception handler */
	moveal	%d0, %a3

	/* restore temporary registers */
	moveal	%sp@+, %a1
	moveal	%sp@+, %a0
	movel	%sp@+, %d1
	movel	%sp@+, %d0
#if !defined(ENABLE_SOFTFLOAT)
	fmovemd	%fp0, %sp@(0)
	fmovemd	%fp1, %sp@(8)
	addal	#8*2, %sp
#endif

	jmp	%a3@

L_asm_handle_exception_not_catched:
	/* we did not find an exception handler in this stackframe */
	/* remove this frame and search in the one above */
	/* %a2 containts exception object ptr, %d2 the actual pv */

	/* remove temporary registers stored */
#if !defined(ENABLE_SOFTFLOAT)
	addal	#4*4 + 8*2, %sp
#else
	addal	#4*4, %sp
#endif

	moveal	%d2, %a3
	movel	%a3@(FrameSize), %d2

	moveal	%sp, %a0
	addal	%d2, %a0	/* %a0 now points to top of stackframe, where saved regs are */

	/* the saved registers have to be restored */
	/* XXX ugly hack: intsave and adrsave share one field */
	movel	%a3@(IntSave), %d0
	andil	#0x0000ffff, %d0	/* this is IntSave */
	cmpb	#0,%d0
	beq	L_asm_handle_ex_int_done
	movel	-(%a0), %d7

	cmpb	#1,%d0
	beq	L_asm_handle_ex_int_done
	movel	-(%a0), %d6

	cmpb	#2,%d0 
	beq	L_asm_handle_ex_int_done
	movel	-(%a0), %d5

L_asm_handle_ex_int_done:

	movel	%a3@(IntSave), %d0
	andil	#0xffff0000, %d0	/* this is AdrSave */
	lsrl	#8, %d0
	lsrl	#8, %d0

#if 0
	cmpb	#0, %d0
	beq	L_asm_handle_ex_adr_done
	moveal	-(%a0), %a5
#else
	cmpb	#0, %d0
	beq	L_asm_handle_ex_adr_done
	moveal	-(%a0), %fp

	cmpb	#1,%d0
	beq	L_asm_handle_ex_adr_done
	moveal	-(%a0), %a5
#endif

L_asm_handle_ex_adr_done:

#if !defined(ENABLE_SOFTFLOAT)
	movel	%a3@(FltSave), %d0
	cmpb	#0,%d0
	beq	L_asm_handle_ex_flt_done
	fmovemd	%a0@(-8), %fp7

	cmpb	#1,%d0
	beq	L_asm_handle_ex_flt_done
	fdmoved	%a0@(-16), %fp6
	
	cmpb	#2,%d0
	beq	L_asm_handle_ex_flt_done
	fdmoved	%a0@(-24), %fp5

L_asm_handle_ex_flt_done:
#else
	/* nothing to do */
#endif
	addal	%d2, %sp		/* remove old stackframe */
	moveal	%sp@+, %a3		/* make return address, new exception rasing address */
	subal	#2, %a3			/* it was a jsr %aX, which is 4 bytes long */
	jmp	L_asm_handle_exception_stack_loop


illegal
.long 0x23234242


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
