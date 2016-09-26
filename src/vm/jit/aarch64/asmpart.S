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
#include "vm/jit/aarch64/md-abi.hpp"

#include "vm/jit/abi-asm.hpp"
#include "vm/jit/methodheader.hpp"

#define SAVE_CNT (ARG_CNT + TMP_CNT + 1) // keep stack 16 byte - aligned


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

	.globl asm_flush_icache_range
	.type  asm_flush_icache_range, %function
	.globl asm_flush_dcache_range
	.type  asm_flush_dcache_range, %function


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
    ldr     a7, [t0], 8

	ldr		fa0, [t0], 8
	ldr		fa1, [t0], 8
	ldr		fa2, [t0], 8
	ldr		fa3, [t0], 8
	ldr		fa4, [t0], 8
	ldr		fa5, [t0], 8
	ldr		fa6, [t0], 8
	ldr		fa7, [t0], 8

    cbz		t1, L_asm_vm_call_method_stack_copy_done    /* check for stack arguments */

	sub		sp, sp, t1, LSL 3             /* calculate new SP for call      */
	tbz 	t1, 0, L_asm_vm_call_method_stack_even      /* stack is 16 byte aligned */
	sub 	sp, sp, 8

L_asm_vm_call_method_stack_even:
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
	sub		pv, lr, (L_asm_vm_call_method_recompute_pv - asm_vm_call_method)
 
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
L_asm_handle_exception_stack_loop:
	sub		sp, sp, 48 
	str		xptr, [sp]
	str		xpc, [sp, 8]
	str		lr, [sp, 24]
	str		zero, [sp, 32]

	mov		a0, lr 
	bl		md_asm_codegen_get_pv_from_pc
	str		a0, [sp, 16]
	mov		pv, a0

	ldr		a0, [sp]
	ldr		a1, [sp, 8]	
	mov		a2, pv 
	add		a3, sp, 48
 
	b		L_asm_handle_exception_continue


asm_handle_exception:
L_asm_handle_exception:
	sub		sp, sp, (SAVE_CNT) * 8

	SAVE_ARGUMENT_REGISTERS
	SAVE_TEMPORARY_REGISTERS(ARG_CNT)

	sub		sp, sp, 48
	str		xptr, [sp]
	str		pv, [sp, 16]
	str		lr, [sp, 24]
	mov		itmp3, 1
	str		itmp3, [sp, 32]

	mov		a0, xptr
	mov		a1, xpc
	mov		a2, pv
	add		a3, sp, (SAVE_CNT + 6) * 8

L_asm_handle_exception_continue:
	bl		exceptions_handle_exception

	cbz		a0, L_asm_handle_exception_not_catched

	mov		xpc, a0
	ldr		xptr, [sp]
	ldr		pv, [sp, 16]
	ldr		lr, [sp, 24]
	ldr		itmp3, [sp, 32]
	add		sp, sp, 48

	cbz		itmp3, L_asm_handle_exception_no_leaf

	RESTORE_ARGUMENT_REGISTERS
	RESTORE_TEMPORARY_REGISTERS(ARG_CNT)

	add		sp, sp, (SAVE_CNT) * 8

L_asm_handle_exception_no_leaf:
	br		xpc

L_asm_handle_exception_not_catched:
	ldr		xptr, [sp]
	ldr		pv, [sp, 16]
	ldr		lr, [sp, 24]
	ldr		itmp3, [sp, 32]
	add		sp, sp, 48

	cbz		itmp3, L_asm_handle_exception_no_leaf_stack

	add		sp, sp, (SAVE_CNT) * 8
	mov		itmp3, 0

L_asm_handle_exception_no_leaf_stack:
	ldr		t0i, [pv, FrameSize]
	add		t0, sp, t0

	ldr		t1i, [pv, IsLeaf]
	cbnz	t1i, L_asm_handle_exception_no_ra_restore

	ldr		lr, [t0, -8]
	sub		t0, t0, 8

