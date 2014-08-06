/* src/vm/jit/arm/asmpart.S - Java-C interface functions for ARM

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

#include <sys/syscall.h>

#include "vm/jit/aarch64/md-asm.hpp"

#include "vm/jit/methodheader.hpp"


	.file "asmpart.S"
	.text
	.align 2

/* export functions ***********************************************************/

	.globl asm_vm_call_method
	.type  asm_vm_call_method, %function
	.globl asm_vm_call_method_int
	.type  asm_vm_call_method_int, %function
	.globl asm_vm_call_method_long
	.type  asm_vm_call_method_long, %function
	.globl asm_vm_call_method_float
	.type  asm_vm_call_method_float, %function
	.globl asm_vm_call_method_double
	.type  asm_vm_call_method_double, %function
	.globl asm_vm_call_method_exception_handler
	.globl asm_vm_call_method_end

	.globl asm_handle_exception
	.globl asm_handle_nat_exception

	.globl asm_abstractmethoderror

	.globl asm_cacheflush
	.type  asm_cacheflush, %function


/* asm_vm_call_method **********************************************************

   This function calls a Java-method (which possibly needs compilation)
   with up to 4 address parameters.

   This functions calls the JIT-compiler which eventually translates the
   method into machine code.

*******************************************************************************/

	.align  2

	.long   0                           /* fltsave                            */
	.long   1                           /* intsave                            */
	.long   0                           /* isleaf                             */
	.long   0                           /* frame size                         */
	.quad   0                           /* codeinfo pointer                   */

asm_vm_call_method:
asm_vm_call_method_int:
asm_vm_call_method_long:
asm_vm_call_method_float:
asm_vm_call_method_double:
	sub		sp, sp, 32                    /* make room for saved registers  */
	str		lr, [sp]                      /* save link register             */

	str		s0, [sp, 8]                   /* save registers                 */

	str     a0, [sp, 16]                  /* save method PV                 */

	mov		t0, a1                        /* address of the argument array  */
	mov     t1, a2                        /* stack argument count           */
    mov     s0, sp                        /* save stack pointer             */

	ldr     a0, [t0], 8                   /* prepare argument registers     */
    ldr     a1, [t0], 8
    ldr     a2, [t0], 8
    ldr     a3, [t0], 8
    ldr     a4, [t0], 8
    ldr     a5, [t0], 8
    ldr     a6, [t0], 8

	ldr		fa0, [t0], 8
	ldr		fa1, [t0], 8
	ldr		fa2, [t0], 8
	ldr		fa3, [t0], 8
	ldr		fa4, [t0], 8
	ldr		fa5, [t0], 8
	ldr		fa6, [t0], 8

    cbz		t1, L_asm_vm_call_method_stack_copy_done    /* check for stack arguments */

	sub		sp, sp, t1, LSL 3             /* calculate new SP for call      */
	mov		t2, sp                        /* tmp 'SP' for loop              */

L_asm_vm_call_method_stack_copy_loop:
	ldr		t3, [t0], 8                   /* load argument into t3, inc addr in t0 by 8 after */
	str		t3, [t2], 8                   /* store arg in the new stack, inc addr of tmp 'sp' after */

	sub		t1, t1, 1                     /* decrement stack argument count by one */
	cbnz	t1, L_asm_vm_call_method_stack_copy_loop

L_asm_vm_call_method_stack_copy_done:
	ldr		mptr, [s0, 16]               /* get address of PV                      */
	mov     pv, mptr                     /* load PV                                */
	blr		pv 
	
L_asm_vm_call_method_recompute_pv:
	sub		pv, lr, (L_asm_vm_call_method_recompute_pv - asm_vm_call_method) + 8

L_asm_vm_call_method_recompute_return:
	mov		sp, s0                      /* restore stack pointer            */
	
	ldr		lr, [sp]				    /* restore load register            */
	ldr		s0, [sp, 8]
	
	add		sp, sp, 32 
	ret

asm_vm_call_method_exception_handler:
	mov		sp, s0                    /* restore stack pointer            */
	mov		a0, xptr                  /* exception pointer is only arg    */
	bl		builtin_throw_exception   /* throw the exception              */

asm_vm_call_method_end:
	b		L_asm_vm_call_method_recompute_return


/********************* function asm_handle_exception ***************************
*                                                                              *
*   This function handles an exception. It does not use the usual calling      *
*   conventions. The exception pointer is passed in REG_ITMP1 and the          *
*   pc from the exception raising position is passed in REG_ITMP2. It searches *
*   the local exception table for a handler. If no one is found, it unwinds    *
*   stacks and continues searching the callers.                                *
*                                                                              *
*   void asm_handle_exception (exceptionptr, exceptionpc);                     *
*                                                                              *
*******************************************************************************/
	

asm_handle_nat_exception:
L_asm_handle_nat_exception:
	sub		sp, sp, 16
	str		a0, [sp]

	mov		a0, lr
	bl		md_asm_codegen_get_pv_from_pc
	mov		pv, a0

	ldr		a0, [sp]
	add		sp, sp, 16
	/*TODO:maybe make a macro out of it!!!*/
