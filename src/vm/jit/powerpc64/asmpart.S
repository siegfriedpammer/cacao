/* src/vm/jit/powerpc64/asmpart.S - Java-C interface functions for PowerPC64
		
   Copyright (C) 1996-2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software.text;  you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation;  either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY	;  without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program;  if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#include "config.h"

#define __ASSEMBLY__

#include "md-abi.hpp"
#include "md-asm.hpp"

#include "vm/jit/abi-asm.hpp"
#include "vm/jit/methodheader.hpp"


/* export functions ***********************************************************/

	.globl asm_vm_call_method_exception_handler
	.globl asm_vm_call_method_end

	.globl asm_cacheflush


/* asm_vm_call_method **********************************************************
*                                                                              *
*   This function calls a Java-method (which possibly needs compilation)       *
*   with up to 4 address parameters.                                           *
*                                                                              *
*   This functions calls the JIT-compiler which eventually translates the      *
*   method into machine code.                                                  *
*                                                                              *
*   C-prototype:                                                               *
*    javaobject_header *asm_calljavamethod (methodinfo *m,                     *
*         void *arg1, void *arg2, void *arg3, void *arg4);                     *
*                                                                              *
*******************************************************************************/
	/* this is the method header see src/vm/jit/methodheader.h */

	.align	3

	.long   0                           /* fltsave                            */
	.long   0                           /* intsave                            */
	.long   0                           /* isleaf                             */
	.long   0                           /* frame size                         */
	.quad   0                           /* codeinfo pointer                   */

#ifdef ENABLE_LIBJVM
	
	.globl asm_vm_call_method
	.globl asm_vm_call_method_int
	.globl asm_vm_call_method_long
	.globl asm_vm_call_method_float
	.globl asm_vm_call_method_double
	.section ".opd","aw"
	.align 3

	asm_vm_call_method:
	asm_vm_call_method_int:
	asm_vm_call_method_long:
	asm_vm_call_method_float:
	asm_vm_call_method_double:
		.quad	.asm_vm_call_method,.TOC.@tocbase,0
		.previous
		.size asm_vm_call_method, 24
		.type .asm_vm_call_method,@function
		.globl .asm_vm_call_method
#else
	asm_vm_call_method:
	.globl asm_vm_call_method
	asm_vm_call_method_int:
	.globl asm_vm_call_method_int
	asm_vm_call_method_long:
	.globl asm_vm_call_method_long
	asm_vm_call_method_float:
	.globl asm_vm_call_method_float
	asm_vm_call_method_double:
	.globl asm_vm_call_method_double
#endif

.asm_vm_call_method:
.asm_vm_call_method_int:
.asm_vm_call_method_long:
.asm_vm_call_method_float:
.asm_vm_call_method_double:
	mflr    r0
	std     r0,LA_LR_OFFSET(sp)
	stdu    sp,-40*8(sp)
	
	std     s0,8*8(sp)                /* save used callee saved registers     */
	std     a0,9*8(sp)                /* save method pointer for compiler     */

	std     pv,11*8(sp)               /* save PV register                     */

	std     itmp3,12*8(sp)            /* registers r14-r31 are callee saved   */
	stfd	ftmp1,13*8(sp)            /* registers f14-f31 are callee saved   */
	stfd	ftmp2,14*8(sp)


	SAVE_TEMPORARY_REGISTERS(15)     
	mr	s0, sp			/* save stack pointer */

	/* a1 contains a pointer to a unit64_t structure filled with all INT_ARG_REG,
	followed by ADR_ARG_CNT and FLT_ARG_CNT, afterwards what else needs to be copied onto
	the stack 
	a2 contains the number of additional stack slots to be copied
	*/

L_register_copy:
	mr	t1, a1
	mr	t2, a2

	ld	a0 ,  0*8(t1)
	ld	a1 ,  1*8(t1)
	ld	a2 ,  2*8(t1)
	ld	a3 ,  3*8(t1)
	ld	a4 ,  4*8(t1)
	ld	a5 ,  5*8(t1)
	ld	a6 ,  6*8(t1)
	ld	a7 ,  7*8(t1)

	lfd	fa0 , 8*8(t1)
	lfd	fa1 , 9*8(t1)
	lfd	fa2 ,10*8(t1)
	lfd	fa3 ,11*8(t1)
	lfd	fa4 ,12*8(t1)
	lfd	fa5 ,13*8(t1)
	lfd	fa6 ,14*8(t1)
	lfd	fa7 ,15*8(t1)
	lfd	fa8 ,16*8(t1)
	lfd	fa9 ,17*8(t1)
	lfd	fa10,18*8(t1)
	lfd	fa11,19*8(t1)
	lfd	fa12,20*8(t1)

	mr.	t2,t2
	beq L_stack_copy_done

L_stack_copy:
	addi	t1,t1,20*8		/* before first possible stack slot arg */
	mr	t3,t2			/* argument counter */
	sldi	t2,t2,8			/* calculate size of stack */
	sub	sp,sp,t2		/* increase the stack */
	mr	t2,sp			/* t2 points to bottom of stack now */

L_stack_copy_loop:
	addi	t1,t1,8			/* next possible stack slot to copy */
	mr.	t3,t3			/* more stack slots to copy ? */
	beq	L_stack_copy_done
	ld	itmp3, 0(t1)
	std	itmp3, 0(t2)
	addi	t2,t2,8
	addi	t3,t3,-1
	b L_stack_copy_loop

L_stack_copy_done:
	mr	itmp1, s0		/* fake invokevirtual invocation */
	addi	itmp1, itmp1, 9*8	/* address of methods pv */
	ld 	pv,0*8(itmp1)
	mtctr   pv
	bctrl
1:
	mflr    itmp1
	addi    pv,itmp1,(.asm_vm_call_method - 1b)@l

L_asm_vm_call_method_return:
	mr      sp,s0                     /* restore the function's sp            */

	ld      s0,8*8(sp)                /* restore used callee saved registers  */

	ld      pv,11*8(sp)               /* save PV register                     */

	ld      itmp3,12*8(sp)
	lfd     ftmp1,13*8(sp)            /* registers f14-f31 are callee saved   */
	lfd     ftmp2,14*8(sp)

	RESTORE_TEMPORARY_REGISTERS(15) 

	ld     r0,40*8+LA_LR_OFFSET(sp)
	mtlr   r0
	addi   sp,sp,40*8
	blr

asm_vm_call_method_exception_handler:
	mr      r3,itmp1
	bl      builtin_throw_exception
	b       L_asm_vm_call_method_return

asm_vm_call_method_end:
	nop


/* asm_cacheflush **************************************************************
	assumes 128 byte cache line size.
	All registers used may be trashed for fun and profit.
*******************************************************************************/

	.section ".opd","aw"
	.align 3
asm_cacheflush:
		.quad	.asm_cacheflush,.TOC.@tocbase,0
		.previous
		.size asm_cacheflush, 24
		.type .asm_cacheflush,@function
		.globl .asm_cacheflush 
.asm_cacheflush:
	/* construct the AND mask */
	li	r6,   0xffffffffffff8000
	ori	r6,r6,0x000000000000ff80

	add     r4,r3,r4
	and.	r3,r3,r6
	addi    r4,r4,127
	and.	r4,r4,r6
	mr      r5,r3
1:
	cmpld   r3,r4
	bge     0f
	dcbst   0,r3
	addi    r3,r3,128
	b       1b
0:
	sync
1:
	cmpld   r5,r4
	bge     0f
	icbi    0,r5
	addi    r5,r5,128
	b       1b
0:
	sync
	isync
	blr


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
