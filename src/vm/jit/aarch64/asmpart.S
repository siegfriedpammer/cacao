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

#include "md-asm.hpp"
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