//	SAVE_ARGUMENT_REGISTERS  
//	mov   a0, lr
//	bl    md_asm_codegen_get_pv_from_pc
//	mov   ip, res1
//	RESTORE_ARGUMENT_REGISTERS  
	/* fall through */


asm_handle_exception:
//	stmfd sp!, {r0 - r3}               /* save possible used registers    */
//	mov   itmp3, #1                    /* set maybe-leaf flag             */
//	mov   a3, #(4*4)                   /* prepare a3 for handle_exception */

asm_handle_exception_loop:
//	stmfd sp!, {ip,lr}                 /* call exception helper here!     */
//	mov   a0, xptr                     /* pass exception pointer          */
//	mov   a1, xpc                      /* pass exception pointer          */
//	mov   a2, ip                       /* pass data segment pointer       */
//	add   a3, sp, a3                   /* calculate Java sp into a3...    */
//	add   a3, a3, #(2*4)
//	bl    exceptions_handle_exception
//	ldmfd sp!, {ip,lr}
//
//	tst   a0, a0
//	beq   asm_handle_exception_not_catched
//
//	mov   xpc, a0                      /* move handlerpc into xpc         */
//	tst   itmp3,itmp3                  /* if this is a lead method ...    */
//	ldmnefd sp!, {r0 - r3}             /* restore argument registers      */
//
//	mov   pc, xpc                      /* jump to handler                 */

asm_handle_exception_not_catched:
//	tst   itmp3,itmp3                  /* if this is a lead method ...    */
//	addne sp, sp, #(4*4)               /* remove maybe-leaf stackframe    */
//	movne itmp3, #0                    /* remove maybe-leaf flag          */
//
//	ldr   a2, [ip, #FrameSize]         /* t2 = frame size                 */
//	add   a0, sp, a2                   /* t0 = pointer to save area       */
//	ldr   a1, [ip, #IsLeaf]            /* t1 = is leaf procedure          */
//	tst   a1, a1                       /* if is leaf ...                  */
//	ldreq lr, [a0, #-4]!               /* ... restore RA                  */
//	mov   xpc, lr                      /* the new xpc is RA               */
//
//	ldr   a1, [ip, #IntSave]           /* t1 = saved int register count   */
//	rsb   a1, a1, #5                   /* t1 = count of unsaved registers */
//	sub   a1, a1, #1
//	add   pc, pc, a1, lsl #2           /* do not load unsaved registers   */
//	ldr   v1, [a0, #-20]               /* ... but restore the other ones  */
//	ldr   v2, [a0, #-16]
//	ldr   v3, [a0, #-12]
//	ldr   v4, [a0, #- 8]
//	ldr   v5, [a0, #- 4]
//
//	add   sp, sp, a2                   /* unwind stack (using t2)         */
//	mov   a3, #0                       /* prepare a3 for handle_exception */
//
//	/*TODO:maybe make a macro out of it!!!*/
//	SAVE_ARGUMENT_REGISTERS  
//	mov   a0, lr
//	bl    md_asm_codegen_get_pv_from_pc
//	mov   ip, res1
//	RESTORE_ARGUMENT_REGISTERS  
//
//	b     asm_handle_exception_loop


/* asm_abstractmethoderror *****************************************************

   Creates and throws an AbstractMethodError.

*******************************************************************************/

asm_abstractmethoderror:
	sub		sp, sp, 16					/* create stackframe                  */
	str		lr,	[sp]                    /* save return address                */

	add		a0, sp, 16                  /* original SP is argument for java   */
	mov		a1, lr						/* pass exception address             */
	bl		exceptions_asm_new_abstractmethoderror

	ldr		lr, [sp]					/* resture return address             */
	add		sp, sp, 16                  /* remove stackframe                  */

	mov		xptr, a0					/* get exception pointer              */
	sub		xpc, lr, 4                  /* exception address is lr - 4        */
	b		L_asm_handle_nat_exception


//	stmfd sp!, {lr}                     /* save return address                */
//	add   a0, sp, #(1*4)                /* pass java sp                       */
//	mov   a1, lr                        /* pass exception address             */
#if defined(ARM_NO_THUMB_IW)
//	bl    exceptions_asm_new_abstractmethoderror
#else
//	blx   exceptions_asm_new_abstractmethoderror
#endif
//	ldmfd sp!, {lr}                     /* restore return address             */
//
//	mov   xptr, res1                    /* get exception pointer              */
//	sub   xpc, lr, #4                   /* exception address is ra - 4        */
//	b     asm_handle_nat_exception

		
/********************* function asm_cacheflush *********************************
*                                                                              *
*   TODO: document me                                                          *
*                                                                              *
*   void asm_cacheflush(void *p, s4 size);                                     *
*                                                                              *
*******************************************************************************/

asm_cacheflush:
	isb
	dsb sy

/* disable exec-stacks ********************************************************/

#if defined(__linux__) && defined(__ELF__)
	.section .note.GNU-stack,"",%progbits
#endif


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: asm
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