L_asm_handle_exception_no_ra_restore:
	mov		xpc, lr
	ldr		t1i, [pv, IntSave]

	adr		t2, L_asm_handle_exception_save_int_end
	
	sub		t2, t2, t1, LSL 2
	br		t2

	ldr		s0, [t0, -10 * 8]
	ldr		s1, [t0, -9 * 8]
	ldr		s2,	[t0, -8 * 8]
	ldr		s3,	[t0, -7 * 8]
	ldr		s4,	[t0, -6 * 8]
	ldr		s5,	[t0, -5 * 8]
	ldr		s6,	[t0, -4 * 8]
	ldr		s7,	[t0, -3 * 8]
	ldr		s8,	[t0, -2 * 8]
	ldr		s9,	[t0, -1 * 8]

L_asm_handle_exception_save_int_end:
	sub		t0, t0, t1, LSL 3

	ldr		t1i, [pv, FltSave]
	adr		t2, L_asm_handle_exception_save_float_end

	sub		t2, t2, t1, LSL 2
	br		t2

	ldr		fs0, [t0, -8 * 8]
	ldr		fs1, [t0, -7 * 8]
	ldr		fs2, [t0, -6 * 8]
	ldr		fs3, [t0, -5 * 8]
	ldr		fs4, [t0, -4 * 8]
	ldr		fs5, [t0, -3 * 8]
	ldr		fs6, [t0, -2 * 8]
	ldr		fs7, [t0, -1 * 8]

L_asm_handle_exception_save_float_end:
	ldr		t0i, [pv, FrameSize]
	add		sp, sp, t0
	b		L_asm_handle_exception_stack_loop


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

		
/********************* function asm_cacheflush *********************************
*                                                                              *
*   Cache flushing on aarch64 wihtout EL1 access is somewhat more involved     *
*   Parts of the code were heavily inspired by the linux kernel                *
*                                                                              *
*   void asm_flush_icache_range(void *start, void *end);                       *
*   void asm_flush_dcache_range(void *start, void *end);                       *
*                                                                              *
*******************************************************************************/

	.align 2

/*
 * dcache_line_size - the CTR_EL0 register contains the minimum D-cache line size
 */
	.macro dcache_line_size, reg, tmp
	mrs 	\tmp, ctr_el0             /* read CTR                             */
	ubfm 	\tmp, \tmp, #16, #19      /* cache line size encoding             */ 
	mov 	\reg, #4                  /* bytes per word                       */
	lsl     \reg, \reg, \tmp          /* actual cache line size               */
	.endm

/*
 * icache_line_size - the CTR_EL0 register contains the minimum I-cache line size
 */
	.macro icache_line_size, reg, tmp
	mrs 	\tmp, ctr_el0             /* read CTR                             */
	and 	\tmp, \tmp, #0xf         /* cache line size encoding             */
	mov 	\reg, #4                  /* bytes per word                       */
	lsl     \reg, \reg, \tmp          /* actual cache line size               */
	.endm

asm_flush_icache_range:
	dcache_line_size a2, a3
	sub 	a3, a2, #1
	bic 	a4, a0, a3
flush_icache_1:
	dc 		cvau, a4
	add 	a4, a4, a2
	cmp 	a4, a1
	b.lo 	flush_icache_1
	dsb 	ish

	icache_line_size a2, a3
	sub 	a3, a2, #1
	bic 	a4, a0, a3
flush_icache_2:
	ic 		ivau, a4
	add 	a4, a4, a2
	cmp 	a4, a1
	b.lo 	flush_icache_2
	dsb 	ish
	isb
	ret

asm_flush_dcache_range:
	dcache_line_size a2, a3
	sub 	a3, a2, #1
	bic 	a4, a0, a3
flush_dcache_1:
	dc 		civac, a4
	add 	a4, a4, a2
	cmp 	a4, a1
	b.lo 	flush_dcache_1
	dsb 	sy
	ret


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
